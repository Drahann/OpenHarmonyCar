#include "NaviInterface.h"

#include "../lcmtype_dir/grid_map_t.h"
#include "../lcmtype_dir/laser_t.h"
#include "../lcmtype_dir/path_t.h"
#include "../lcmtype_dir/pose_t.h"
#include "../lcmtype_dir/robot_control_t.h"
#include "../udp/UdpClient.h"
#include "lcm/lcm.h"

#include <dirent.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

using namespace std;

// #define MAP_PATH_NAME "/mnt/cf/mapfile"
// #define MAP_PATH_NAME1 "/mnt/cf/mapfile/"
#define MAP_PATH_NAME "/data/test"
#define MAP_PATH_NAME1 "/data/test/"

#define LCM_CMD_LoadMap 10
#define LCM_CMD_SetWaypoint 20
#define LCM_CMD_Arrived 21
#define LCM_CMD_Follow 22
#define LCM_CMD_ClearWP 23
#define LCM_CMD_CreateMap 30
#define LCM_CMD_AskMap 80
#define LCM_CMD_SaveMap 32
#define LCM_CMD_AllMapName 33
#define LCM_CMD_ModeToLocalization 34
#define LCM_CMD_ReceiveMap 35
#define LCM_CMD_Quit 90
#define LCM_CMD_SendMapFile 104
#define LCM_CMD_SendMapNames 102
#define LCM_CMD_BeginLocalization 103
#define LCM_CMD_FullPathCoverage 127

lcm_t *lcm;
char g_strfileName[512];
static char nowusestrfileName[512];
char strfileName0[512];

unsigned char ucLoadmapflag = 0;

bool UpdateMapCallBackFunc(void);
bool UpdateMapBeginCallBackFunc(void);

void *LCMRecvTask(void *arg) {
    while (1) {
        lcm_handle(lcm);
    }
}

vector<Pose> ReadFullPathFromFile(const string& filepath) {
    vector<Pose> path;
    ifstream infile(filepath);
    if (!infile.is_open()) {
        cerr << "Cannot open path file: " << filepath << endl;
        return path;
    }

    double x, y;
    char delim;
    while (infile >> x >> delim >> y) {
        Pose p;
        p.x = x;
        p.y = y;
        p.theta = 0;
        path.push_back(p);
    }
    return path;
}

bool GetFileNameInDir(const char *strPath, char *pstrFileName[],
                      const char *strSuffix, int &nFileNum) {
    int i = 0;
    int nLengthSuffix = 0;
    char *strTemp;
    char strFullName[512];
    struct dirent *pDirent;
    struct stat statBuff;
    DIR *pDIR;

    // printf("opendir %s\n",strPath);

    if (strPath == NULL)
        return false;
    if ((pDIR = opendir(strPath)) == NULL)
        return false;
    if (strSuffix != NULL)
        nLengthSuffix = strlen(strSuffix);

    while ((pDirent = readdir(pDIR)) != NULL) {

        if (strcmp(pDirent->d_name, ".") != 0 &&
            strcmp(pDirent->d_name, "..") != 0) {

            strcpy(strFullName, strPath);
            strcat(strFullName, "/");
            strcat(strFullName, pDirent->d_name);
            printf("%s \n", pDirent->d_name);
            fflush(stdout);

            chmod(strFullName, S_IRWXU | S_IRWXG | S_IRWXO);

            if (S_ISDIR(statBuff.st_mode) == 0) {
                if (strSuffix != NULL) {
                    strTemp = pDirent->d_name + strlen(pDirent->d_name) -
                              nLengthSuffix;
                    if (strncasecmp(strTemp, strSuffix, nLengthSuffix) == 0) {
                        if (pstrFileName != NULL) {
                            strcpy(pstrFileName[i], pDirent->d_name);
                            if (i == nFileNum - 1)
                                break;
                        }

                        i++;
                    }
                }
            }
        }
    }

    if (pstrFileName == NULL)
        nFileNum = i;
    closedir(pDIR);

    return true;
}
void SendAllMapNames() {
    int nFileName = 0;
    GetFileNameInDir(MAP_PATH_NAME, NULL, "txt", nFileName);
    if (nFileName <= 0) {
        return;
    }
    char *pStrFile[nFileName];

    for (int i = 0; i < nFileName; i++) {
        pStrFile[i] = new char[512];
    }

    GetFileNameInDir(MAP_PATH_NAME, pStrFile, "txt", nFileName);
    long systemtime = (long)time((time_t *)0);

    usleep(50000);
    robot_control_t cmd;
    double dend = 0;
    unsigned char bend = 0;
    int i = 0;

    cmd.utime = systemtime;
    cmd.commandid = 102;
    cmd.ndparams = 1;
    cmd.dparams = &dend;
    cmd.nsparams = nFileName;
    cmd.sparams = pStrFile;
    cmd.niparams = 1;
    cmd.iparams = (signed char *)&i;
    cmd.nbparams = 1;
    cmd.bparams = &bend;

    robot_control_t_publish(lcm, "ROBOT_CONTROL", &cmd);
    for (int i = 0; i < nFileName; i++) {
        delete pStrFile[i];
    }
}

void *SendMapThreadProc(LPVOID pPara) {
    long systemtime1 = (long)time((time_t *)0);
    signed char iparams1[2];
    robot_control_t cmd1;
    double dend1 = 0;
    unsigned char bend1 = 0;

    ifstream infile;
    infile.open(g_strfileName, ios::in);

    if (!infile) {
        return NULL;
    }

    double _range;
    double _resolution;
    int nwidth;
    int nheigh;

    infile >> _range;
    infile >> _resolution;
    infile >> nheigh;
    infile >> nwidth;

    infile.clear();
    infile.close();

    int no = 0;
    int num = 0;

    int iparams[2];
    double percent = 0.0;

    FILE *fp = fopen(g_strfileName, "rb");
    size_t len = 0;
    char *line = NULL;
    ssize_t read;

    if (fp == NULL) {
        return NULL;
    }

    while ((read = getline(&line, &len, fp)) != -1) {

        int bytenum = (int)read;
        usleep(50000);
        no++;
        robot_control_t cmd;
        long systemtime = (long)time((time_t *)0);

        cmd.utime = systemtime;
        cmd.commandid = 104;
        cmd.nsparams = 0;
        cmd.ndparams = 1;
        percent = (double)(no) / (double)(nheigh + 1);
        cmd.dparams = &percent;
        cmd.niparams = 2;
        if (no == 1) {

            iparams[0] = 1;
        } else {
            iparams[0] = 0;
        }

        iparams[1] = 0;

        cmd.iparams = (signed char *)iparams;
        cmd.nbparams = bytenum;
        cmd.bparams = (unsigned char *)line;

        robot_control_t_publish(lcm, "MAPFILE", &cmd);
    }
    if (!line) {
        free(line);
    }
    usleep(100000);
    {

        cmd1.utime = systemtime1;
        cmd1.commandid = 104;
        cmd1.ndparams = 1;
        cmd1.dparams = &dend1;
        cmd1.nsparams = 0;
        cmd1.niparams = 2;
        iparams1[0] = 0;
        iparams1[1] = 1;

        cmd1.iparams = (signed char *)iparams1;
        cmd1.nbparams = 1;
        cmd1.bparams = &bend1;

        printf("send over\n");
        fflush(stdout);
        robot_control_t_publish(lcm, "MAPFILE", &cmd1);
    }
    fclose(fp);

    return NULL;
}

void SendWall(vector<Pose> &vtWall) {
    long systemtime = (long)time((time_t *)0);
    double dend = 0;
    robot_control_t cmd;
    unsigned char bend = 0;
    int i = 0;

    int size = vtWall.size();

    double *pdata = new double[size * 2];
    for (int i = 0; i < size; i++) {
        Pose wall;
        wall = vtWall.at(i);

        pdata[2 * i + 0] = wall.x;
        pdata[2 * i + 1] = wall.y;
    }
    usleep(100000);

    path_t path;
    path.utime = systemtime;
    int j = 0;

    double *pathdot[size];

    for (j = 0; j < size; j++) {
        pathdot[j] = (pdata + j * 2);
    }
    path.xyr = pathdot;
    path.length = size;

    if (pdata != NULL) {
        delete pdata;
    }
    printf("send wall****\n");
    fflush(stdout);
}
void SendLoadMapError() {

    robot_control_t cmd;
    double dend = 0;
    unsigned char bend = 0;
    int i = 0;
    long systemtime = (long)time((time_t *)0);

    cmd.utime = systemtime;
    cmd.commandid = 13;
    cmd.ndparams = 1;
    cmd.dparams = &dend;
    cmd.nsparams = 0;
    cmd.niparams = 1;
    cmd.iparams = (signed char *)&i;
    cmd.nbparams = 1;
    cmd.bparams = &bend;

    robot_control_t_publish(lcm, "ROBOT_CONTROL", &cmd);
}
void CmdRespondSaveMap(void) {
    robot_control_t cmd;
    int i = 0;
    long systemtime = (long)time((time_t *)0);
    cmd.utime = systemtime;
    cmd.commandid = 101;
    cmd.ndparams = 0;
    cmd.nsparams = 0;
    cmd.niparams = 1;
    cmd.iparams = (signed char *)&i;
    cmd.nbparams = 0;

    robot_control_t_publish(lcm, "ROBOT_CONTROL", &cmd);
}
#if 1
void ReviseWallHandle(const lcm_recv_buf_t *rbuf, const char *channel,
                      const path_t *collisiondata, void *user) {
    if (NULL == collisiondata) {
        return;
    }
    NAVI_GetCollisionData(collisiondata->xyr, collisiondata->length);
}
#endif
void LaserDataHandle(const lcm_recv_buf_t *rbuf, const char *channel,
                     const laser_t *laserdata, void *user) {
    NAVI_PutLaserData(laserdata->nranges, laserdata->ranges,
                      laserdata->nintensities, laserdata->intensities);
}

void PoseDataHandle(const lcm_recv_buf_t *rbuf, const char *channel,
                    const pose_t *encoderdata, void *user) {

    Pose encoderPos;
    encoderPos.x = encoderdata->pos[0];
    encoderPos.y = encoderdata->pos[1];
    encoderPos.theta = encoderdata->pos[2];
    NAVI_PutEncoderData(&encoderPos);
}

void GetPathCallBackFunc(double *pathxyr, int pnum, int type) {
    if (type == 1) {
        long systemtime = (long)time((time_t *)0);
        path_t path;
        path.utime = systemtime;
        int j = 0;

        double *pathdot[pnum];

        for (j = 0; j < pnum; j++) {
            pathdot[j] = (pathxyr + j * 2);
        }
        path.xyr = pathdot;
        path.length = pnum;
        path_t_publish(lcm, "PATH", &path);

        // printf("publish-----------------path\n");
    }

    if (type > 1) {
        int k = 0;

        if (pnum % 120 == 0) {
            k = pnum / 120;
        } else {
            k = pnum / 120 + 1;
        }

        for (int i = 0; i < k; i++) {

            long systemtime = (long)time((time_t *)0);

            robot_control_t cmd;
            double dend = 0;
            unsigned char bend = 0;
            int num = 120;

            if (pnum % 120 != 0 && (i == (k - 1))) {
                num = pnum % 120;
            }
            signed char iparam[2];
            iparam[0] = i;
            iparam[1] = type - 2;
            cmd.utime = systemtime;
            cmd.commandid = 73;
            cmd.ndparams = num;
            cmd.dparams = pathxyr + i * 120;
            cmd.nsparams = 0;
            cmd.niparams = 2;
            cmd.iparams = iparam;
            cmd.nbparams = 1;
            cmd.bparams = &bend;

            robot_control_t_publish(lcm, "SERVICE_COMMAND", &cmd);
        }
    }
}

void RobotCtrlHandle(const lcm_recv_buf_t *rbuf, const char *channel,
                     const robot_control_t *robotctrldata, void *user) {
    int searchType = 3;
    switch (robotctrldata->commandid) {
    case 7:
        if (robotctrldata->ndparams >= 7) {
            long systemtime = (long)time((time_t *)0);

            robot_control_t cmd;
            double dend = 0;
            unsigned char bend = 0;
            int i = 0;
            RobotConfig config;

            // config.laserconfig.positionX = robotctrldata->dparams[0];
            // config.laserconfig.positionY = robotctrldata->dparams[1];
            // config.laserconfig.positionZ = 0.584;
            // config.laserconfig.max_range = robotctrldata->dparams[2];
            // config.robotconfig.radius = robotctrldata->dparams[3];
            // config.robotconfig.width = robotctrldata->dparams[3];
            // config.ScanMatchconfig.xScanMatchRange =
            // robotctrldata->dparams[4]; config.ScanMatchconfig.yScanMatchRange
            // = robotctrldata->dparams[5];
            // config.ScanMatchconfig.thetaScanMatchRange =
            // robotctrldata->dparams[6];

            // TODO 重写配置，其配置值固定
            // config.laserconfig.positionX = 0.00;
            config.laserconfig.positionX = 0.11;
            config.laserconfig.positionY = 0.00;
            config.laserconfig.positionZ = 0.13;
            config.laserconfig.max_range = 8.00;
            config.robotconfig.radius = 0.23;
            config.robotconfig.width = 1.30;
            config.ScanMatchconfig.xScanMatchRange = 0.2;
            config.ScanMatchconfig.yScanMatchRange = 0.2;
            config.ScanMatchconfig.thetaScanMatchRange = 15.0;
            //////////////////////////
            NAVI_SetLocationType(INIT);
            cmd.utime = systemtime;
            cmd.commandid = 6;
            cmd.ndparams = 1;
            cmd.dparams = &dend;
            cmd.nsparams = 0;
            cmd.niparams = 1;
            cmd.iparams = (signed char *)&i;
            cmd.nbparams = 1;
            cmd.bparams = &bend;

            robot_control_t_publish(lcm, "ROBOT_CONTROL", &cmd);
            NAVI_SetConfig(config);
        }
        break;
    case 8: // REVISE POSE	似乎没用上
        if (robotctrldata->ndparams >= 6) {
#if 1
            Pose pos;
            double xSearchRange;
            double ySearchRange;
            double thetaSearchRange;

            pos.x = robotctrldata->dparams[0];
            pos.y = robotctrldata->dparams[1];
            pos.theta = robotctrldata->dparams[2];
            xSearchRange = robotctrldata->dparams[3];
            ySearchRange = robotctrldata->dparams[4];
            thetaSearchRange = robotctrldata->dparams[5];

            unsigned char flag = robotctrldata->bparams[0];

            NAVI_RevisePoseAndRange(pos, xSearchRange, ySearchRange,
                                    thetaSearchRange, flag);
#endif
        }

        break;
    case 9: // particle filter
        break;
    case 10: //加载地图
        Pose initPos;
        Pose initRange;
        char strfileName0[512];
        strcpy(strfileName0, MAP_PATH_NAME1);
        strcpy(nowusestrfileName, MAP_PATH_NAME1);
        // TODO使用默认地图
        if (0 < robotctrldata->nsparams) {
            strcat(strfileName0, robotctrldata->sparams[0]);
        } else {
            printf("error name use defult name\n");
            fflush(stdout);
            strcat(strfileName0, "defultMap.txt");
        }
        //////////////////////

        // if(robotctrldata->nsparams>=1)
        // {
        // 	strcat(strfileName0,robotctrldata->sparams[0]);
        // 	strcat(nowusestrfileName,"update");
        // 	strcat(nowusestrfileName,robotctrldata->sparams[0]);
        // }
        if (robotctrldata->ndparams >= 7) {
            vector<Pose> vtWall;
            // initRange.x = robotctrldata->dparams[0]; //到底取哪个\E5??
            // initRange.y = robotctrldata->dparams[1];
            // initRange.theta = robotctrldata->dparams[2];
            initRange.x = 0.2;
            initRange.y = 0.2;
            initRange.theta = 15;

            initPos.x = robotctrldata->dparams[4];
            initPos.y = robotctrldata->dparams[5];
            initPos.theta = robotctrldata->dparams[6];
            if (NAVI_LoadMapAndLoc(strfileName0, initPos, initRange, vtWall)) {
                ucLoadmapflag = 1;
            } else {
                ucLoadmapflag = 0;
            }
            SendLoadMapError();
            if (vtWall.size() >= 0) {
                SendWall(vtWall);
            }
        }
        printf("load map %s\n", strfileName0);
        fflush(stdout);
        break;
    case 12:

        if (robotctrldata->niparams >= 1) {
            if (robotctrldata->iparams[0] == 0) {
                NAVI_SetLocationType(SCANMATCH);
            }
            if (robotctrldata->iparams[0] == 1) {
                NAVI_SetLocationType(SLAM);
            }
        }

        break;
    case 19: //似乎与位姿估计有关，用不到
        NAVI_SetRefineMode(1);
        break;
    case 20:
        NAVI_SetSearchType(searchType);
        Pose goal;
        goal.x = robotctrldata->dparams[0];
        goal.y = robotctrldata->dparams[1];
        goal.theta = robotctrldata->dparams[2];
        printf("set goal case = 20 x = %f, y =%f\n", goal.x, goal.y);
        fflush(stdout);
   
        NAVI_VisionRecenter();
    
        NAVI_SetGoalPoint(goal);

        break;
    case 23:
        //TODO delete goal
        printf("start delete goal\n");
        fflush(stdout);
        //////////////////////////////////
        NAVI_DeleteGoal();
        break;
    case 24:
        break;
    case 26: //似乎没用上
        NAVI_SetFind();
        break;
    case 30: {
        // 开始建图
        double metersPerPixel = 0.05;
        printf("create map \n");
        fflush(stdout);

        if (robotctrldata->ndparams >= 1) {
            if (robotctrldata->dparams[0] > 0.01 &&
                robotctrldata->dparams[0] < 0.16)
                metersPerPixel = robotctrldata->dparams[0];
            printf("create map %f\n", robotctrldata->dparams[0]);
            fflush(stdout);
        }
        NAVI_CreateMap(metersPerPixel);
    }
        { //主控中没有51号指令
            robot_control_t rc_cmd;
            double pose[3];
            signed char flag = 1;
            pose[0] = 0;
            pose[1] = 0;
            pose[2] = 0;
            rc_cmd.utime = 0;
            rc_cmd.commandid = 51;
            rc_cmd.ndparams = 3;
            rc_cmd.dparams = pose;
            rc_cmd.nsparams = 0;
            rc_cmd.niparams = 1;
            rc_cmd.iparams = &flag;
            rc_cmd.nbparams = 0;
            robot_control_t_publish(lcm, "SERVICE_COMMAND", &rc_cmd);
        }
        break;
    case 32:
        printf("save map\n");
        fflush(stdout);
        char strfileName[512];
        char strfileNameunprob[512];
        strcpy(strfileName, MAP_PATH_NAME1);
        strcpy(strfileNameunprob, MAP_PATH_NAME1);
#if 1
        if (0 < robotctrldata->nsparams) {
            strcat(strfileName, robotctrldata->sparams[0]);
            strcat(strfileNameunprob, "unprob");
            strcat(strfileNameunprob, robotctrldata->sparams[0]);
        } else {
            printf("error name use defult name\n");
            fflush(stdout);
            strcat(strfileNameunprob, "unprobdefultMap.txt");
            strcat(strfileName, "defultMap.txt");
        }
#endif
        NAVI_SaveMap(strfileNameunprob);
        { //主控中没有52号指令
            robot_control_t rc_cmd;
            signed char flag = 0;
            rc_cmd.utime = 0;
            rc_cmd.commandid = 52;
            rc_cmd.ndparams = 0;
            rc_cmd.nsparams = 0;
            rc_cmd.niparams = 1;
            rc_cmd.iparams = &flag;
            rc_cmd.nbparams = 0;
            robot_control_t_publish(lcm, "SERVICE_COMMAND", &rc_cmd);
        }
        NAVI_OptimizeMap(strfileName);
        CmdRespondSaveMap();
        break;
    case 33:
        printf("all map\n");
        fflush(stdout);
        SendAllMapNames();
        break;

    case 34:
        char strfileName1[512];
        strcpy(strfileName1, MAP_PATH_NAME1);
        if (robotctrldata->nsparams >= 1) {
            strcat(strfileName1, robotctrldata->sparams[0]);
            if (remove(strfileName1) == -1) {
                printf("delete map failed\n");
                fflush(stdout);
            }
        }

    case 36: //遥控模式和自动模式
    {
        if (1 != robotctrldata->nbparams) {
            break;
        }
        if (0 == robotctrldata->bparams[0]) {
            NAVI_SetOdmSwitch(false);
        } else if (1 == robotctrldata->bparams[0]) {
            NAVI_SetOdmSwitch(true);
        }
    } break;
    case 37: //用不到
    {
        printf("case 37\n");
        fflush(stdout);
        NAVI_SetBackforward(true);
    } break;
    case 39: //扩展地图，暂时用不到
    {
        if (1 != robotctrldata->nbparams) {
            break;
        }
        if (0 == robotctrldata->bparams[0]) {
            printf("end modify map\n");
            fflush(stdout);
            NAVI_Setmodifymap(false);
        } else if (1 == robotctrldata->bparams[0]) {
            printf("begin modify map\n");
            fflush(stdout);
            NAVI_Setmodifymap(true);
        }
    } break;
    case 40: //自动充电，暂时用不到
    {

        printf("begin autocharge !\n");
        fflush(stdout);
        NAVI_Setautocharge(true);
    } break;
    case 41: //用不到
    {
        if (1 == robotctrldata->iparams[0]) {
            NAVI_Savevisiondate(1);
        } else {
            NAVI_Savevisiondate(0);
        }
    } break;
    case 48: //地图更新
    {
        int iupdate = robotctrldata->iparams[0];
        if (1 == iupdate) {
            printf("begin update !\n");
            fflush(stdout);
            NAVI_Setexpandmode(false);
            NAVI_Setmanualupdate(false);
            bool mode = true;
            NAVI_Setupdatemode(mode);
            NAVI_OpenVisionDate();
        }
        if (0 == iupdate) {
            printf("end update and save map !\n");fflush(stdout);
            NAVI_Setlimitdis(25);
            bool updatemode = NAVI_Getupdatemode();
            bool expandmode = NAVI_Getexpandmode();
            bool manualmode = NAVI_Getmanualmode();
            if (updatemode || expandmode || manualmode) {
                if (manualmode) {
                    if (strlen(strfileName0) != 0) {
                        NAVI_UpdateProbMap(strfileName0);
                        printf("manualupdate:%s\n", strfileName0);fflush(stdout);
                    } else {
                        NAVI_clearupdateg();
                    }
                } else if (updatemode) {
                    if (strlen(nowusestrfileName) != 0) {
                        NAVI_UpdateProbMap(nowusestrfileName);
                        printf("autoupdate:%s\n", nowusestrfileName);fflush(stdout);
                    } else {
                        NAVI_clearupdateg();
                    }
                }
                bool mode = false;
                NAVI_Setupdatemode(mode);
                NAVI_Setexpandmode(mode);
                NAVI_Setmanualupdate(mode);
                UpdateMapCallBackFunc();
                printf("save update map !\n");fflush(stdout);
            }
            if (1 == ucLoadmapflag) {
                NAVI_SaveModifyMap();
            }
            NAVI_CloseVisionDate();
        }
    } break;
    case 49: //地图更新
    {
        int iupdate = robotctrldata->iparams[0];
        if (1 == iupdate) {
            printf("begin manualupdate !\n");fflush(stdout);
            bool mode = false;
            NAVI_Setlimitdis(15);
            NAVI_Setupdatemode(mode);
            NAVI_Setexpandmode(mode);

            NAVI_clearupdateg();

            NAVI_Setmanualupdate(true);
            UpdateMapBeginCallBackFunc();
        }
        if (0 == iupdate) ///****
        {
            printf("exit without saving map !\n");fflush(stdout);

            bool mode = false;
            NAVI_Setupdatemode(mode);
            NAVI_Setexpandmode(mode);
            NAVI_Setmanualupdate(mode);

            NAVI_clearupdateg();
        }
    } break;

    case 55: //似乎没用到
    {
        vector<double> vw;
        vw.push_back(robotctrldata->dparams[0]);
        vw.push_back(robotctrldata->dparams[1]);
        NAVI_SetVW(vw);
        break;
    }
    case 60: //用不到
    {
        if (1 == robotctrldata->nbparams) {
            NAVI_SetSpeedLevle(robotctrldata->bparams[0]);
        }
        break;
    }
    case 80: //发送地图，用不到
        printf("SendMap\n");fflush(stdout);
        if (80 != robotctrldata->commandid) {
            break;
        }
        strcpy(g_strfileName, MAP_PATH_NAME1);
        strcat(g_strfileName, robotctrldata->sparams[0]);

        int status;
        pthread_t thrdSendMap;
        status = pthread_create(&thrdSendMap, NULL, SendMapThreadProc,
                                (LPVOID)g_strfileName);
        break;
    case 81: //保存更新的地图
        NAVI_SaveModifyMap();
        break;
    case 127:  // 全息路径规划 - AnXin
    /*
     * AnXin：2025-2-9
     * 这里需要注意，在创造新的命令 ID 时，ID 应位于 [-128, 127] 范围内，否则会出现错误！
     * 具体原因可查看 robot_control_t 中关于 commandid 的类型定义。
     * 本人将全息路径规划的命令 ID 初始设置为 1024，系统不会报错，研究了整整一下午加一晚上，终于找到了问题所在 T~T。
     */
    {
        NAVI_SetSearchType(searchType);
        printf("Receive AnXin Path\n");fflush(stdout);
        int algNum = robotctrldata->iparams[0];
        cout << "robotctrldata->iparams[0] = " << robotctrldata->iparams[0] << endl;
        cout << "algNum = " << algNum << endl;
        NAVI_SetPlanFullPath(algNum);
        break;
    }
    case 125:
    {
        printf("Select room vertices ...\n");fflush(stdout);
        cout << robotctrldata->iparams[0] << ", " << robotctrldata->dparams[0] << ", " << robotctrldata->dparams[1] << endl;
        NAVI_SetRoomVertex(
            robotctrldata->iparams[0],
            robotctrldata->dparams[0],
            robotctrldata->dparams[1]
        );
        break;
    }
    case 126:   // 取消全息路径规划 - AnXin
    {
        printf("Cancel AnXin Path\n");fflush(stdout);
        NAVI_CancelPlanFullPath();
        break;
    }
    case 124:   // 从属机拉取主机地图 - AnXin
    {
        printf("Main - Getting Map!\n");fflush(stdout);
        int ip[4] = { 0 };
        ip[0] = robotctrldata->iparams[0];
        ip[1] = robotctrldata->iparams[1];
        ip[2] = robotctrldata->iparams[2];
        ip[3] = robotctrldata->iparams[3];
        NAVI_SubGetMapFromMain(ip);
        break;
    }

    case 123: // 读取路径文件进行指定主机/子机进行全路径覆盖
    {
        int robotId = robotctrldata->iparams[0]; // 0 主机器人，1 副机器人
        
        //NAVI_NavigatePathByGridPoints(subPath);
        NAVI_SetPlanFullPath(2);
        NAVI_SetrobotId(robotId);
        break;
    }
    case 122://读取对角坐标后生成路径文件
    {
        Pose A, B;
        A.x = robotctrldata->dparams[3];
        A.y = robotctrldata->dparams[4];
        B.x = robotctrldata->dparams[5];
        B.y = robotctrldata->dparams[6];
        cout << "-------------------Receive A and B----------------------"  << endl;
        cout << "A.x = " << A.x << ", A.y = " << A.y << endl;
        cout << "B.x = " << B.x << ", B.y = " << B.y << endl;
        NAVI_SetRoomVertex(0,A.x, A.y);
        NAVI_SetRoomVertex(1,B.x, B.y);
        NAVI_SetRoomVertex(2,robotctrldata->dparams[0], robotctrldata->dparams[1]);
        /*IPoint tempPose;
        tempPose = NAVI_GlobalToGrid(A.x, A.y);
        int x1 = tempPose.x;
        int y1 = tempPose.y;
        tempPose = NAVI_GlobalToGrid(B.x, B.y);
        int x2 = tempPose.x;
        int y2 = tempPose.y;

        IPoint curPose;
        curPose = NAVI_GlobalToGrid(robotctrldata->dparams[0], robotctrldata->dparams[1]);
        int rob[2] = { curPose.x, curPose.y };
        int safesize = 7; 
        int minsize = 5; 
        NAVI_CreateFullPath(x1, y1, x2, y2, rob, safesize, minsize);
        printf("Create full path from (%d, %d) to (%d, %d) with robot at (%d, %d), successfully\n",
               x1, y1, x2, y2, rob[0], rob[1]);fflush(stdout);*/
    }
    default:
        break;
    }
}

void RevisePoseHandle(const lcm_recv_buf_t *rbuf, const char *channel,
                      const pose_t *revise_pose, void *user) {

    Pose pos;
    int flag = (int)revise_pose->vel[0];
    pos.x = revise_pose->pos[0];
    pos.y = revise_pose->pos[1];
    pos.theta = revise_pose->pos[2];

    NAVI_RevisePose(pos, flag);
}

int LCMInit(void) {
    lcm = lcm_create( NULL );
    // TODO 测试跨机
    // lcm = lcm_create("udpm://239.255.76.67:7667?ttl=1");
    /////////////////
    if (!lcm) {
        printf("lcm_create failed\n");fflush(stdout);
        return 1;
    }

    laser_t_subscribe(lcm, "HOKUYO_LIDAR", &LaserDataHandle, NULL);
    pose_t_subscribe(lcm, "POSE", &PoseDataHandle, NULL);
    robot_control_t_subscribe(lcm, "ROBOT_CONTROL", &RobotCtrlHandle, NULL);
    // pose_t_subscribe	      (lcm, "REVISE_POSE",
    // &RevisePoseHandle, NULL); path_t_subscribe         (lcm,"VIRTUAL_WALL",
    // &ReviseWallHandle, NULL);
    return 0;
}

void GetPosCallBackFunc(Pose *pos, double flag, vector<double> rate_flag) {
    long systemtime = (long)time((time_t *)0);

    pose_t posedata;

    posedata.utime = systemtime;
    posedata.pos[0] = pos->x;     // x
    posedata.pos[1] = pos->y;     // y
    posedata.pos[2] = pos->theta; //角度

    posedata.vel[0] = flag;
    // TODO
    posedata.vel[1] = 0;
    posedata.vel[2] = 0;
    /////////////////////

    posedata.orientation[0] = rate_flag[0];
    posedata.orientation[1] = rate_flag[1];
    posedata.orientation[2] = rate_flag[2];
    // TODO
    posedata.orientation[3] = 0;

    posedata.rotation_rate[0] = 0;
    posedata.rotation_rate[1] = 0;
    posedata.rotation_rate[2] = 0;

    posedata.accel[0] = 0;
    posedata.accel[1] = 0;
    posedata.accel[2] = 0;

    ////////////////////
    pose_t_publish(lcm, "CURRENTPOSE", &posedata);
}


void HaveNoPathCallBackFunc(int &a) {
    long systemtime = (long)time((time_t *)0);
    robot_control_t cmd;
    double dend = 0;
    unsigned char bend = a;
    int i = 0;

    cmd.utime = systemtime;
    cmd.commandid = 25;
    cmd.ndparams = 1;
    cmd.dparams = &dend;
    cmd.nsparams = 0;
    cmd.niparams = 1;
    cmd.iparams = (signed char *)&i;
    cmd.nbparams = 1;
    cmd.bparams = &bend;

    robot_control_t_publish(lcm, "ROBOT_CONTROL", &cmd);
}

bool UpdateMapCallBackFunc(void) {
    robot_control_t s_cmd;
    unsigned char bend = 1;
    s_cmd.commandid = 105;
    s_cmd.ndparams = 0;
    s_cmd.nsparams = 0;
    s_cmd.niparams = 0;
    s_cmd.nbparams = 1;
    s_cmd.bparams = &bend;

    robot_control_t_publish(lcm, "ROBOT_CONTROL", &s_cmd);
    printf("LCM send update map\n");fflush(stdout);
    return true;
}
bool UpdateMapBeginCallBackFunc(void) {
    robot_control_t s_cmd;
    unsigned char bend = 1;
    s_cmd.commandid = 107;
    s_cmd.ndparams = 0;
    s_cmd.nsparams = 0;
    s_cmd.niparams = 0;
    s_cmd.nbparams = 1;
    s_cmd.bparams = &bend;

    robot_control_t_publish(lcm, "ROBOT_CONTROL", &s_cmd);
    printf("LCM send update map\n");fflush(stdout);
    return true;
}

void CmdCallBackFunc(int tmpcmd) {
    robot_control_t cmd;
    double dend = 0;
    unsigned char bend = 0;
    int i = 0;
    long systemtime = (long)time((time_t *)0);
    cmd.utime = systemtime;
    cmd.commandid = 26;
    cmd.ndparams = 1;
    cmd.dparams = &dend;
    cmd.nsparams = 0;
    cmd.niparams = 1;
    cmd.iparams = (signed char *)&i;
    cmd.nbparams = 1;
    cmd.bparams = &bend;

    robot_control_t_publish(lcm, "ROBOT_CONTROL", &cmd);
}

#if 1

void LogInfoCallBackFunc(int loginfo) {
    robot_control_t cmd;
    static int lastloginfo = 0;
    if (lastloginfo != loginfo) {
        unsigned char bend = 0;
        bend = loginfo;
        long systemtime = (long)time((time_t *)0);
        cmd.utime = systemtime;
        cmd.commandid = 11;
        cmd.ndparams = 0;
        cmd.nsparams = 0;
        cmd.niparams = 0;
        cmd.nbparams = 1;
        cmd.bparams = &bend;

        robot_control_t_publish(lcm, "ROBOT_LOG", &cmd);

        lastloginfo = loginfo;
    }
}
#endif

void deleteMapFile(const char *path) {
    if (remove(path) == -1) {
        printf("delete map %s failed\n", path);fflush(stdout);
    } else {
        printf("mapfile %s deleted!\n", path);fflush(stdout);
    }
}



int main(void) {
    // 判断当前有效用户id是否为0
    if (getuid() != 0) {
        printf("Please run as root\n");fflush(stdout);
        return 1;
    }
    deleteMapFile("/data/test/defultMap.txt");
    deleteMapFile("/data/test/defultMap.txt.txt");
    deleteMapFile("/data/test/unprobdefultMap.txt");
    
    printf("version Debug V1.2.1.9 system begin\n");fflush(stdout);
    /* lcm 数据接收线程 */
    pthread_t lcmrecv_thread;
    pthread_t udprecv_thread;
    int status;
    pthread_attr_t thread_attribute;
    struct sched_param sched;

    /* ...................udp.................... 7/2
     * add........................... */
    // UdpInit();
    // printf("udp init finished\n");
    /*................................................................................*/

    NAVI_RegisterGetPosCallback(PGetPosCallBackFunc(&GetPosCallBackFunc));
    NAVI_ReginsterGetPathCallback(PGetPathCallBackFunc(&GetPathCallBackFunc));
    NAVI_RegisterNoPathCallback(
        PHaveNoPathCallBackFunc(&HaveNoPathCallBackFunc));
    NAVI_RegisterCmdCallback(PCmdCallBackFunc(&CmdCallBackFunc));

    NAVI_RegisterLogInfoCallback(PLogInfoCallBackFunc(&LogInfoCallBackFunc));

    printf("call back set!\n");fflush(stdout);
    /* ...................udp.................... 7/2
     * add........................... */

    status = pthread_attr_init(&thread_attribute);
    if (status != 0) {
        fprintf(stderr,
                "Thread Attribute init failed.  Terminating with an error\n");
        exit(EXIT_FAILURE);
    }

    status = pthread_attr_setschedpolicy(&thread_attribute, SCHED_RR);
    if (status != 0) {
        fprintf(stderr,
                "Thread Schedule Policy failed.  Terminating with an error\n");
        exit(EXIT_FAILURE);
    }
    printf("Valid priority range for SCHED_RR: %d~%d\n",
           sched_get_priority_min(SCHED_RR), sched_get_priority_max(SCHED_RR));fflush(stdout);
    struct sched_param param;
    pthread_attr_getschedparam(&thread_attribute, &param);

    param.sched_priority = 5;

    pthread_attr_setschedparam(&thread_attribute, &param);

    status = pthread_create(&udprecv_thread, &thread_attribute,
                            UDP_Recv_LoopFunc, NULL);

    if (status != 0) {
        fprintf(stderr, "udprecv_thread with an error\n");
        exit(EXIT_FAILURE);
    }

    // pthread_join(udprecv_thread,NULL);

    /*................................................................................*/
    printf("let's init lcm...\n");fflush(stdout);
    if (LCMInit()) {
        printf("System Initialize Failed\n");fflush(stdout);

        return 0;
    }

    status = pthread_attr_init(&thread_attribute);
    if (status != 0) {
        fprintf(stderr,
                "Thread Attribute init failed.  Terminating with an error\n");
        exit(EXIT_FAILURE);
    }

    status = pthread_attr_setschedpolicy(&thread_attribute, SCHED_RR);
    if (status != 0) {
        fprintf(stderr,
                "Thread Schedule Policy failed.  Terminating with an error\n");
        exit(EXIT_FAILURE);
    }

    pthread_attr_getschedparam(&thread_attribute, &param);

    param.sched_priority = 10;

    pthread_attr_setschedparam(&thread_attribute, &param);

    status =
        pthread_create(&lcmrecv_thread, &thread_attribute, LCMRecvTask, NULL);

    if (status != 0) {
        fprintf(stderr, "Initial thread terminating with an error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {

        long systemtime = (long)time((time_t *)0);
        robot_control_t cmd;
        double dend = 0;
        unsigned char bend = 0;
        int i = 0;

        cmd.utime = systemtime;
        cmd.commandid = 99;
        cmd.ndparams = 1;
        cmd.dparams = &dend;
        cmd.nsparams = 0;
        cmd.niparams = 1;
        cmd.iparams = (signed char *)&i;
        cmd.nbparams = 1;
        cmd.bparams = &bend;

        robot_control_t_publish(lcm, "ALIVE", &cmd);
        robot_control_t_publish(lcm, "ROBOT_LOG", &cmd);

        sleep(1);
    }
}
