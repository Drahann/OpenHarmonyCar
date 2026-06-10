#include "NaviInterface.h"
#include "Astarplanner.h"
#include <cerrno>
#include <cstdio>

extern lcm_t *lcm;
extern lcm_t *coop_lcm;

int g_count = 0;
int printnopathflag = 0;
int nopathflag = 0;
int nopathstatus = 0;
int iLimitDis = 25;
bool visionswitch = true;
int iVisionDateFlag = 1;
int g_iSaveDateFlag = 0;

static const double kCoopMinSafeRadius = 0.35;
static const double kCoopRobotMargin = 0.20;
static const double kCoopLaserEvidenceMargin = 0.30;
static const long kCoopMessageTimeoutMs = 2500;

static long coopNowMs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static double coopDist2(double ax, double ay, double bx, double by)
{
    double dx = ax - bx;
    double dy = ay - by;
    return dx * dx + dy * dy;
}

static double coopPointToSegmentDistance(Pose p, Pose a, Pose b)
{
    double vx = b.x - a.x;
    double vy = b.y - a.y;
    double len2 = vx * vx + vy * vy;
    if (len2 < 1e-9) {
        return sqrt(coopDist2(p.x, p.y, a.x, a.y));
    }
    double t = ((p.x - a.x) * vx + (p.y - a.y) * vy) / len2;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    Pose q;
    q.x = a.x + t * vx;
    q.y = a.y + t * vy;
    q.theta = 0;
    return sqrt(coopDist2(p.x, p.y, q.x, q.y));
}

static double coopSegmentDistance(Pose a1, Pose a2, Pose b1, Pose b2)
{
    double d1 = coopPointToSegmentDistance(a1, b1, b2);
    double d2 = coopPointToSegmentDistance(a2, b1, b2);
    double d3 = coopPointToSegmentDistance(b1, a1, a2);
    double d4 = coopPointToSegmentDistance(b2, a1, a2);
    double d = d1 < d2 ? d1 : d2;
    d = d < d3 ? d : d3;
    return d < d4 ? d : d4;
}

static bool defaultPublishedMapExists()
{
    ifstream mapFile("/data/test/defultMap.txt", ios::in);
    return mapFile.good();
}

static bool fileHasContent(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return false;
    }
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fclose(fp);
    return fileSize > 0;
}

static bool replaceFileAtomic(const char *tmpName, const char *finalName)
{
    if (rename(tmpName, finalName) == 0) {
        return true;
    }

    int firstErr = errno;
    remove(finalName);
    if (rename(tmpName, finalName) == 0) {
        return true;
    }

    printf("ERR: replace file failed, tmp=%s final=%s errno=%d first_errno=%d\n",
           tmpName, finalName, errno, firstErr);
    fflush(stdout);
    remove(tmpName);
    return false;
}

static bool closeAndReplaceTextFile(ofstream &outFile,
                                    const char *tmpName,
                                    const char *finalName)
{
    outFile.flush();
    bool ok = outFile.good();
    outFile.close();
    if (!ok) {
        printf("ERR: write file failed, tmp=%s\n", tmpName);
        fflush(stdout);
        remove(tmpName);
        return false;
    }
    return replaceFileAtomic(tmpName, finalName);
}

static void removeGeneratedFileIfExists(const char *path)
{
    if (remove(path) == 0) {
        printf("remove old generated file: %s\n", path);
        fflush(stdout);
        return;
    }

    if (errno != ENOENT) {
        printf("WARN: remove old generated file failed, path=%s errno=%d\n",
               path, errno);
        fflush(stdout);
    }
}

static void clearGeneratedMapFilesForNewMapping()
{
    const char *files[] = {
        "/data/test/defultMap.txt",
        "/data/test/defultMap.txt.tmp",
        "/data/test/defultMap.txt.bak",
        "/data/test/defultMap.txt.txt",
        "/data/test/defultMap.txt.txt.tmp",
        "/data/test/defultMap.txt.txt.bak",
        "/data/test/unprobdefultMap.txt",
        "/data/test/unprobdefultMap.txt.tmp",
        "/data/test/unprobdefultMap.txt.bak",
        "/data/test/roadFile.txt",
        "/data/test/roadFile.txt.tmp",
        "/data/test/tmpcoverageMap.txt",
        "/data/test/initCoverageMap.txt",
        "/data/test/midMap.txt",
        "/data/test/coverageMap.txt",
        "/data/test/pathcheck.txt",
        "/data/test/pathplanmap.txt",
        "/data/test/testmap_astar.txt",
        "/data/test/testmap_gauss.txt",
        "/data/test/testmap_planmap.txt",
        "/data/test/vision.txt",
        "/data/test/optmap.txt",
        "/data/test/updateCopymapB.txt",
        "/data/test/updateCopymapC.txt",
        "/data/test/cleanmap.txt",
        "/data/test/haha2.txt"
    };

    for (int i = 0; i < (int)(sizeof(files) / sizeof(files[0])); ++i) {
        removeGeneratedFileIfExists(files[i]);
    }
}

static const char *fullPathErrorName(int errorCode)
{
    switch (errorCode) {
    case FULLPATH_ROOM_INIT_FAILED:
        return "FULLPATH_ROOM_INIT_FAILED";
    case FULLPATH_ROAD_FILE_INVALID:
        return "FULLPATH_ROAD_FILE_INVALID";
    case TARGET_STATIC_INVALID:
        return "TARGET_STATIC_INVALID";
    case TARGET_DYNAMIC_BLOCKED_LASER:
        return "TARGET_DYNAMIC_BLOCKED_LASER";
    case TARGET_DYNAMIC_BLOCKED_VISION:
        return "TARGET_DYNAMIC_BLOCKED_VISION";
    case ASTAR_NO_PATH:
        return "ASTAR_NO_PATH";
    default:
        return "FULLPATH_OK";
    }
}

OptimizeMap::OptimizeMap(void) {
    OpScanMatcher = new ScanMatcher;
    OpMapServer = new MapServer;
    OpMatcher = new MultiResolutionScanMatcher;
}

OptimizeMap::~OptimizeMap(void) {}

Scanlinkmatch::Scanlinkmatch(void) {
    linkScanMatcher = new ScanMatcher;
    linkMatcher = new MultiResolutionScanMatcher;
}

Scanlinkmatch::~Scanlinkmatch(void) {}

CNaviInterface::CNaviInterface(void) {

    m_bIsLoc = false;
    m_bConverged = false;
    m_bRunning = true;
    m_bPoseError = false;
    m_bRevise = false;
    m_bfind = false;
    m_bfind2rc = false;
    m_bodmswitch = false;
    m_ifmodifymap = false;
    m_laser1only = false;
    bpfsetpose = false;
    m_pOptimizeMap = NULL;
    m_pscanlinkmatch = NULL;
    m_pscanMatcher = NULL;
    m_pms = NULL;
    m_pmatcher = NULL;
    m_pmsSLAM = NULL;
    /***********/
    pscanMatcher = NULL;
    m_pmsSLAMtest = NULL;
    /*************/
    drawvision = false;
    backforward = false;
    backforwardnum = 0;
    m_pPf = NULL;
    m_bexpandmap = false;
    m_bupdatemap = false;
    m_bifrecenter = false;
    m_bmanualupdate = false;
    m_blaserdwa = false;
    m_blaserastar = false;
    enableCoverage = false;
    targetErr = false;
    searchType = 1;
    m_mapState = MAP_STATE_IDLE;
    m_lastFullPathError = FULLPATH_OK;
    m_eNaviType = LOCALIZATION;
    m_eLocationType = SCANMATCH;
    m_CurPos.x = 0;
    m_CurPos.y = 0;
    m_CurPos.theta = 0;

    chargemappose.x = 0;
    chargemappose.y = 0;
    chargemappose.theta = 0;

    lastchargemappose.x = 0;
    lastchargemappose.y = 0;
    lastchargemappose.theta = 0;

    m_bautocharge = false;
    m_bcreateautochargemap = false;

    search_x_m = 0.2;
    search_y_m = 0.2;
    search_theta_rad = degrees_to_radians(15.0);

    S2B[0][0] = 1.0;
    S2B[0][1] = 0.0;
    S2B[0][2] = 0.0;
    S2B[0][3] = 0.15;
    S2B[1][0] = 0.0;
    S2B[1][1] = 1.0;
    S2B[1][2] = 0.0;
    S2B[1][3] = 0.0;
    S2B[2][0] = 0.0;
    S2B[2][1] = 0.0;
    S2B[2][2] = 1.0;
    S2B[2][3] = 0.584;
    S2B[3][0] = 0.0;
    S2B[3][1] = 0.0;
    S2B[3][2] = 0.0;
    S2B[3][3] = 1.0;

    max_range = 29.5;

    m_pPosCbFunc = NULL;
    m_pPathCbFunc = NULL;
    m_pLogCbFunc = NULL;
    pfright = 0;
    /******/
    m_imode = 0; //初始化时为0，对refineFunc无限制，在底盘定位成功后，会置位成1
                 /*****/
    m_ifirstinitloc = 0;
    dvibration = 0;
    ifirstpublish = 0;
    m_iplanend = 1;

    m_ijumpnum = 0;
    m_ilowscorenum = 0;
    m_ucspeedlever = 3;
    autochargemapcount = 0;

    pthread_mutex_init(&m_csPose_mutex, NULL);
    pthread_mutex_init(&m_csLoc_mutex, NULL);
    pthread_mutex_init(&m_csLaser_mutex, NULL);
    pthread_mutex_init(&m_goal_mutex, NULL);
    pthread_mutex_init(&m_csPlan_mutex, NULL);
    pthread_mutex_init(&m_pf_mutex, NULL);
    pthread_mutex_init(&m_initloc_mutex, NULL);

    pthread_mutex_init(&m_mutex, NULL);

    pthread_mutex_init(&m_vw_mutex, NULL);

    pthread_cond_init(&m_condPathPlan, NULL);

    m_config.pathplannerconfig.maxCost = 254;
    m_config.pathplannerconfig.minCellCost = 40;
    m_config.pathplannerconfig.troughWidth = 1;
    m_config.pathplannerconfig.upsample = 1;

    m_config.robotconfig.radius = 0.6;
    m_config.robotconfig.width = 0.6;

    m_config.ScanMatchconfig.xScanMatchRange = 0.2;
    m_config.ScanMatchconfig.yScanMatchRange = 0.2;
    m_config.ScanMatchconfig.thetaScanMatchRange = 15.0;

    robotId = 0;
    pthread_mutex_init(&m_coop_mutex, NULL);
    m_coopState = COOP_AVOID_NORMAL;
    m_coopSeq = 0;
    m_coopActiveSeq = 0;
    m_coopActivePeer = -1;
    m_coopTriggerReason = 0;
    m_coopStateTimeMs = 0;
    m_coopPendingStopRequest = false;
    m_coopPendingStopSource = -1;
    m_coopPendingStopSeq = 0;
    m_coopSavedGoalValid = false;
    m_coopSavedGoal.x = 0;
    m_coopSavedGoal.y = 0;
    m_coopSavedGoal.theta = 0;
    m_coopSavedCoverageIndex = -1;
    m_coverageTurnIndex = -1;
    m_coverageActive = false;
    m_coopPeerPoseValid = false;
    m_coopPeerHasGoal = false;
    m_coopPeerPose.x = 0;
    m_coopPeerPose.y = 0;
    m_coopPeerPose.theta = 0;
    m_coopPeerGoal.x = 0;
    m_coopPeerGoal.y = 0;
    m_coopPeerGoal.theta = 0;

    int status;
    /************************************************************************************/
    status = pthread_attr_init(&m_thread_attribute);
    if (status != 0) {
        fprintf(stderr,
                "Thread Attribute init failed.  Terminating with an error\n");
        // exit(EXIT_FAILURE);
    }

    status = pthread_attr_setschedpolicy(&m_thread_attribute, SCHED_RR);
    if (status != 0) {
        fprintf(stderr,
                "Thread Schedule Policy failed.  Terminating with an error\n");
        // exit(EXIT_FAILURE);
    }
    struct sched_param param;
    pthread_attr_getschedparam(&m_thread_attribute, &param);

    param.sched_priority = 20;

    pthread_attr_setschedparam(&m_thread_attribute, &param);
    /************************************************************************************/
    status =
        pthread_create(&m_thrdPathPlan, NULL, PlanThreadProc, (LPVOID)this);

    if (status != 0) {
        fprintf(stderr, "Initial thread terminating with an error\n");
        //  exit(EXIT_FAILURE);
    }

    /************************************************************************************/
    status = pthread_create(&m_thrdFullPathCoveragePlan, NULL, CoverageThreadProc, (LPVOID)this);

    if (status != 0) {
        fprintf(stderr, "Initial thread terminating with an error\n");
        // exit(EXIT_FAILURE);
    }

    m_bslam = false;

    revise_Range_x = 1.0;
    revise_Range_y = 1.0;
    revise_Range_theta = PI / 3;

    m_pf_count = 0;

    ordi = 0;
    ordi_1 = 0;
    m_ifastar = 1;
    vw_0 = 0;
    rrset = 0;
    curastar = 0;
    istartgo = 0;
    m_ifpf = 1;
    m_VisionWallData.clear();
    m_CollisionWallData.clear();

}

CNaviInterface::~CNaviInterface(void) {

    pthread_mutex_destroy(&m_csPose_mutex);
    pthread_mutex_destroy(&m_csLoc_mutex);
    pthread_mutex_destroy(&m_csPlan_mutex);
    pthread_mutex_destroy(&m_csLaser_mutex);
    pthread_mutex_destroy(&m_goal_mutex);
    pthread_mutex_destroy(&m_pf_mutex);
    pthread_mutex_destroy(&m_vw_mutex);
    pthread_mutex_destroy(&m_initloc_mutex);
    pthread_mutex_destroy(&m_coop_mutex);
}

void CNaviInterface::setMapLifecycleState(MapLifecycleState state) {
    m_mapState = state;
    printf("map lifecycle state = %d\n", (int)state);
    fflush(stdout);
}

MapLifecycleState CNaviInterface::getMapLifecycleState(void) {
    return m_mapState;
}

void CNaviInterface::setLastFullPathError(int errorCode) {
    m_lastFullPathError = errorCode;
    if (errorCode != FULLPATH_OK) {
        printf("fullpath_error=%s(%d)\n", fullPathErrorName(errorCode), errorCode);
        fflush(stdout);
    }
}

void CNaviInterface::setConfig(RobotConfig &config) {
    S2B[0][3] = config.laserconfig.positionX;
    S2B[1][3] = config.laserconfig.positionY;
    S2B[2][3] = config.laserconfig.positionZ;

    max_range = config.laserconfig.max_range;

    m_config.robotconfig.radius = config.robotconfig.radius;
    m_config.robotconfig.width = config.robotconfig.width;

    if (config.ScanMatchconfig.xScanMatchRange > 0.09 &&
        config.ScanMatchconfig.xScanMatchRange < 0.31) {
        m_config.ScanMatchconfig.xScanMatchRange =
            config.ScanMatchconfig.xScanMatchRange;
        search_x_m = m_config.ScanMatchconfig.xScanMatchRange;
    }
    if (config.ScanMatchconfig.yScanMatchRange > 0.09 &&
        config.ScanMatchconfig.yScanMatchRange < 0.31) {
        m_config.ScanMatchconfig.yScanMatchRange =
            config.ScanMatchconfig.yScanMatchRange;
        search_y_m = m_config.ScanMatchconfig.yScanMatchRange;
    }

    if (config.ScanMatchconfig.thetaScanMatchRange > 14.0 &&
        config.ScanMatchconfig.thetaScanMatchRange < 31.1) {

        m_config.ScanMatchconfig.thetaScanMatchRange =
            config.ScanMatchconfig.thetaScanMatchRange;
        // search_theta_rad =
        // degrees_to_radians(m_config.ScanMatchconfig.thetaScanMatchRange);
        search_theta_rad = degrees_to_radians(15);
    }

    printf("set config %f, %f, %f ,max_range = %f , radius = %f, width = %f "
           ",search_x_m=%f,search_y_m = %f,search_theta=%f\n",
           S2B[0][3], S2B[1][3], S2B[2][3], max_range,
           m_config.robotconfig.radius, m_config.robotconfig.width, search_x_m,
           search_y_m, search_theta_rad);fflush(stdout);
}

void *CNaviInterface::PlanThreadProc(LPVOID pPara) {

    CNaviInterface *pObject = (CNaviInterface *)pPara;
    while (pObject->m_bRunning) {
        // DWORD ret = WaitForSingleObject(pObject->m_hEvent, INFINITE);
        double omitRange = 0.35;
        double maxRange = pObject->max_range;
        vector<vector<double>> points;
        vector<vector<double>> forelaser;
        vector<vector<double>> limitlaser;
        laser_st laserData;
        timeval begintime;
        timeval endtime;
        int time;
        int waypoint = 0;

        pthread_mutex_lock(&(pObject->m_mutex));
        pthread_cond_wait(&(pObject->m_condPathPlan), &(pObject->m_mutex));

        gettimeofday(&begintime, NULL);

        pObject->updateCoopAvoidance();
        if (pObject->isStoppedForCoopPeer()) {
            pObject->publishZeroVelocity();
            pthread_mutex_unlock(&(pObject->m_mutex));
            continue;
        }

        pthread_mutex_lock(&(pObject->m_csLaser_mutex));

        // m_stlaserdata是激光数据，omitRange是激光的最小有效距离,
        // maxRange是激光采集的最大有效距离，points是转换后的激光直角坐标系下的坐标
        pObject->laser2Points(pObject->m_stlaserdata, omitRange, maxRange, NULL,
                              0, points, forelaser, limitlaser, iLimitDis);

        // time1 = firsttime;

        pthread_mutex_unlock(&(pObject->m_csLaser_mutex));

        if (points.size() == 0) {
            pthread_mutex_unlock(&(pObject->m_mutex));
            continue;
        }

        vector<Pose> bodyPoints;
        vector<Pose> forelaserPoints;
        vector<Pose> limitlaserPoints;
        //按照变换矩阵，把激光直角坐标系下的坐标转换为机器人坐标系下的坐标，因为激光器与机器人底盘中心有距离
        LinAlg::transform(pObject->S2B, points, bodyPoints); // points;
        LinAlg::transform(pObject->S2B, forelaser, forelaserPoints);
        LinAlg::transform(pObject->S2B, limitlaser, limitlaserPoints);
        pthread_mutex_lock(&(pObject->m_csLoc_mutex));

        pObject->update(bodyPoints, forelaserPoints, limitlaserPoints);
        pthread_mutex_unlock(&(pObject->m_csLoc_mutex));

        pthread_mutex_lock(&(pObject->m_goal_mutex));
        waypoint = pObject->m_waypoints.size();
        if (waypoint > 0) {
            pObject->m_iplanend = 0; //! must delete visionmap after planpath
                                     //! end ,check the deletegoal function
        }
        pthread_mutex_unlock(&(pObject->m_goal_mutex));

        if (waypoint > 0) {
            GridMap visMap;
            pthread_mutex_lock(&(pObject->m_csPlan_mutex));
            int visionNum = pObject->m_VisionWallData.size();
            if (visionNum > 100) {
                printf("attention please : obstacles ahead ! (By camera, num = "
                       "%d)\n",
                       visionNum);fflush(stdout);
            }
            if (100 < visionNum) {
                pObject->m_VisionWallDataCopy.clear();
                int num = pObject->m_VisionWallData.size();
                for (int i = 0; i < num; i++) {
                    pObject->m_VisionWallDataCopy.push_back(
                        pObject->m_VisionWallData.at(i));
                }
                pObject->m_VisionWallData.clear();
            }
            pObject->m_CollisionWallDataCopy.clear();
            int cnum = pObject->m_CollisionWallData.size();
            for (int i = 0; i < cnum; i++) {
                pObject->m_CollisionWallDataCopy.push_back(
                    pObject->m_CollisionWallData.at(i));
            }
            pObject->m_CollisionWallData.clear();

            // pObject->m_pms->returnLaserMap(pObject->m_CurPos,bodyPoints,pObject->m_VisionWallDataCopy);
            pObject->m_pms->returnLaserMap(pObject->m_CurPosForPath, bodyPoints,
                                           pObject->m_VisionWallDataCopy);
            pthread_mutex_unlock(&(pObject->m_csPlan_mutex));
            switch (pObject->searchType)
            {
                case 1:
                    pObject->planPath(pObject->m_pms->astarMap,
                              pObject->m_CurPosForPath, 1, 1);
                    break;
                case 2:
                    pObject->planPath(pObject->m_pms->astarMap,
                              pObject->m_CurPosForPath, 2, 1);
                    break;
                case 3:
                    pObject->planPath(pObject->m_pms->astarMap,
                              pObject->m_CurPosForPath, 3, 1);
                    break;
                default:
                    printf("Invalid choice! Default choice : 1\n");fflush(stdout);
                    pObject->planPath(pObject->m_pms->astarMap,
                              pObject->m_CurPosForPath, 1, 1);
                    break;
            }
        }
        if (true == pObject->saveMapCallBack()) {
            pObject->SaveMapCallBack();
            pObject->SetSaveMapDone(0);
        }

        gettimeofday(&endtime, NULL);
        time = (endtime.tv_sec - begintime.tv_sec) * 1000 +
               (double)(endtime.tv_usec - begintime.tv_usec) / 1000;
        if (time < 300)

            usleep((300 - time) * 1000);

        pthread_mutex_unlock(&(pObject->m_mutex));
    }

    return NULL;
}

//激光数据转化为激光坐标系下的坐标，omitRange是激光数据采集的最小距离，maxRange是激光数据采集的最大距离，mask_out_rad为NULL，masklength为0
void CNaviInterface::laser2Points(laser_st &ldata, double omitRange,
                                  double maxRange, double *mask_out_rad,
                                  int masklength,
                                  vector<vector<double>> &points,
                                  vector<vector<double>> &forelaser,
                                  vector<vector<double>> &limitlaser,
                                  int limitdis) {

    if (mask_out_rad != NULL) {
        for (int i = 0; i < masklength; i++)
            mask_out_rad[i] = MathUtil::toRadians(mask_out_rad[i]);
    }

    for (int i = 0; i < ldata.nranges; i++) {
        double theta = (-1) * ldata.intensities[i] * PI / 180;
        double r = ldata.ranges[i] / 1000;
        // TODO
        //  printf("r = %f,theta = %f\n",r,theta);
        //////////////////////

        if (r > maxRange || r < omitRange)
            continue;

        if (mask_out_rad != NULL) {
            bool mask = false;

            for (int j = 0; j < masklength; j += 2) {

                if (mask_out_rad[j] <= theta && theta <= mask_out_rad[j + 1])
                    mask = true;
            }

            if (mask)
                continue;
        }

        double d1 = r * cos(theta);
        double d2 = r * sin(theta);

        vector<double> p;
        p.push_back(d1);
        p.push_back(d2);

        points.push_back(p);
        forelaser.push_back(p);
        if (limitdis > r) {
            limitlaser.push_back(p);
        }
    }
}
/***********************************************************************************************/

void CNaviInterface::RevisePose(Pose &posRevise, int flag) //纠偏
{
    // receive pose error
    if (flag == 2) {
        m_bPoseError = false;
        return;
    }
    if (flag == 3) {
        if (m_pscanMatcher != NULL)
            m_pscanMatcher->SetEncoderXyt(posRevise); //码盘点赋给当前位姿
    }

    pthread_mutex_lock(&m_csPose_mutex);

    m_CurPos.x = posRevise.x; //码盘赋值
    m_CurPos.y = posRevise.y;
    m_CurPos.theta = posRevise.theta;

    search_x_m = m_config.ScanMatchconfig.xScanMatchRange * 2;
    search_y_m = m_config.ScanMatchconfig.yScanMatchRange * 2;
    search_theta_rad =
        degrees_to_radians(m_config.ScanMatchconfig.thetaScanMatchRange) * 1.5;
    if (flag == 0) {
        m_bPoseError = true;
    }
    pthread_mutex_unlock(&m_csPose_mutex);
}

void CNaviInterface::putLaserData(laser_st *plaserData) {
    pthread_mutex_lock(&m_csLaser_mutex);

    m_stlaserdata.nranges = (*plaserData).nranges;

    m_stlaserdata.ranges.clear();

    for (int i = 0; i < (*plaserData).ranges.size(); i++) {
        m_stlaserdata.ranges.push_back((*plaserData).ranges.at(i));
    }

    m_stlaserdata.nintensities = (*plaserData).nintensities;

    m_stlaserdata.intensities.clear();

    for (int i = 0; i < (*plaserData).intensities.size(); i++) {
        m_stlaserdata.intensities.push_back((*plaserData).intensities.at(i));
    }

    pthread_mutex_unlock(&m_csLaser_mutex);

    pthread_cond_signal(&m_condPathPlan);
}
/*****************************************************************************************/
void CNaviInterface::GetVisionData(vector<Pose> *pvisionData) {

    pthread_mutex_lock(&m_csPlan_mutex);

    m_VisionWallData.clear();

    for (int i = 0; i < (*pvisionData).size(); i++) {
        m_VisionWallData.push_back((*pvisionData).at(i));
    }

    pthread_mutex_unlock(&m_csPlan_mutex);
}
/***********************************collision**********************************************/
void CNaviInterface::GetCollisionData(vector<Pose> *pcollisionData) {

    pthread_mutex_lock(&m_csPlan_mutex);

    m_CollisionWallData.clear();

    for (int i = 0; i < (*pcollisionData).size(); i++) {
        m_CollisionWallData.push_back((*pcollisionData).at(i));
    }

    pthread_mutex_unlock(&m_csPlan_mutex);
}
/***********************************collision**********************************************/
void CNaviInterface::ClearVisionData(void) {

    pthread_mutex_lock(&m_csPlan_mutex);

    m_VisionWallData.clear();

    pthread_mutex_unlock(&m_csPlan_mutex);
}

void CNaviInterface::ClearCollisionData(void) {

    pthread_mutex_lock(&m_csPlan_mutex);

    m_CollisionWallData.clear();

    pthread_mutex_unlock(&m_csPlan_mutex);
}
void CNaviInterface::putEncoderData(Pose *pPos) {
    static int encodernum = 0;

    if (encodernum == 0) {
        m_lastxyt.x = pPos->x;
        m_lastxyt.y = pPos->y;
        m_lastxyt.theta = pPos->theta;
        encodernum = 1;
    }

    m_nowxyt.x = pPos->x;
    m_nowxyt.y = pPos->y;
    m_nowxyt.theta = pPos->theta;

    //计算m_nowxyt相对于m_lastxyt的增量，增量表示在m_lastxyt坐标系下
    globalOdoT = LinAlg::xytInvMul31(m_lastxyt, m_nowxyt);
    pthread_mutex_lock(&m_csPose_mutex);

    if (m_eNaviType == LOCALIZATION && m_bIsLoc) {
        if (m_bslam) {
            Pose curPose;

            if (m_pscanMatcher != NULL) {

                m_pscanMatcher->getPosition(curPose);
                curPose = LinAlg::xytMultiply(curPose, globalOdoT);
                m_pscanMatcher->SetXyt(curPose);
            }

        } else {

            m_CurPos = LinAlg::xytMultiply(m_CurPos, globalOdoT);
            m_CurPosForPath = LinAlg::xytMultiply(m_CurPosForPath, globalOdoT);
            scanlinktest = LinAlg::xytMultiply(scanlinktest, globalOdoT);
            chargemappose = LinAlg::xytMultiply(chargemappose, globalOdoT);
        }
    }

    if (m_eNaviType == MANUAL) {

        Pose curPose;

        if (m_pscanMatcher != NULL) {

            m_pscanMatcher->getPosition(curPose);
            curPose = LinAlg::xytMultiply(curPose, globalOdoT);
            m_pscanMatcher->SetXyt(curPose);
        }
    }

    pthread_mutex_unlock(&m_csPose_mutex);
    m_lastxyt.x = pPos->x;
    m_lastxyt.y = pPos->y;
    m_lastxyt.theta = pPos->theta;
}

void CNaviInterface::RevisePoseAndRange(Pose &posRevise, double xSearchRange,
                                        double ySearchRange,
                                        double thetaSearchRange,
                                        unsigned char flag) {
    pthread_mutex_lock(&m_csPose_mutex);
    if (1 == flag) {
        m_CurPos.x = posRevise.x;
        m_CurPos.y = posRevise.y;
        m_CurPos.theta = posRevise.theta;
    }

    search_x_m = xSearchRange;
    search_y_m = ySearchRange;
    search_theta_rad = thetaSearchRange;

    revise_Range_x = xSearchRange;
    revise_Range_y = ySearchRange;
    revise_Range_theta = thetaSearchRange;

    m_bRevise = true;
    pthread_mutex_unlock(&m_csPose_mutex);
}

void CNaviInterface::recentervisionmap(Pose &p1, Pose &p2, GridMap &map) {
    if (m_bifrecenter) {
        double x = (p1.x + p2.x) / 2;
        double y = (p1.y + p2.y) / 2;
        map.recenter(x, y, 1);
    }
    // draw
    map.fill(0);
    Pose lastp = p1;
    bool bFirst = true;
    m_bifrecenter = false;
}

void CNaviInterface::update(vector<Pose> &bodyPoints,
                            vector<Pose> &forelaserPoints,
                            vector<Pose> &limitlaserPoints) {
    // sensor to body

    double search_theta_res_m = degrees_to_radians(1.0);
    vector<double> pose_flag;
    double dErrorPose[2] = {0.0};
    pthread_mutex_lock(&m_csPose_mutex);
    if (m_eNaviType == LOCALIZATION &&
        m_bIsLoc) //需要底盘定位后，m_bIsLoc 置成 true
    {
        if (!m_bConverged || m_eLocationType == SCANMATCH) //自主行走模式
        {
            Pose res;
            res.x = m_CurPos.x;
            res.y = m_CurPos.y;
            res.theta = m_CurPos.theta;
            //自主行走用的是m_pmatcher，建图用的是m_pscanMatcher，m_pmsSLAM
            //在loadmap中，m_pmatcher的gm已经是高斯地图了

            //只要没有底盘定位成功，则m_imode =
            // 0，和之前方式一样，如果定位成功，就走直方图滤波
            vector<int> id;
            id.clear();

            int bias_level;

            if (m_bRevise == true) {
                m_imode = 0;
                m_ifirstinitloc = 1;
                m_pmatcher->initWeight();
            }
            double h2;

            if ((m_bfind || 0 == m_imode) && !m_bmanualupdate) {

                m_pmatcher->initWeight();
                m_pmatcher->matchRaw(bodyPoints, m_CurPos, NULL, 0.4, 0.4,
                                     search_theta_rad, search_theta_res_m, res,
                                     pose_flag, 0);

                lastAmapupdatepose = res;

            } else {

                if (!m_bexpandmap) {

                    int navimode;
                    m_pmatcher->HistogramFilter_matchRaw(bodyPoints, m_CurPos,
                                                         res, pose_flag,
                                                         m_imode, id, navimode);

                    int bias_x = id.at(0) - 4;
                    int bias_y = id.at(1) - 4;
                    int bias = bias_x * bias_x + bias_y * bias_y;
                    if (bias == 0) {
                        bias_level = 0;
                        m_ijumpnum = 0;
                    } else {
                        if (bias > 0 && bias < 8) {
                            bias_level = 1;
                            m_ijumpnum += 1;
                        } else {
                            bias_level = 2;
                            m_ijumpnum += 2;
                        }
                    }

                    if ((6 <= m_ijumpnum) && (!m_bmanualupdate)) {
                        printf("find jump\n");fflush(stdout);
                        m_ijumpnum = 0;
                        m_bfind = true;
                        m_bfind2rc = true;
                        m_ucfindmode = 0;
                        dErrorPose[0] = res.x;
                        dErrorPose[1] = res.y;
                        triggerCoopAvoidance(COOP_AVOID_TRIGGER_MATCH_JUMP);
                    }

                    res.theta = Simu_normalize_theta(res.theta);
                    h2 = pose_flag.at(1);

                    if (m_bcreateautochargemap) {
                        // printf("create chargemap !\n");
                        if (0 == autochargemapcount) {
                            if ((res.x < 1.5) && (res.y < 1.5)) {
                                chargemappose = res;
                                lastchargemappose = res;
                                vector<vector<Pose>> test_rpoints;
                                pscanMatcher->contourExtractor.getContours_Pose(
                                    bodyPoints, test_rpoints);

                                for (int i = 0; i < test_rpoints.size(); i++) {
                                    vector<Pose> contour_points =
                                        test_rpoints.at(i);

                                    pscanMatcher->addProbMap(
                                        m_pms->autochargemap, chargemappose,
                                        contour_points, 5);
                                }

                                autochargemapcount++;
                            } else {
                                m_bcreateautochargemap = false;
                                printf("chargemap creation is error, curpose "
                                       "is not close to origin !\n");fflush(stdout);
                            }
                        } else {
                            GridMap GaussianMap;
                            int ix = (int)((chargemappose.x - 25 -
                                            m_pms->autochargemap.x0) /
                                           0.05);
                            int iy = (int)((chargemappose.y - 25 -
                                            m_pms->autochargemap.y0) /
                                           0.05);
                            double x0 = m_pms->autochargemap.x0 + ix * 0.05;
                            double y0 = m_pms->autochargemap.y0 + iy * 0.05;
                            LUT lut;
                            GaussianMap.makePixels(x0, y0, 1000, 1000, 0.05,
                                                   (BYTE)0, false);
                            GaussianMap.makeGaussianLUT(
                                1.0, 0, 1.0 / LinAlg::sq(0.06), lut);

                            int autochargemapwidthtop = GaussianMap.width + ix;
                            int autochargemapheighttop =
                                GaussianMap.height + iy;

                            vector<double> flag;
                            flag.clear();
                            flag.push_back(0);
                            flag.push_back(0);
                            flag.push_back(0);

                            if ((ix >= 0) &&
                                (ix < m_pms->autochargemap.width) &&
                                (autochargemapwidthtop >= 0) &&
                                (autochargemapwidthtop <
                                 m_pms->autochargemap.width) &&
                                (iy >= 0) &&
                                (iy < m_pms->autochargemap.height) &&
                                (autochargemapheighttop >= 0) &&
                                (autochargemapheighttop <
                                 m_pms->autochargemap.height)) {
                                for (int i = 0; i < GaussianMap.width; i++) {
                                    for (int j = 0; j < GaussianMap.height;
                                         j++) {
                                        if (m_pms->autochargemap
                                                .data[(j + iy) *
                                                          m_pms->autochargemap
                                                              .width +
                                                      i + ix] > 0.5) {
                                            GaussianMap.drawDot(
                                                (i + 0.5) *
                                                        m_pms->autochargemap
                                                            .metersPerPixel +
                                                    x0,
                                                (j + 0.5) *
                                                        m_pms->autochargemap
                                                            .metersPerPixel +
                                                    y0,
                                                lut, lut.length);
                                        }
                                    }
                                }
                                pmatcher.setModel(GaussianMap);

                                // map match
                                Pose autochargemapres;
                                pmatcher.matchRaw(bodyPoints, chargemappose,
                                                  NULL, search_x_m, search_y_m,
                                                  search_theta_rad,
                                                  search_theta_res_m,
                                                  autochargemapres, flag, 0);

                                chargemappose = autochargemapres;

                                double ddist = LinAlg::DistancePose(
                                    chargemappose, lastchargemappose);
                                double dtheta = fabs(
                                    MathUtil::mod2pi(chargemappose.theta -
                                                     lastchargemappose.theta));

                                if (ddist >= 0.4 ||
                                    dtheta >= MathUtil::toRadians(30)) {
                                    vector<vector<Pose>> test_rpoints;
                                    pscanMatcher->contourExtractor
                                        .getContours_Pose(bodyPoints,
                                                          test_rpoints);

                                    for (int i = 0; i < test_rpoints.size();
                                         i++) {
                                        vector<Pose> contour_points =
                                            test_rpoints.at(i);
                                        if (contour_points.size() <= 1) {
                                            continue;
                                        }

                                        pscanMatcher->addProbMap(
                                            m_pms->autochargemap, chargemappose,
                                            contour_points, 4);
                                    }

                                    lastchargemappose = chargemappose;

                                    autochargemapcount++;

                                    if (5 <= autochargemapcount) {
                                        printf(
                                            "chargemap creation is done !\n");fflush(stdout);
                                        m_bcreateautochargemap = false;
                                    }
                                }
                            } else {
                                printf("chargemap creation is out of range\n");fflush(stdout);
                                m_bcreateautochargemap = false;
                            }
                        }
                    }

                    if (m_bautocharge) {

                        GridMap GaussianMap;
                        int ix =
                            (int)((m_CurPos.x - 25 - m_pms->autochargemap.x0) /
                                  0.05);
                        int iy =
                            (int)((m_CurPos.y - 25 - m_pms->autochargemap.y0) /
                                  0.05);

                        double x0 = m_pms->autochargemap.x0 + ix * 0.05;
                        double y0 = m_pms->autochargemap.y0 + iy * 0.05;
                        LUT lut;
                        GaussianMap.makePixels(x0, y0, 1000, 1000, 0.05,
                                               (BYTE)0, false);
                        GaussianMap.makeGaussianLUT(
                            1.0, 0, 1.0 / LinAlg::sq(0.06), lut);

                        int autochargemapwidthtop = GaussianMap.width + ix;
                        int autochargemapheighttop = GaussianMap.height + iy;

                        vector<double> flag;
                        flag.clear();
                        flag.push_back(0);
                        flag.push_back(0);
                        flag.push_back(0);

                        if ((ix >= 0) && (ix < m_pms->autochargemap.width) &&
                            (autochargemapwidthtop >= 0) &&
                            (autochargemapwidthtop <
                             m_pms->autochargemap.width) &&
                            (iy >= 0) && (iy < m_pms->autochargemap.height) &&
                            (autochargemapheighttop >= 0) &&
                            (autochargemapheighttop <
                             m_pms->autochargemap.height)) {
                            for (int i = 0; i < GaussianMap.width; i++) {
                                for (int j = 0; j < GaussianMap.height; j++) {
                                    if (m_pms->autochargemap.data
                                            [(j + iy) *
                                                 m_pms->autochargemap.width +
                                             i + ix] > 0.5) {
                                        GaussianMap.drawDot(
                                            (i + 0.5) * m_pms->autochargemap
                                                            .metersPerPixel +
                                                x0,
                                            (j + 0.5) * m_pms->autochargemap
                                                            .metersPerPixel +
                                                y0,
                                            lut, lut.length);
                                    }
                                }
                            }
                            pmatcher.setModel(GaussianMap);
                            // printf("switch to autochargemap !\n");

                            Pose chargepose;
                            double chargescore = 0.0;

                            pmatcher.matchRaw(bodyPoints, m_CurPos, NULL, 0.3,
                                              0.3, search_theta_rad,
                                              search_theta_res_m, chargepose,
                                              flag, 0);
                            chargescore = flag.at(1);
                            if (0.5 > chargescore) {
                                printf("chargemap is not good enough !\n");fflush(stdout);
                            } else {
                                res = chargepose;
                                h2 = 0.65;
                            }

                        } else {
                            printf("autochargepose is out of range\n");fflush(stdout);
                        }
                    }

                    if (0.5 > h2) {
                        printf("h2 = %f\n", h2);fflush(stdout);
                    }

                    if (0.7 < h2) {

                        double ddist = LinAlg::DistancePose(lastfixpose, res);
                        visionswitch = true;

                        // double dtheta =
                        // fabs(Simu_normalize_theta(lastfixpose.theta -
                        // res.theta) ); double thetachange = PI/6; if(ddist >=
                        // 0.2 || dtheta >= thetachange)
                        if (ddist >= 0.2) {
                            lastfixpose = res;
                            vector<vector<Pose>> test_rpoints;
                            m_pms->contourExtractor.getContours_Pose(
                                forelaserPoints, test_rpoints);
                            for (int i = 0; i < test_rpoints.size(); i++) {
                                vector<Pose> contour_points =
                                    test_rpoints.at(i);
                                if (contour_points.size() <= 1) {
                                    continue;
                                }

                                m_pscanMatcher->addProbMap(
                                    m_pms->globalcorrectionMap, res,
                                    contour_points, 5);
                                m_pscanMatcher->addProbMap(
                                    m_pms->globalcorrectionMap2, res,
                                    contour_points, 5);
                            }
                        }
                    }
                    if ((h2 < 0.6) && (!m_bmanualupdate)) //
                    {
                        double a2 = 0.0;
                        Pose candidateres;
                        GridMap GaussianMap;
                        int ix = (int)((m_CurPos.x - 25 -
                                        m_pms->globalcorrectionMap.x0) /
                                       0.05);
                        int iy = (int)((m_CurPos.y - 25 -
                                        m_pms->globalcorrectionMap.y0) /
                                       0.05);

                        double x0 = m_pms->globalcorrectionMap.x0 + ix * 0.05;
                        double y0 = m_pms->globalcorrectionMap.y0 + iy * 0.05;
                        LUT lut;
                        GaussianMap.makePixels(x0, y0, 1000, 1000, 0.05,
                                               (BYTE)0, false);
                        GaussianMap.makeGaussianLUT(
                            1.0, 0, 1.0 / LinAlg::sq(0.06), lut);

                        int submapbwidthtop = GaussianMap.width + ix;
                        int submapbheighttop = GaussianMap.height + iy;

                        vector<double> flag;
                        flag.clear();
                        flag.push_back(0);
                        flag.push_back(0);
                        flag.push_back(0);

                        if ((ix >= 0) &&
                            (ix < m_pms->globalcorrectionMap.width) &&
                            (submapbwidthtop >= 0) &&
                            (submapbwidthtop <
                             m_pms->globalcorrectionMap.width) &&
                            (iy >= 0) &&
                            (iy < m_pms->globalcorrectionMap.height) &&
                            (submapbheighttop >= 0) &&
                            (submapbheighttop <
                             m_pms->globalcorrectionMap.height)) {
                            for (int i = 0; i < GaussianMap.width; i++) {
                                for (int j = 0; j < GaussianMap.height; j++) {
                                    if (m_pms->globalcorrectionMap
                                            .data[(j + iy) *
                                                      m_pms->globalcorrectionMap
                                                          .width +
                                                  i + ix] > 0.5) {
                                        GaussianMap.drawDot(
                                            (i + 0.5) *
                                                    m_pms->globalcorrectionMap
                                                        .metersPerPixel +
                                                x0,
                                            (j + 0.5) *
                                                    m_pms->globalcorrectionMap
                                                        .metersPerPixel +
                                                y0,
                                            lut, lut.length);
                                    }
                                }
                            }
                            pmatcher.setModel(GaussianMap);

                            pmatcher.matchRaw(
                                bodyPoints, m_CurPos, NULL, search_x_m,
                                search_y_m, search_theta_rad,
                                search_theta_res_m, candidateres, flag, 0);
                            // candidateres.theta =
                            // Simu_normalize_theta(candidateres.theta);
                            a2 = flag.at(1);
                        } else {
                            a2 = 0;
                        }
                        if ((a2 > h2) && (a2 > 0.6)) {
                            printf("switch map to B\n");fflush(stdout);
                            res.x = candidateres.x;
                            res.y = candidateres.y;
                            res.theta = candidateres.theta;

                            visionswitch = true;

                            pose_flag = flag;

                            h2 = a2;

                            double ddist =
                                LinAlg::DistancePose(lastfixpose, res);
                            if ((ddist > 0.2) && (a2 > 0.6)) {
                                lastfixpose = res;
                                vector<vector<Pose>> test_rpoints;
                                m_pms->contourExtractor.getContours_Pose(
                                    forelaserPoints, test_rpoints);
                                for (int i = 0; i < test_rpoints.size(); i++) {
                                    vector<Pose> contour_points =
                                        test_rpoints.at(i);
                                    if (contour_points.size() <= 1) {
                                        continue;
                                    }

                                    m_pscanMatcher->addProbMap(
                                        m_pms->globalcorrectionMap2, res,
                                        contour_points, 4);
                                }
                            }

                        } else {
                            Pose candidaterb;
                            double b2 = 0.0;

                            int ix1 = (int)((m_CurPos.x - 25 -
                                             m_pms->globalcorrectionMap2.x0) /
                                            0.05);
                            int iy1 = (int)((m_CurPos.y - 25 -
                                             m_pms->globalcorrectionMap2.y0) /
                                            0.05);
                            double x0 =
                                m_pms->globalcorrectionMap2.x0 + ix * 0.05;
                            double y0 =
                                m_pms->globalcorrectionMap2.y0 + iy * 0.05;
                            LUT lut;
                            GaussianMap.makePixels(x0, y0, 1000, 1000, 0.05,
                                                   (BYTE)0, false);
                            GaussianMap.makeGaussianLUT(
                                1.0, 0, 1.0 / LinAlg::sq(0.06), lut);

                            submapbwidthtop = GaussianMap.width + ix1;
                            submapbheighttop = GaussianMap.height + iy1;

                            vector<double> flag;
                            flag.clear();
                            flag.push_back(0);
                            flag.push_back(0);
                            flag.push_back(0);
                            if ((ix1 >= 0) &&
                                (ix1 < m_pms->globalcorrectionMap2.width) &&
                                (submapbwidthtop >= 0) &&
                                (submapbwidthtop <
                                 m_pms->globalcorrectionMap2.width) &&
                                (iy1 >= 0) &&
                                (iy1 < m_pms->globalcorrectionMap2.height) &&
                                (submapbheighttop >= 0) &&
                                (submapbheighttop <
                                 m_pms->globalcorrectionMap2.height)) {
                                for (int i = 0; i < GaussianMap.width; i++) {
                                    for (int j = 0; j < GaussianMap.height;
                                         j++) {
                                        if (m_pms->globalcorrectionMap2.data
                                                [(j + iy1) *
                                                     m_pms->globalcorrectionMap2
                                                         .width +
                                                 i + ix1] > 0.5) {
                                            GaussianMap.drawDot(
                                                (i + 0.5) *
                                                        m_pms
                                                            ->globalcorrectionMap2
                                                            .metersPerPixel +
                                                    x0,
                                                (j + 0.5) *
                                                        m_pms
                                                            ->globalcorrectionMap2
                                                            .metersPerPixel +
                                                    y0,
                                                lut, lut.length);
                                        }
                                    }
                                }
                                pmatcher.setModel(GaussianMap);
                                pmatcher.matchRaw(
                                    bodyPoints, m_CurPos, NULL, search_x_m,
                                    search_y_m, search_theta_rad,
                                    search_theta_res_m, candidaterb, flag, 0);
                                b2 = flag.at(1);
                            } else {
                                b2 = 0;
                            }
                            if ((b2 > h2) && (b2 > a2) && (b2 > 0.6)) {
                                printf("switch map to C\n");fflush(stdout);
                                res.x = candidaterb.x;
                                res.y = candidaterb.y;
                                res.theta = candidaterb.theta;

                                visionswitch = true;

                                pose_flag = flag;

                                h2 = b2;

                                double ddist =
                                    LinAlg::DistancePose(lastfixpose, res);
                                if ((ddist > 0.2) && (b2 > 0.6)) {
                                    lastfixpose = res;
                                    vector<vector<Pose>> test_rpoints;
                                    m_pms->contourExtractor.getContours_Pose(
                                        forelaserPoints, test_rpoints);
                                    for (int i = 0; i < test_rpoints.size();
                                         i++) {
                                        vector<Pose> contour_points =
                                            test_rpoints.at(i);
                                        if (contour_points.size() <= 1) {
                                            continue;
                                        }

                                        m_pscanMatcher->addProbMap(
                                            m_pms->globalcorrectionMap, res,
                                            contour_points, 4);
                                    }
                                }

                            } else {

                                if ((h2 < 0.35) || (h2 > 0.45) ||
                                    (1 == g_iSaveDateFlag)) {
                                    visionswitch = true;
                                } else {
                                    visionswitch = false;
                                }
                            }
                        }
                    }
                }
#if 1
                Pose scanlinkres;
                scanlinkres.x = res.x;
                scanlinkres.y = res.y;
                scanlinkres.theta = res.theta;

                vector<double> scanlinkpose_flag;
                scanlinkpose_flag.push_back(0);
                scanlinkpose_flag.push_back(0);
                scanlinkpose_flag.push_back(0);

                pthread_mutex_lock(&m_initloc_mutex);

                if (!m_bfind && (true == m_bodmswitch)) {
                    if (m_ifirstinitloc == 1) {
                        navipose.x = res.x;
                        navipose.y = res.y;
                        navipose.theta = res.theta;

                        scanlinktest.x = res.x;
                        scanlinktest.y = res.y;
                        scanlinktest.theta = res.theta;

                        m_ifirstinitloc = 0;
                        scanlinkpose.x = scanlinkres.x;
                        scanlinkpose.y = scanlinkres.y;
                        scanlinkpose.theta = scanlinkres.theta;
                        // draw scan
                        vector<Pose> tmp_p(forelaserPoints);

                        Scan scan;
                        // scan.xyt = scanlinkpose;
                        scan.xyt = scanlinkres;
                        LinAlg::transform(scanlinkres, tmp_p, scan.gpoints);
                        m_pscanlinkmatch->linkScanMatcher->contourExtractor
                            .getContours(scan.gpoints, scan.gcontours);
                        m_pscanlinkmatch->linkScanMatcher->drawScan(scan);
                        m_pscanlinkmatch->linkMatcher->dgm =
                            m_pscanlinkmatch->linkScanMatcher->matcher.dgm;
                    }
                    m_pscanlinkmatch->linkMatcher->matchRaw(
                        forelaserPoints, scanlinktest, NULL, search_x_m,
                        search_y_m, search_theta_rad, search_theta_res_m,
                        scanlinkres, scanlinkpose_flag, 0);
                    scanlinkres.theta = Simu_normalize_theta(scanlinkres.theta);

                    double ddist =
                        LinAlg::DistancePose(scanlinkpose, scanlinkres);
                    double dtheta = fabs(Simu_normalize_theta(
                        scanlinkpose.theta - scanlinkres.theta));
                    double thetachange = PI / 4;

                    if (ddist >= 0.4 || dtheta >= thetachange) {
                        // draw scan
                        vector<Pose> tmp_p(forelaserPoints);

                        Scan scan;
                        scan.xyt = scanlinkres;
                        scanlinkpose = scanlinkres;
                        scanlinktest = scanlinkres;
                        LinAlg::transform(scanlinkres, tmp_p, scan.gpoints);
                        m_pscanlinkmatch->linkScanMatcher->contourExtractor
                            .getContours(scan.gpoints, scan.gcontours);
                        m_pscanlinkmatch->linkScanMatcher->drawScan(scan);
                        m_pscanlinkmatch->linkMatcher->dgm =
                            m_pscanlinkmatch->linkScanMatcher->matcher.dgm;

                        double navidist = LinAlg::DistancePose(navipose, res);
                        double navitheta = fabs(
                            Simu_normalize_theta(navipose.theta - res.theta));

                        if (ddist >= 0.4) {
                            double disttest = fabs((navidist - ddist));
                            if ((disttest >= 0.2) && (!m_bmanualupdate)) {
                                printf("find laser odm !!!\n");fflush(stdout);
                                m_bfind2rc = true;
                                m_bfind = true;
                                m_ucfindmode = 1;
                                dErrorPose[0] = res.x;
                                dErrorPose[1] = res.y;
                            }
                        } else {
                            double thetatest = fabs((navitheta - dtheta));
                            if ((thetatest >= PI / 9) && (!m_bmanualupdate)) {
                                m_bfind2rc = true;
                                m_bfind = true;
                                m_ucfindmode = 2;
                                dErrorPose[0] = res.x;
                                dErrorPose[1] = res.y;
                            }
                        }

                        navipose = res;
                    }
                }

                if (m_bfind2rc && 0.5 > h2) {

                    find2rc(dErrorPose);

                } else {
                    m_bfind2rc = false;
                    m_bfind = false;
                }
                pthread_mutex_unlock(&m_initloc_mutex);
#endif
            }
            if (m_bmanualupdate) {

                manualupdatemap(m_CurPos, forelaserPoints, res, lastupdatepose,
                                limitlaserPoints, search_theta_res_m);
            }

            //*****************************

            m_CurPos.x = res.x;
            m_CurPos.y = res.y;
            m_CurPos.theta = res.theta;
            m_CurPos.theta = Simu_normalize_theta(
                m_CurPos.theta); //角度标准化，范围为[-pi,pi)

            if ((m_bupdatemap || m_bexpandmap) && !m_bfind) {
                saveupdateg(lastupdatepose, m_CurPos, updateg, h2,
                            limitlaserPoints);
            }

            double ddist = LinAlg::DistancePose(lastAmapupdatepose, m_CurPos);
            double dtheta = fabs(Simu_normalize_theta(lastAmapupdatepose.theta -
                                                      m_CurPos.theta));
            double thetachange = PI / 6;
#if 1
            bool freezeStaticNavigationMap = enableCoverage || m_coverageActive;
            if (((ddist > 0.2 || dtheta > thetachange) && h2 > 0.7) &&
                (false == m_ifmodifymap)) // h2 > 0.55
            {
                lastAmapupdatepose = m_CurPos; /////////lastupdatepose not init

                if ((m_bupdatemap || m_bexpandmap) && !freezeStaticNavigationMap) {
                    m_pms->addProbMap(m_pms->astarMap, m_CurPos, forelaserPoints);
                } else if (freezeStaticNavigationMap) {
                    printf("Static astarMap update skipped during full coverage.\n");fflush(stdout);
                }
            }
            if (true == m_ifmodifymap) {
                lastAmapupdatepose = m_CurPos; /////////lastupdatepose not init
                m_pms->ModifyProbMap(m_pms->astarMap, m_CurPos,
                                     limitlaserPoints);
            }
#endif
            if (m_bConverged) {
                search_x_m = m_config.ScanMatchconfig.xScanMatchRange;
                search_y_m = m_config.ScanMatchconfig.yScanMatchRange;
                search_theta_rad = degrees_to_radians(
                    m_config.ScanMatchconfig.thetaScanMatchRange);

                if (m_bRevise) {
                    printf("revise !!!\n");fflush(stdout);
                    m_bRevise = false;
                    m_bPoseError = false;
                    m_imode = 1;

                    if (m_pPosCbFunc != NULL)
                        m_pPosCbFunc(&m_CurPos, m_laseronlyflag, pose_flag);

                    m_bfind = true;
                    m_ifirstinitloc = 1;

                } else {

                    if (m_pPosCbFunc != NULL)
                        m_pPosCbFunc(&m_CurPos, m_laseronlyflag, pose_flag);
                }
            }

            if (!m_bPoseError) {
                m_CurPosForPath = m_CurPos;
            }

            m_bConverged = true;
            m_bslam = false;
        }

        if (m_bslam && m_eLocationType == SLAM) {
            if (m_pscanMatcher != NULL)
                m_pscanMatcher->processScan(bodyPoints, forelaserPoints,
                                            pose_flag);

            doSLAM(bodyPoints);

            Pose curPos;
            m_pscanMatcher->getPosition(curPos);

            if (m_pPosCbFunc != NULL)
                m_pPosCbFunc(&curPos, m_laseronlyflag, pose_flag);

            if (!m_bPoseError) {
                m_CurPosForPath = curPos;
            }

            if (m_bRevise) {
                if (m_bPoseError) {
                    curPos.x = m_CurPos.x;
                    curPos.y = m_CurPos.y;
                    curPos.theta = m_CurPos.theta;
                    m_CurPosForPath = curPos;
                }

                Pose res;
                res.x = 0.0;
                res.y = 0.0;
                res.theta = 0.0;
                m_bRevise = false;
                m_pmatcher->matchRaw(bodyPoints, curPos, NULL, search_x_m,
                                     search_y_m, search_theta_rad,
                                     search_theta_res_m, res, pose_flag, 0);

                m_CurPos.x = res.x;
                m_CurPos.y = res.y;
                m_CurPos.theta = res.theta;
                m_CurPos.theta = Simu_normalize_theta(m_CurPos.theta);

                if (m_pPosCbFunc != NULL)
                    m_pPosCbFunc(&m_CurPos, m_laseronlyflag, pose_flag);
                m_bslam = false;
                m_bPoseError = false;
                search_x_m = m_config.ScanMatchconfig.xScanMatchRange;
                search_y_m = m_config.ScanMatchconfig.yScanMatchRange;
                search_theta_rad = degrees_to_radians(
                    m_config.ScanMatchconfig.thetaScanMatchRange);
                m_eLocationType = SCANMATCH;
            }
        }
        if (m_bConverged && !m_bslam && m_eLocationType == SLAM) {

            printf("slam init\n");fflush(stdout);
            Pose pos;
            // MUTEX

            if (m_pscanMatcher != NULL) {
                delete m_pscanMatcher;
                m_pscanMatcher = NULL;
            }

            if (m_pmsSLAM != NULL) {
                delete m_pmsSLAM;
                m_pmsSLAM = NULL;
            }

            m_pscanMatcher = new ScanMatcher();
            m_pmsSLAM = new MapServer(15, 0.05);
            g_count = 0;
            pos.x = m_CurPos.x;
            pos.y = m_CurPos.y;
            pos.theta = m_CurPos.theta;

            if (m_pscanMatcher != NULL)
                m_pscanMatcher->SetXyt(pos);

            // m_eNaviType	= MANUAL;

            m_bslam = true;
        }
    }

    if (m_eNaviType ==
        MANUAL) //手动模式，建图，用的是m_pscanMatcher，m_pmsSLAM，自主行走用的是m_pmatcher
    {

        if (m_pscanMatcher != NULL) // vector<double> pose_flag;
            m_pscanMatcher->processScan(bodyPoints, forelaserPoints, pose_flag);
        doSLAM(bodyPoints); // m_pmsSLAM
        Pose curPos;
        m_pscanMatcher->getPosition(curPos);
        if (pose_flag.size() == 0) //只有第一个点时为0
        {
            pose_flag.push_back(1.0);
            pose_flag.push_back(1.0);
            pose_flag.push_back(1.0);
        }
        if (m_pPosCbFunc != NULL)
            m_pPosCbFunc(&curPos, m_laseronlyflag, pose_flag);
    }
    pthread_mutex_unlock(&m_csPose_mutex);
}

void CNaviInterface::find2rc(double *dErrorPose) {
    long systemtime = (long)time((time_t *)0);

    robot_control_t cmd;

    unsigned char bend = m_ucfindmode;
    cmd.utime = systemtime;
    cmd.commandid = 94;
    cmd.ndparams = 2;
    cmd.nsparams = 0;
    cmd.niparams = 0;
    cmd.nbparams = 1;
    cmd.bparams = &bend;
    cmd.dparams = dErrorPose;
    robot_control_t_publish(lcm, "ROBOT_CONTROL", &cmd);

    m_bfind2rc = false;
}

void CNaviInterface::SaveMapCallBack(void) {
    long systemtime = (long)time((time_t *)0);

    robot_control_t cmd;

    unsigned char bend = 1;
    cmd.utime = systemtime;
    cmd.commandid = 95;
    cmd.ndparams = 0;
    cmd.nsparams = 0;
    cmd.niparams = 0;
    cmd.nbparams = 1;
    cmd.bparams = &bend;
    robot_control_t_publish(lcm, "ROBOT_CONTROL", &cmd);
}
void CNaviInterface::manualupdatemap(Pose &m_CurPos,
                                     vector<Pose> &forelaserPoints, Pose &res,
                                     Pose &lastupdatepose,
                                     vector<Pose> &limitlaserPoints,
                                     double &search_theta_res_m) {
    GridMap GaussianMap;
    int ix = (int)((m_CurPos.x - 25 - m_pms->globalProbMap.x0) / 0.05);
    int iy = (int)((m_CurPos.y - 25 - m_pms->globalProbMap.y0) / 0.05);

    int nwidth = 1000;
    int nheight = 1000;

    int updatemapbwidthtop = nwidth + ix;
    int updatemapbheighttop = nheight + iy;

    if (0 > ix) {
        double x2edge = m_CurPos.x - m_pms->globalProbMap.x0;

        if (3 > x2edge) {
            res = m_CurPos;
            // printf("robot is too close to the edge : x-!\n");
            return;
        } else {
            ix = 0;
        }
    }

    if (0 > iy) {
        double y2edge = m_CurPos.y - m_pms->globalProbMap.y0;
        if (3 > y2edge) {
            res = m_CurPos;
            //  printf("robot is too close to the edge : y-!\n");
            return;
        } else {
            iy = 0;
        }
    }

    double x0 = m_pms->globalProbMap.x0 + ix * 0.05;
    double y0 = m_pms->globalProbMap.y0 + iy * 0.05;

    if (updatemapbwidthtop >= m_pms->globalProbMap.width) // infact "==" is ok
    {
        double xtop2edge = m_pms->globalProbMap.x0 +
                           m_pms->globalProbMap.width * 0.05 - m_CurPos.x;
        if (3 > xtop2edge) {
            res = m_CurPos;
            // printf("robot is too close to the edge : x+!\n");
            return;
        } else {
            updatemapbwidthtop =
                m_pms->globalProbMap.width - 1; // infact "==" is ok
        }
    }

    if (updatemapbheighttop >= m_pms->globalProbMap.height) // infact "==" is ok
    {
        double ytop2edge = m_pms->globalProbMap.y0 +
                           m_pms->globalProbMap.height * 0.05 - m_CurPos.y;
        if (3 > ytop2edge) {
            res = m_CurPos;
            // printf("robot is too close to the edge : y+!\n");
            return;
        } else {
            updatemapbheighttop =
                m_pms->globalProbMap.height - 1; // infact "==" is ok
        }
    }

    nwidth = updatemapbwidthtop - ix;
    nheight = updatemapbheighttop - iy;

    LUT lut;
    GaussianMap.makePixels(x0, y0, nwidth, nheight, 0.05, (BYTE)0, false);
    GaussianMap.makeGaussianLUT(1.0, 0, 1.0 / LinAlg::sq(0.06), lut);

    if ((ix >= 0) && (ix < m_pms->globalProbMap.width) &&
        (updatemapbwidthtop >= 0) &&
        (updatemapbwidthtop < m_pms->globalProbMap.width) && (iy >= 0) &&
        (iy < m_pms->globalProbMap.height) && (updatemapbheighttop >= 0) &&
        (updatemapbheighttop < m_pms->globalProbMap.height)) {
        for (int i = 0; i < GaussianMap.width; i++) {
            for (int j = 0; j < GaussianMap.height; j++) {
                if (m_pms->globalProbMap
                        .data[(j + iy) * m_pms->globalProbMap.width + i + ix] >
                    0.5) {
                    GaussianMap.drawDot(
                        (i + 0.5) * m_pms->globalProbMap.metersPerPixel + x0,
                        (j + 0.5) * m_pms->globalProbMap.metersPerPixel + y0,
                        lut, lut.length);
                }
            }
        }
    } else {
        res = m_CurPos;
        printf("robot is too close to the edge!\n");fflush(stdout);
        return;
    }
    pmatcher.setModel(GaussianMap);

    vector<double> flag;
    flag.clear();
    flag.push_back(0);
    flag.push_back(0);
    flag.push_back(0);
    pmatcher.matchRaw(limitlaserPoints, m_CurPos, NULL, search_x_m, search_y_m,
                      search_theta_rad, search_theta_res_m, res, flag, 0);
    double a2 = flag.at(1);
    double ddist = LinAlg::DistancePose(lastupdatepose, res);
    double dtheta =
        fabs(Simu_normalize_theta(lastupdatepose.theta - res.theta));
    double thetachange = PI / 6;
    if (ddist >= 0.35 || dtheta >= thetachange) {
        lastupdatepose = res;
        vector<vector<Pose>> test_rpoints;
        m_pms->contourExtractor.getContours_Pose(limitlaserPoints,
                                                 test_rpoints);
        for (int i = 0; i < test_rpoints.size(); i++) {
            vector<Pose> contour_points = test_rpoints.at(i);
            if (contour_points.size() <= 1) {
                continue;
            }

            if (a2 >= 0.9) {
                m_pscanMatcher->addProbMap(m_pms->globalProbMap, res,
                                           contour_points, 3);
            } else {
                if (a2 >= 0.8) {
                    m_pscanMatcher->addProbMap(m_pms->globalProbMap, res,
                                               contour_points, 4);
                } else {
                    if (a2 >= 0.6) {
                        m_pscanMatcher->addProbMap(m_pms->globalProbMap, res,
                                                   contour_points, 5);
                    } else {
                        printf("manual score is too low\n");fflush(stdout);
                    }
                }
            }
        }
#if 1
        int subedge = 15;
        int subw = subedge / 0.05;

        int subx = (res.x - subedge / 2) / 0.05;
        int suby = (res.y - subedge / 2) / 0.05;

        int biasx = (res.x - subedge / 2 - m_pms->globalProbMap.x0) / 0.05;
        int biasy = (res.y - subedge / 2 - m_pms->globalProbMap.y0) / 0.05;

        int8_t iparams[subw * subw];

        int k = 0;
        for (int j = 0; j < subw; j++) {
            for (int i = 0; i < subw; i++) {
                if (m_pms->globalProbMap
                        .data[(j + biasy) * m_pms->globalProbMap.width + i +
                              biasx] > 0.5) {
                    iparams[k] = 1;
                } else {
                    iparams[k] = 0;
                }

                k++;
            }
        }

        char src2[sizeof(iparams) / sizeof(iparams[0])];
        char tmp[10];
        memset(src2, 0, sizeof(iparams) / sizeof(iparams[0]));
        for (int i = 0; i < (sizeof(src2)); i++) {
            sprintf(tmp, "%d", iparams[i]);fflush(stdout);
            strcat(src2, tmp);
        }

        char *src;
        src = src2;

        int size_src = strlen(src);
        char *compressed = NULL;
        compressed = (char *)malloc(size_src * 2);
        memset(compressed, 0, size_src * 2);
        int gzSize = gzCompress(src, size_src, compressed, size_src * 2);
        if (gzSize <= 0) {
            printf("compress error.\n");fflush(stdout);
        }
        grid_map_t grid_data;
        grid_data.utime = 0;
        grid_data.encoding = 0;
        grid_data.x0 = (double)subx;
        grid_data.y0 = (double)suby;
        grid_data.meters_per_pixel = 0.05;
        grid_data.width = subw;
        grid_data.height = subw;
        grid_data.src = compressed;
        grid_data.datalen = gzSize;
        grid_data.dst = (uint8_t *)compressed;

        grid_map_t_publish(lcm, "MAPINFO", &grid_data);
#endif
    }
}

void CNaviInterface::saveupdateg(Pose &lastupdatepose, Pose &m_CurPos,
                                 Graph &updateg, double &h2,
                                 vector<Pose> &limitlaserPoints) {
    double ddist = LinAlg::DistancePose(lastupdatepose, m_CurPos);
    double dtheta =
        fabs(Simu_normalize_theta(lastupdatepose.theta - m_CurPos.theta));
    double thetachange = PI / 6;
    if (ddist >= 0.3 && (0.6 < h2 && h2 < 0.9)) {
        lastupdatepose = m_CurPos;
        GXYTNode gn;

        gn.state = m_CurPos;
        gn.init = m_CurPos;
        gn.truth = m_CurPos;
        gn.setAttribute("points", limitlaserPoints);
        updateg.nodes.push_back(gn);
    }
}

void CNaviInterface::doSLAM(vector<Pose> &bodyPoints) {

    if (0 == g_count) //第一次为0
    {
        if (m_pscanMatcher != NULL) //肯定不为空
        {

            GridMap gm;

            // g是向量，是proccessScan函数中的，里面元素是GNode，一个GNode里有三个点，和一个图
            m_pmsSLAM->globalUpdate(
                m_pscanMatcher->g,
                gm); // Graph g;
                     // class Graph:vector<GNode> nodes;
                     // GNode gn: pose state; pose init; pose truth

            /**************/
            // m_pmsSLAMtest->globalUpdate_test(m_pscanMatcher->g, gm);
            /*********/
            g_count = 1;
        }

    } else {

        g_count = 0;
        Pose curpos;
        m_pscanMatcher->getPosition(curpos); //取出当前的机器人位姿
        m_pmsSLAM->localUpdate(curpos, bodyPoints);
        /***************/
        // m_pmsSLAMtest->localUpdate(curpos, bodyPoints);
        /***************************/
    }
}

void CNaviInterface::setdrawvisionmode(bool mode) { drawvision = mode; }

bool CNaviInterface::loadMap(const char *strMapName, vector<Pose> &vtWallPos) {

    ifstream infile;
    infile.open(strMapName, ios::in);

    if (!infile)
        return false;

    printf("load map\n");fflush(stdout);
    pthread_mutex_lock(&m_csPlan_mutex);
    ///////
    vtwallpose.clear();
    vtwallpose = vtWallPos;
    memset(mapfilename, 0, 512);
    strcpy(mapfilename, strMapName);

    /////////

    if (m_pms != NULL) {
        delete m_pms;
        m_pms = NULL;
    }

    m_pms = new MapServer();

    if (pscanMatcher != NULL) {
        delete pscanMatcher;
        pscanMatcher = NULL;
    }

    pscanMatcher = new ScanMatcher();

    if (!m_pms->loadMap(strMapName, vtWallPos)) {
        pthread_mutex_unlock(&m_csPlan_mutex);
        return false;
    }

    //重置astar的全局地图，及初始化子地图
    astarPlanner.FreePlanner();
    astarPlanner.SetConfig(m_config.robotconfig.radius);
    astarPlanner.mallocspace(m_pms->laserMap);
    astarPlanner.PlannerInitial(m_pms->astarMap);
    astarPlanner.mallocvisionspace(m_pms->visionMap);

    //动态规划一些标志位及存储器重置
    path.clear();
    ordi = 0;
    ordi_1 = 0;
    m_ifastar = 1;

    pthread_mutex_unlock(&m_csPlan_mutex);

    pthread_mutex_lock(&m_csLoc_mutex);
    if (m_pmatcher != NULL) {
        delete m_pmatcher;
        m_pmatcher = NULL;
    }

    m_pmatcher = new MultiResolutionScanMatcher();

    m_pmatcher->setModel(m_pms->globalGaussianMap);

    if (m_pscanlinkmatch != NULL) {
        delete m_pscanlinkmatch;
        m_pscanlinkmatch = NULL;
    }

    m_pscanlinkmatch = new Scanlinkmatch;

    m_ifirstinitloc = 1;

    m_bupdatemap = false;
    m_bmanualupdate = false;
    updateg.nodes.clear();
    pthread_mutex_unlock(&m_csLoc_mutex);

    setMapLifecycleState(MAP_STATE_MAP_READY);
    return true;
}
void CNaviInterface::planPath(ProbMap &map, Pose cur, int type, int ratio) {
    timeval begintime;
    timeval endtime;

    PathPlanner wfpp;
    wfpp.setConfig(m_config.pathplannerconfig, m_config.robotconfig.width,
                   m_config.robotconfig.radius);

    // CAstar astarPlanner;

    /**************/
    //先确定好栅格大小，然后计算机器人半径占了多少个栅格
    // astarPlanner.GGridSize = map.metersPerPixel;
    astarPlanner.set_GGridSize(map.metersPerPixel);
    /****************/
    astarPlanner.SetConfig(m_config.robotconfig.radius);

    // vector<vector<double> > path;
    int j = 0;
    Pose goal;

    pthread_mutex_lock(&m_goal_mutex);

    if (1 <= m_waypoints.size()) {

        goal = m_waypoints.front();

        if (0.25 > LinAlg::DistancePose(cur, goal)) {
            printf("Distance < 0.25 (from planPath)!\n");fflush(stdout);
            m_waypoints.pop();

            double *pathdot = new double[4];

            pathdot[0] = 0;
            pathdot[1] = 0;
            pathdot[2] = 0;
            pathdot[3] = 0;

            if (m_pPathCbFunc != NULL) {
                m_pPathCbFunc(pathdot, 2, 1);
            }
            delete[] pathdot;
        }
        // TODO: if you need to replan the path, do not use
        // (!waypoints.get(0).planned)

        if (1 <= m_waypoints.size()) {

            goal = m_waypoints.front();

        } else {
            m_iplanend = 1;
            pthread_mutex_unlock(&m_goal_mutex);
            return;
        }
    } else {
        m_iplanend = 1;

        pthread_mutex_unlock(&m_goal_mutex);
        return;
    }
    pthread_mutex_unlock(&m_goal_mutex);

    m_iplanend = 0;
    astarPlanner.initLaserMap(m_pms->laserMap);

    // recentervisionmap(cur,goal,m_pms->visionMap);
    int ix = (int)((cur.x - 3.0 - m_pms->globalBinaryMap.x0) /
                    m_pms->visionMap.metersPerPixel);
    int iy = (int)((cur.y - 3.0 - m_pms->globalBinaryMap.y0) /
                    m_pms->visionMap.metersPerPixel);
    m_pms->visionMap.x0 =
        m_pms->globalBinaryMap.x0 + ix * m_pms->visionMap.metersPerPixel;
    m_pms->visionMap.y0 =
        m_pms->globalBinaryMap.y0 + iy * m_pms->visionMap.metersPerPixel;
    m_pms->visionMap.fill(0);

    int num = vpose.size();
    for (int i = 0; i < num; i++) {
        vector<Pose> pose = vpose.at(i);

        int N = pose.size();
        for (int j = 0; j < N; j++) {
            Pose p = pose.at(j);
            m_pms->visionMap.setValue(p.x, p.y, (BYTE)255);
        }
    }

    int vnum = m_VisionWallDataCopy.size();
    for (int i = 0; i < vnum; i++) {

        Pose p = m_VisionWallDataCopy.at(i);
        m_pms->visionMap.setValue(p.x, p.y, (BYTE)255); // draw vision date
    }
    int cnum = cpose.size();
    for (int i = 0; i < cnum; i++) {
        vector<Pose> pose = cpose.at(i);

        int N = pose.size();
        for (int j = 0; j < N; j++) {
            Pose p = pose.at(j);
            m_pms->visionMap.setValue(p.x, p.y, (BYTE)255);
        }
    }

    int collisionnum = m_CollisionWallDataCopy.size();
    for (int i = 0; i < collisionnum; i++) {

        Pose p = m_CollisionWallDataCopy.at(i);
        m_pms->visionMap.setValue(p.x, p.y, (BYTE)255); // draw vision date
    }

    astarPlanner.initVisionMap(m_pms->visionMap);

    if ((m_ifastar == 1) && (0 > backforwardnum)) {

        // ------------------ 新增：起点和终点合法性检查 ------------------
        vector<double> curp = {cur.x, cur.y};
        vector<double> target = {goal.x, goal.y};

        int start_check = ifrobotsafe(curp); // 检查起点是否合法（如在障碍物中）
        if (start_check == 3) {
            printf("Start point wrong: %d\n", start_check);fflush(stdout);
            if (m_pNoPathCbFunc) m_pNoPathCbFunc(start_check);
            m_ifastar = 1;
            m_iplanend = 1;
            return;
        }

        TargetCheckResult targetCheck = checkTargetLegal(target, true);
        if (targetCheck == TARGET_CHECK_STATIC_INVALID) { // 检查终点是否合法
            int reason = 2;
            printf("Target point wrong: %d\n", reason);fflush(stdout);
            if (m_pNoPathCbFunc) m_pNoPathCbFunc(reason);
            m_ifastar = 1;
            m_iplanend = 1;
            return;
        } else if (targetCheck == TARGET_CHECK_DYNAMIC_BLOCKED_LASER ||
                   targetCheck == TARGET_CHECK_DYNAMIC_BLOCKED_VISION) {
            printf("Target point temporarily blocked by dynamic map, hold and retry.\n");fflush(stdout);
            triggerCoopAvoidance(COOP_AVOID_TRIGGER_NOPATH);
            publishZeroVelocity();
            m_ifastar = 1;
            m_iplanend = 1;
            return;
        }
        // ---------------------------------------------------------------


        backforwardnum = -1;

        if ((m_VisionWallDataCopy.size() > 0)) //&& !m_blaserdwa)
        {
            int N = m_VisionWallDataCopy.size();
            double xx0 = m_pms->globalBinaryMap.x0;
            double yy0 = m_pms->globalBinaryMap.y0;
            double merPpix = m_pms->globalBinaryMap.metersPerPixel;
            int wth = m_pms->globalBinaryMap.width;

            for (int i = 0; i < N; i++) {
                Pose tmp = m_VisionWallDataCopy.at(i);
                int a = (int)((tmp.x - xx0) / merPpix);
                int b = (int)((tmp.y - yy0) / merPpix);
                m_pms->globalVisionMap.data[b * wth + a] = (BYTE)2;
            }
            m_pms->saveMap_Vision();

            vpose.push_back(m_VisionWallDataCopy); // vision date to astar

            m_VisionWallDataCopy.clear();
            // store up to 10 frames of data
            if (vpose.size() > 10) {
                vector<vector<Pose>>::iterator iter = vpose.begin();
                vpose.erase(iter);
            }
        }
        astarPlanner.InitializeCellTotal(m_pms->astarMap);

        vw_0 = 0;
        path.clear();
        ordi = 0;
        ordi_1 = 0;
        testpath.clear();
        for (int i = 0; i < 27; i++) {
            testpath.push_back(0);
        }
        m_ifastar = 0;

        // if(!m_blaserastar)//////fix me
        if (true) {
            printf("ready for plan\n");fflush(stdout);
            int reason;
            vector<double> curp;
            curp.push_back(cur.x);
            curp.push_back(cur.y);
            reason = ifrobotsafe(curp);
            if (0 == reason || 3 == reason) {
            // if (0 == reason) {
                m_pNoPathCbFunc(reason);
                printf("check for robot : error , %d\n", reason);fflush(stdout);

                m_ifastar = 1;

                return;
            } else {
                vector<double> target;
                target.push_back(goal.x);
                target.push_back(goal.y);
                TargetCheckResult targetCheck2 = checkTargetLegal(target, true);
                if (targetCheck2 == TARGET_CHECK_OK) {
                    printf("begin plan !\n");fflush(stdout);
                    bool isYES = astarPlanner.plan(goal, m_pms->astarMap, cur, path,
                                        m_pms->laserMap, m_pms->visionMap, type);
                    if (isYES == false)
                    {
                        targetErr = false;
                        setLastFullPathError(ASTAR_NO_PATH);
                    }
                } else if (targetCheck2 == TARGET_CHECK_DYNAMIC_BLOCKED_LASER ||
                           targetCheck2 == TARGET_CHECK_DYNAMIC_BLOCKED_VISION) {
                    printf("Target point temporarily blocked before planning, hold and retry.\n");fflush(stdout);
                    triggerCoopAvoidance(COOP_AVOID_TRIGGER_NOPATH);
                    publishZeroVelocity();
                    m_ifastar = 1;
                    return;
                } else {
                    reason = 2;
                    m_pNoPathCbFunc(reason);
                    printf("check for target : error , %d\n", reason);fflush(stdout);

                    m_ifastar = 1;
                    return;
                }
            }

        } else {
            astarPlanner.plan(goal, m_pms->emptyMap, cur, path,
                                m_pms->laserMap, m_pms->visionMap, type);
            printf("laser only astar\n");fflush(stdout);
        }
        int num = path.size();
        if (num > 0) {
            printf("astar planning succeed !\n");fflush(stdout);
            istartgo = 1;
            rrset = 0;
            if (1 == printnopathflag) {
                m_pLogCbFunc(1);
                printf("refind path in a*\n");fflush(stdout);
                printnopathflag = 0;
            }
            ifirstpublish = 0;
            double xx0 = m_pms->globalBinaryMap.x0;
            double yy0 = m_pms->globalBinaryMap.y0;
            double merPpix = m_pms->globalBinaryMap.metersPerPixel;
            int wth = m_pms->globalBinaryMap.width;

            for (int i = 0; i < num; i++) {
                vector<double> tmp = path.at(i);
                int a = (int)((tmp.at(0) - xx0) / merPpix);
                int b = (int)((tmp.at(1) - yy0) / merPpix);
                m_pms->globalBinaryMap.data[b * wth + a] = (BYTE)2;
            }
            m_pms->saveMap_Astar();
        }
        if (num <= 0) {
            printf("astar planning failure !\n");fflush(stdout);
            setLastFullPathError(ASTAR_NO_PATH);
            triggerCoopAvoidance(COOP_AVOID_TRIGGER_NOPATH);
            rrset += 1;
            if (0 == printnopathflag) {
                printf("have no path in a*\n");fflush(stdout);
                printnopathflag = 1;
            }
            m_ifastar = 1;
            double *vw_nopath = new double[4];

            vw_nopath[0] = 0;
            vw_nopath[1] = 0;
            vw_nopath[2] = 0;
            vw_nopath[3] = 0;

            if (m_pPathCbFunc != NULL)
                m_pPathCbFunc(vw_nopath, 2, 1);
            if (rrset >= 2) {
                int reason;
                vector<double> curp;
                curp.push_back(cur.x);
                curp.push_back(cur.y);
                reason = ifrobotsafe(curp);
                m_pNoPathCbFunc(reason);
                // double goaldist;
                // goaldist = sqrt( (m_CurPos.x-goal.x)*(m_CurPos.x-goal.x)
                // + (m_CurPos.y-goal.y)*(m_CurPos.y-goal.y) );
                // if(reason == 1 && goaldist < 6)
                //{
                //	m_blaserastar = true;
                // }
            } else {
                // vpose.clear();

                // pthread_mutex_lock(&m_goal_mutex);
                // m_VisionWallData.clear();
                // pthread_mutex_unlock(&m_goal_mutex);
                // printf("1st no path,clear vision data\n");
#if 0
                        printf("1st no path, reverse finding path\n");fflush(stdout);
                        for(int i=0;i<m_pms->testpathmap.width*m_pms->testpathmap.height;i++)
                        {
                            if(m_pms->testpathmap.data[i] != (BYTE)255 )
                            {
                                m_pms->testpathmap.data[i] = 0;
                            }
                        }

                        for(int i=0;i<astarPlanner.GMapWidth*astarPlanner.GMapLength;i++)
                        {
                            if(astarPlanner.m_gblGrid[i].state == OPEN || astarPlanner.m_gblGrid[i].state == CLOSED)
                            {
                                m_pms->testpathmap.data[i] = 2;
                            }
                        }

                        m_pms->saveMap_pathcheck();
#endif
                // printf("robot state =
                // %d\n",astarPlanner.m_gblGrid[astarPlanner.m_gblRobot[1]*astarPlanner.GMapWidth+astarPlanner.m_gblRobot[0]
                // ].state);

                astarPlanner.InitializeCellTotal(m_pms->astarMap);
                astarPlanner.plan(cur, m_pms->astarMap, goal, path,
                                    m_pms->laserMap, m_pms->visionMap, type);
                if (path.size() > 0) {
                    m_ifastar = 0;
                    printf("reverse finding succeed !!!!!\n");fflush(stdout);
                    vector<vector<double>> reversepath;
                    int num = path.size();
                    for (int i = 0; i < num; i++) {
                        reversepath.push_back(path.at(num - 1 - i));
                    }

                    path.clear();
                    for (int i = 0; i < num; i++) {
                        path.push_back(reversepath.at(i));
                    }

                    istartgo = 1;
                    rrset = 0;
                    if (1 == printnopathflag) {
                        m_pLogCbFunc(1);
                        printf("refind path in a*\n");fflush(stdout);
                        printnopathflag = 0;
                    }
                    ifirstpublish = 0;
                    double xx0 = m_pms->globalBinaryMap.x0;
                    double yy0 = m_pms->globalBinaryMap.y0;
                    double merPpix = m_pms->globalBinaryMap.metersPerPixel;
                    int wth = m_pms->globalBinaryMap.width;

                    for (int i = 0; i < num; i++) {
                        vector<double> tmp = path.at(i);
                        int a = (int)((tmp.at(0) - xx0) / merPpix);
                        int b = (int)((tmp.at(1) - yy0) / merPpix);
                        m_pms->globalBinaryMap.data[b * wth + a] = (BYTE)2;
                    }
                    m_pms->saveMap_Astar();
                } else {
                    printf("reverse fingding failure\n");fflush(stdout);
                }
            }

            delete[] vw_nopath;
            m_iplanend = 1;
            return;
        }
        m_iplanend = 1;
        return;
    }
    vector<double> vw;
    int ifend = 0;
    if (backforward) {
        if (m_CollisionWallDataCopy.size() > 0) {
            cpose.push_back(m_CollisionWallDataCopy);
        }

        backforward = false;
        m_ifastar = 1;
        backforwardnum = 6;
        double *pathdot = new double[4];

        pathdot[0] = 0;
        pathdot[1] = 0;
        pathdot[2] = 0;
        pathdot[3] = 0;

        if (m_pPathCbFunc != NULL)
            m_pPathCbFunc(pathdot, 2, 1);

        delete[] pathdot;
        return;
    }

    if (backforwardnum >= 0) {
        if (backforwardnum == 0) {
            double *pathdot = new double[4];

            pathdot[0] = 0;
            pathdot[1] = 0;
            pathdot[2] = 0;
            pathdot[3] = 0;

            if (m_pPathCbFunc != NULL)
                m_pPathCbFunc(pathdot, 2, 1);

            delete[] pathdot;
            backforwardnum -= 1;
            return;
        }
        double *pathdot = new double[4];

        printf("backforward!\n");fflush(stdout);

        pathdot[0] = -0.15;
        pathdot[1] = 0;
        pathdot[2] = -0.15;
        pathdot[3] = 0;

        if (m_pPathCbFunc != NULL)
            m_pPathCbFunc(pathdot, 2, 1);

        delete[] pathdot;
        backforwardnum -= 1;
        return;
    }
    if (!m_blaserdwa) {
        printf("ready for dwa\n");fflush(stdout);
        m_laseronlyflag = 0.0;
        DWAplan(vw, ifend);
    } else {
        m_laseronlyflag = 1.0;
        DWAplanlaseronly(vw, ifend);
        printf("laser only dwa\n");fflush(stdout);
    }
    if (1 == ifend) {
        // m_ifastar = 1;///////fix me:if the goal is cannot reach,the next
        // step is re_astar or dwa
        double *pathdot = new double[4];

        pathdot[0] = 0;
        pathdot[1] = 0;
        pathdot[2] = 0;
        pathdot[3] = 0;

        if (m_pPathCbFunc != NULL)
            m_pPathCbFunc(pathdot, 2, 1);
        delete[] pathdot;
        int reason = 2;
        if (reason != nopathstatus) {
            nopathstatus = reason;
            nopathflag = 1;
        }
        triggerCoopAvoidance(COOP_AVOID_TRIGGER_NOPATH);
        m_pNoPathCbFunc(reason); // can not arrive

        m_iplanend = 1;

        /*****

                                                int a=0;
                                                while(a<=10000)
                                                {
                                                        a++;
                                                }
                                                printf("a++ done\n");

        ****/
        return;

    } else {
        nopathstatus = 0;
    }

    double *pathdot = new double[4];

    pathdot[0] = vw.at(0);
    pathdot[1] = vw.at(1);
    pathdot[2] = vw.at(0);
    pathdot[3] = vw.at(1);

    if (m_pPathCbFunc != NULL)
        m_pPathCbFunc(pathdot, 2, 1);

    delete[] pathdot;
    m_iplanend = 1;

    return;

    // printf("path time = %f\n", (endtime.tv_sec - begintime.tv_sec)*1000 +
    // (double)(endtime.tv_usec -begintime.tv_usec)/1000);
}

//有标志位m_ifastar,flag是否到点结束，角度匹配交给底盘，需要Vcur,Wcur,需要底盘信息：执行作业状态，来决定动态逻辑
int CNaviInterface::DWAplan(vector<double> &goalvw, int &flag) {
    flag = 0;
    double MaxV = 0.1 * m_ucspeedlever; // 0.45
    double MaxW = PI / 3; // PI/4  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    double MinW = -PI / 3;

    double curx = 0.0;
    double cury = 0.0;
    double curtheta = 0.0;
    pthread_mutex_lock(&m_csPose_mutex);

    curx = m_CurPos.x;
    cury = m_CurPos.y;
    curtheta = m_CurPos.theta;

    pthread_mutex_unlock(&m_csPose_mutex);

    double endposex;
    double endposey;

    int pnum = path.size();
    if (0 < pnum) {
        endposex = path[pnum - 1].at(0);
        endposey = path[pnum - 1].at(1);
    } else {
        printf("illegal path for dwa !\n");fflush(stdout);
        return 0;
    }

    double cur2enddist = sqrt((endposex - curx) * (endposex - curx) +
                              (endposey - cury) * (endposey - cury));

    if (1.6 >= cur2enddist) {
        vector<double> pathendpose = path[pnum - 1];

        //TODO
        printf("cant arv1");fflush(stdout);
        //////////////////

        double doarrive = doArrive(pathendpose);
        if (0 == doarrive) {
            printf("can not arrive !\n");fflush(stdout);
            goalvw.push_back(0);
            goalvw.push_back(0);
            flag = 1;
            return 0;
        }
    }
    if (1 >= cur2enddist) {
        MaxV = 0.2;
        MaxW = PI / 4;
        MinW = -PI / 4;
    }
    double curv = Vcur;
    double curw = Wcur;
    double Vmax = min(Vcur + 0.08, MaxV); // 300ms plus max 0.168
    double Vmin = max(Vcur - 0.10, 0);

    if (0.3 <= Vmin) {
        Vmin = 0.2;
    }

    if (0.10 >= Vmax) {
        Vmax = 0.3;
    }

    if (Vmin >= Vmax) {
        Vmin = max(Vmax - 0.1, 0);
    }

    double Wmax = min(Wcur + PI / 3, MaxW); // PI/5   !!!!!!!!!!!!!!!!!!!!!!
    double Wmin = max(Wcur - PI / 3, MinW);
    if (Wmin >= Wmax) {
        Wmin = Wmax - PI / 4;
    }
    double Vstep = 0.02;
    double Wstep = 0.02;

    int numv = (Vmax - Vmin) / Vstep;
    int numw = (Wmax - Wmin) / Wstep;

    double v = 0;
    double w = 0;

    Pose subpos;

    double s = sin(curtheta);
    double c = cos(curtheta);

    vector<vector<Pose>> tempendpose;
    vector<Pose> tempsubpose;
    vector<Pose> endpose;
    vector<vector<double>> tempvw;
    vector<vector<double>> endvw;
    vector<double> goalthetascore;
    vector<double> goaldistscore;
    vector<double> goalthetascore_1;
    vector<double> goaldistscore_1;
    vector<double> savescore;
    vector<double> vibrascore;
    vector<double> speedscore;
    vector<double> goal;
    vector<double> goal_1;
    vector<double> goal_follow;

    if (ordi >= 0) {
        double x = path[ordi].at(0);
        double y = path[ordi].at(1);
        double dist = sqrt((x - curx) * (x - curx) + (y - cury) * (y - cury));
        if (1.3 < dist) // 0.75
        {
            m_ifastar = 1;
            goalvw.push_back(0);
            goalvw.push_back(0);
            flag = 0;
            return 0;
        }
    }

    if (ordi_1 >= 0) {
        double x = path[ordi_1].at(0);
        double y = path[ordi_1].at(1);
        double dist = sqrt((x - curx) * (x - curx) + (y - cury) * (y - cury));
        if (1.3 < dist) // 0.85
        {
            m_ifastar = 1;
            goalvw.push_back(0);
            goalvw.push_back(0);
            flag = 0;
            return 0;
        }
    }

    for (int i = ordi; i < path.size(); i++) {
        double x = path[i].at(0);
        double y = path[i].at(1);
        double dist = sqrt((x - curx) * (x - curx) + (y - cury) * (y - cury));

        ordi = i;

        if (0.4 <= dist) // 0.7
        {
            goal.push_back(path[i].at(0));
            goal.push_back(path[i].at(1));
            break;
        }
    }

    for (int i = ordi_1; i < path.size(); i++) {
        double x = path[i].at(0);
        double y = path[i].at(1);
        double dist = sqrt((x - curx) * (x - curx) + (y - cury) * (y - cury));

        ordi_1 = i;

        if (0.7 <= dist) // 0.8
        {
            goal_1.push_back(path[i].at(0));
            goal_1.push_back(path[i].at(1));
            break;
        }
    }

    for (int i = 0; i < 27; i++) {
        for (int j = testpath[i]; j < path.size(); j++) {
            double x = path[j].at(0);
            double y = path[j].at(1);
            double dist =
                sqrt((x - curx) * (x - curx) + (y - cury) * (y - cury));
            testpath[i] = j;

            if ((0.25 + i * 0.05) <= dist) {
                break;
            }
        }
        Pose tempforsafepose;
        tempforsafepose.x = path.at(testpath[i]).at(0);
        tempforsafepose.y = path.at(testpath[i]).at(1);
        tempforsafepose.theta = 0;
        double pscore = bifsave(tempforsafepose);
        if (0 == pscore) {
            m_ifastar = 1;

            goalvw.push_back(0);
            goalvw.push_back(0);
            m_pLogCbFunc(2);
            printf("path changed\n");fflush(stdout);
            return 0;
        }

        if (testpath[i] == path.size() - 1) {
            break;
        }
    }

    if ((ordi + 1) == path.size()) {
        goal.clear();
        goal.push_back(path[ordi].at(0));
        goal.push_back(path[ordi].at(1));

        // double x = path[ordi].at(0);
        // double y = path[ordi].at(1);
        // double dist = sqrt((x-m_CurPos.x)*(x-m_CurPos.x) + (y -
        // m_CurPos.y)*(y - m_CurPos.y));
    }

    if ((ordi_1 + 1) == path.size()) {
        goal_1.clear();
        goal_1.push_back(path[ordi_1].at(0));
        goal_1.push_back(path[ordi_1].at(1));

        // double x = path[ordi_1].at(0);
        // double y = path[ordi_1].at(1);
        // double dist = sqrt((x-m_CurPos.x)*(x-m_CurPos.x) + (y -
        // m_CurPos.y)*(y - m_CurPos.y));
    }

    Pose robot;

    robot.x = curx;
    robot.y = cury;
    robot.theta = curtheta;

    int goalnum;
    goalnum = choose_goal(robot, goal, goal_1, goal_follow);

    if (1 == istartgo) {
        double dtheta = 0.0;

        dtheta = DetaTheta(robot, goal_follow);
        double followDist =
            sqrt((goal_follow.at(0) - robot.x) * (goal_follow.at(0) - robot.x) +
                 (goal_follow.at(1) - robot.y) * (goal_follow.at(1) - robot.y));
        if (followDist < 0.08 || (0.08 >= dtheta && -0.08 <= dtheta)) {
            goalvw.push_back(0.0);
            goalvw.push_back(0.0);

            istartgo = 0;
            return 1;
        }
        if (0.35 < dtheta) {
            goalvw.push_back(0.0);
            goalvw.push_back(PI / 8);
            return 1;
        }
        if (0.35 >= dtheta && 0.08 < dtheta) {
            goalvw.push_back(0.0);
            goalvw.push_back(PI / 16);
            return 1;
        }
        if (-0.35 > dtheta) {
            goalvw.push_back(0.0);
            goalvw.push_back(-PI / 8);
            return 1;
        }
        if (-0.35 <= dtheta && -0.08 > dtheta) {
            goalvw.push_back(0.0);
            goalvw.push_back(-PI / 16);
            return 1;
        }

        return 1;
    }

    for (int i = 0; i <= numv; i++) {
        v = Vmin + i * Vstep;

        for (int j = 0; j <= numw; j++) {
            w = Wmin + j * Wstep;
            tempsubpose.clear();
            if (w == 0) {
                for (int j = 2; j <= 24; j = j + 2) {
                    double i = (double)(j / 10);
                    subpos.x = curx + v * i * c;
                    subpos.y = cury + v * i * s;
                    subpos.theta = curtheta;
                    tempsubpose.push_back(subpos);
                }
                tempendpose.push_back(tempsubpose);
                tempsubpose.clear();
                vector<double> vw;
                vw.push_back(v);
                vw.push_back(w);
                tempvw.push_back(vw);

            } else {
                for (int j = 2; j <= 24; j = j + 2) {
                    double i = (double)(j / 10);
                    subpos.x =
                        curx - (v / w) * s + (v / w) * sin(curtheta + w * i);
                    subpos.y =
                        cury + (v / w) * c - (v / w) * cos(curtheta + w * i);
                    subpos.theta = curtheta + w * i;
                    tempsubpose.push_back(subpos);
                }
                tempendpose.push_back(tempsubpose);
                tempsubpose.clear();
                vector<double> vw;
                vw.push_back(v);
                vw.push_back(w);
                tempvw.push_back(vw);
            }
        }
    }

    int num = tempendpose.size();

    double sumthetascore = 0.0;
    double sumdistscore = 0.0;
    double sumthetascore_1 = 0.0;
    double sumdistscore_1 = 0.0;
    double sumsavescore = 0.0;
    double sumvibrascore = 0.0;
    double sumspeedscore = 0.0;

    int endnum = 0;

    for (int i = 0; i < num; i++) {
        vector<Pose> p = tempendpose[i];

        double isave = difsafe(p);

        if (0 < isave) {
            endvw.push_back(tempvw[i]);
            double thetascore = ThetaScore(p[4], goal);
            double distscore = DistScore(p[4], goal, 0);

            double thetascore_1 =
                ThetaScore(p[4], goal_1); // change p[i] to test the effection
            double distscore_1 = DistScore(p[4], goal_1, 1);

            //算总分
            sumthetascore += thetascore;
            sumdistscore += distscore;
            sumsavescore += isave;

            sumthetascore_1 += thetascore_1;
            sumdistscore_1 += distscore_1;

            if (1 <= ifirstpublish) {
                double difvibra = 0.0;
                difvibra = tempvw[i].at(1) * dvibration;
                if (0.0 <= difvibra) {
                    vibrascore.push_back(2);
                    sumvibrascore += 2;
                } else {
                    vibrascore.push_back(1);
                    sumvibrascore += 1;
                }
            } else {
                vibrascore.push_back(1);
            }

            //各个分压栈
            goalthetascore.push_back(thetascore);
            goaldistscore.push_back(distscore);
            savescore.push_back(isave);

            goalthetascore_1.push_back(thetascore_1);
            goaldistscore_1.push_back(distscore_1);

            endnum++;
        }
    }

    double bestscore = 0.0;
    double a = 3.8; // theta
    double b = 2.0; // dist
    double d = 2.0; // safe
    double e = 0.2; // vib  0.5

    double f = 3.4; // theta 1.2 !!!!!!!!!!!!!!
    double g = 2.4; // dist 2.0
    int bestid = 0;

    if (1 > endnum) {
        printf("endnum = %d\n", endnum);fflush(stdout);
        goalvw.push_back(0);
        goalvw.push_back(0);
        m_ifastar = 1;
        flag = 0;
        m_blaserdwa = true;
        m_pLogCbFunc(3);
        return 0;
    }

    if (sumthetascore == 0 || 0 == sumdistscore) {
        goalvw.push_back(0);
        goalvw.push_back(0);

        m_ifastar = 1;
        return 0;
    }

    for (int i = 0; i < endnum; i++) {

        //各个分算比例
        goalthetascore[i] = goalthetascore[i] / sumthetascore;
        goaldistscore[i] = goaldistscore[i] / sumdistscore;
        savescore[i] = savescore[i] / sumsavescore;

        goalthetascore_1[i] = goalthetascore_1[i] / sumthetascore_1;
        goaldistscore_1[i] = goaldistscore_1[i] / sumdistscore_1;

        if (1 <= ifirstpublish) {
            vibrascore[i] = vibrascore[i] / sumvibrascore;
        } else {
            vibrascore[i] = 0;
        }

        double tempbestscore = 0.0;

        if (1 == goalnum) {
            tempbestscore = a * goalthetascore[i] + b * goaldistscore[i] +
                            d * savescore[i] + e * vibrascore[i] +
                            f * goalthetascore[i] + g * goaldistscore[i];
        } else {
            tempbestscore = a * goalthetascore_1[i] + b * goaldistscore_1[i] +
                            d * savescore[i] + e * vibrascore[i] +
                            f * goalthetascore_1[i] + g * goaldistscore_1[i];
        }

        //找到最高分
        if (bestscore < tempbestscore) {
            bestscore = tempbestscore;
            bestid = i;
        }
    }
    ifirstpublish = 1;

    //最高分对应的v,w
    goalvw = endvw[bestid];
    if (0 <= goalvw[1]) {
        dvibration = 1;
    } else {
        dvibration = -1;
    }

    flag = 0;
    m_ifastar = 0;
    return 1;
}

int CNaviInterface::choose_goal(Pose &robot, vector<double> &goal,
                                vector<double> &goal_1,
                                vector<double> &goal_follow) {
    goal_follow.clear();
    double metersperpix = 0.05;
    double xa = robot.x;
    double ya = robot.y;
    double xb = goal_1.at(0);
    double yb = goal_1.at(1);

    double dist =
        sqrt((xb - xa) * (xb - xa) + (yb - ya) * (yb - ya)); //算出两点距离
    int nsteps = (int)(dist / metersperpix + 1); //这些距离有多少个像素点
    double pixelsPerMeter = 1.0 / metersperpix;

    //算a/b之间的直线上每个像素点的坐标，x/y单独算
    //例：一共10个点，第3个点：x = xa+3/10(xb-xa) = (1-3/10)xa+3/10xb =
    // alpha*xb+(1-alpha)*xa, xa/xb反过来也一样
    for (int i = 0; i < nsteps; i++) {
        double alpha = ((double)i) / nsteps;
        double x = xa * alpha + xb * (1 - alpha);
        double y = ya * alpha + yb * (1 - alpha);

        int ix = (x - m_pms->astarMap.x0) / metersperpix;
        int iy = (y - m_pms->astarMap.y0) / metersperpix;

        int GMapWidth = m_pms->astarMap.width;

        int value = astarPlanner.m_pGridState[iy * GMapWidth + ix].CurrentState;
        if ((0 == value) || (5 == value)) {
            int lx = (x - m_pms->laserMap.x0) / metersperpix;
            int ly = (y - m_pms->laserMap.y0) / metersperpix;

            int width = m_pms->laserMap.width;
            int height = m_pms->laserMap.height;

            int vx = (x - m_pms->visionMap.x0) / metersperpix;
            int vy = (y - m_pms->visionMap.y0) / metersperpix;

            int vwidth = m_pms->visionMap.width;
            int vheight = m_pms->visionMap.height;

            if (lx >= 0 && lx < width && ly >= 0 && ly < height) {
                int laservalue = astarPlanner.m_plaserGridState[ly * width + lx]
                                     .CurrentState;

                if (laservalue != 0 && laservalue != 5) {
                    goal_follow.push_back(goal.at(0));
                    goal_follow.push_back(goal.at(1));
                    return 1;
                }

                if (vx >= 0 && vx < vwidth && vy >= 0 && vy < vheight) {
                    int visionvalue =
                        astarPlanner.m_pvisionGridState[vy * vwidth + vx]
                            .CurrentState;
                    if (visionvalue != 0 && visionvalue != 5) {
                        goal_follow.push_back(goal.at(0));
                        goal_follow.push_back(goal.at(1));
                        return 1;
                    }
                }
            }
        } else {
            goal_follow.push_back(goal.at(0));
            goal_follow.push_back(goal.at(1));
            return 1;
        }
    }
    goal_follow.push_back(goal_1.at(0));
    goal_follow.push_back(goal_1.at(1));
    return 2;
}

double CNaviInterface::difsafe(vector<Pose> &path) {
    int num = path.size();
    if (num == 0) {
        return 0;
    }
    double sumscore;
    sumscore = 0;
    for (int i = 0; i < num; i++) {
        double score = bifsave(path[i]);
        if (0 == score) {
            return 0;
        }
        sumscore += score;
    }
    return sumscore;
}

double CNaviInterface::difsafelaseronly(vector<Pose> &path) {
    int num = path.size();
    if (num == 0) {
        return 0;
    }
    double sumscore;
    sumscore = 0;
    for (int i = 0; i < num; i++) {
        double score = bifsavelaseronly(path[i]);
        if (0 == score) {
            return 0;
        }
        sumscore += score;
    }
    return sumscore;
}

double CNaviInterface::bifsave(Pose &p) {

    double metersperpix = m_pms->astarMap.metersPerPixel;

    int ix = (p.x - m_pms->astarMap.x0) / metersperpix;
    int iy = (p.y - m_pms->astarMap.y0) / metersperpix;

    int GMapWidth = m_pms->astarMap.width;

    int value = astarPlanner.m_pGridState[iy * GMapWidth + ix].CurrentState;
    if ((0 == value) || (5 == value)) {
        int lx = (p.x - m_pms->laserMap.x0) / metersperpix;
        int ly = (p.y - m_pms->laserMap.y0) / metersperpix;

        int width = m_pms->laserMap.width;
        int height = m_pms->laserMap.height;

        int vx = (p.x - m_pms->visionMap.x0) / metersperpix;
        int vy = (p.y - m_pms->visionMap.y0) / metersperpix;

        int vwidth = m_pms->visionMap.width;
        int vheight = m_pms->visionMap.height;

        if (lx >= 0 && lx < width && ly >= 0 && ly < height) {
            int laservalue =
                astarPlanner.m_plaserGridState[ly * width + lx].CurrentState;

            if (laservalue != 0 && laservalue != 5) {
                // printf("lasermap !\n");
                return 0;
            }

            if (vx >= 0 && vx < vwidth && vy >= 0 && vy < vheight) {
                int visionvalue =
                    astarPlanner.m_pvisionGridState[vy * vwidth + vx]
                        .CurrentState;
                if (visionvalue != 0 && visionvalue != 5) {
                    // printf("visionmap !\n");
                    return 0;
                }
            }

            if (0 == laservalue) {
                if (0 == value) {
                    if (vx >= 0 && vx < vwidth && vy >= 0 && vy < vheight) {
                        int visionvalue =
                            astarPlanner.m_pvisionGridState[vy * vwidth + vx]
                                .CurrentState;
                        if (visionvalue == 0) {
                            return 16;
                        } else {
                            return 10;
                        }
                    }
                    return 16;
                } else {
                    return 10;
                }
            }
            if (5 == laservalue) {
                if (0 == value) {
                    if (vx >= 0 && vx < vwidth && vy >= 0 && vy < vheight) {
                        int visionvalue =
                            astarPlanner.m_pvisionGridState[vy * vwidth + vx]
                                .CurrentState;
                        if (visionvalue == 5) {
                            return 4;
                        }
                    }
                    return 6;
                } else {
                    return 4;
                }
            } else {
                return 0;
            }
        }

        if (vx >= 0 && vx < vwidth && vy >= 0 && vy < vheight) {
            int visionvalue =
                astarPlanner.m_pvisionGridState[vy * vwidth + vx].CurrentState;
            if (visionvalue == 0) {
                if (0 == value) {
                    return 3;
                }
                return 1;
            }
            if (visionvalue == 5) {
                if (0 == value) {
                    return 1;
                }
                return 0;
            }
            return 0;
        }

        if (0 == value) {
            return 3;
        } else {
            return 1;
        }

    } else {
        // printf("astarmap !\n");
        return 0;
    }
}

double CNaviInterface::bifsavelaseronly(Pose &p) {

    double metersperpix = m_pms->astarMap.metersPerPixel;

    int ix = (p.x - m_pms->astarMap.x0) / metersperpix;
    int iy = (p.y - m_pms->astarMap.y0) / metersperpix;

    int GMapWidth = m_pms->astarMap.width;

    int lx = (p.x - m_pms->laserMap.x0) / metersperpix;
    int ly = (p.y - m_pms->laserMap.y0) / metersperpix;

    int width = m_pms->laserMap.width;
    int height = m_pms->laserMap.height;

    int vx = (p.x - m_pms->visionMap.x0) / metersperpix;
    int vy = (p.y - m_pms->visionMap.y0) / metersperpix;

    int vwidth = m_pms->visionMap.width;
    int vheight = m_pms->visionMap.height;

    if (lx >= 0 && lx < width && ly >= 0 && ly < height) {
        int laservalue =
            astarPlanner.m_plaserGridState[ly * width + lx].CurrentState;

        if (laservalue != 0 && laservalue != 5) {
            return 0;
        }

        if (vx >= 0 && vx < vwidth && vy >= 0 && vy < vheight) {
            int visionvalue =
                astarPlanner.m_pvisionGridState[vy * vwidth + vx].CurrentState;
            if (visionvalue != 0 && visionvalue != 5) {
                return 0;
            }
        }

        if (0 == laservalue) {

            if (vx >= 0 && vx < vwidth && vy >= 0 && vy < vheight) {
                int visionvalue =
                    astarPlanner.m_pvisionGridState[vy * vwidth + vx]
                        .CurrentState;
                if (visionvalue == 0) {
                    return 16;
                } else {
                    return 10;
                }
            }
            return 16;
        }
        if (5 == laservalue) {

            if (vx >= 0 && vx < vwidth && vy >= 0 && vy < vheight) {
                int visionvalue =
                    astarPlanner.m_pvisionGridState[vy * vwidth + vx]
                        .CurrentState;
                if (visionvalue == 5) {
                    return 4;
                }
            }
            return 6;

        } else {
            return 0;
        }
    }

    if (vx >= 0 && vx < vwidth && vy >= 0 && vy < vheight) {
        int visionvalue =
            astarPlanner.m_pvisionGridState[vy * vwidth + vx].CurrentState;
        if (visionvalue == 0) {
            return 3;
        }
        if (visionvalue == 5) {

            return 1;
        }
        return 0;
    }
}

double CNaviInterface::doArrive(vector<double> &p) {

    double metersperpix = m_pms->astarMap.metersPerPixel;
    // astarmap
    int ix = (p[0] - m_pms->astarMap.x0) / metersperpix;
    int iy = (p[1] - m_pms->astarMap.y0) / metersperpix;

    int GMapWidth = m_pms->astarMap.width;
    int GMapHeight = m_pms->astarMap.height;
    // lasermap
    int lx = (p[0] - m_pms->laserMap.x0) / metersperpix;
    int ly = (p[1] - m_pms->laserMap.y0) / metersperpix;

    int width = m_pms->laserMap.width;
    int height = m_pms->laserMap.height;
    // visionmap
    int vx = (p[0] - m_pms->visionMap.x0) / metersperpix;
    int vy = (p[1] - m_pms->visionMap.y0) / metersperpix;

    int vmapwidth = m_pms->visionMap.width;
    int vmapheight = m_pms->visionMap.height;

    int dx = lx - ix;
    int dy = ly - iy;

    int ddx = vx - ix;
    int ddy = vy - iy;

    int buffer = 0.15 / metersperpix;

    for (int i = ix - buffer; i <= ix + buffer; i++) {
        for (int j = iy - buffer; j <= iy + buffer; j++) {
            if (i >= 0 && i < GMapWidth && j >= 0 && j < GMapHeight) {
                int gmapvalue =
                    astarPlanner.m_pGridState[j * GMapWidth + i].CurrentState;
                if (gmapvalue != 0 && gmapvalue != 3 && gmapvalue != 5) {
                    printf("Blocked on A* map at (i=%d, j=%d), value=%d\n", i, j, gmapvalue);fflush(stdout);
                    for (int z = i - 3; z <= i + 2; z++) {
                        for (int x = j - 3; x <= j + 2; x++) {
                            printf("Grid (%d, %d) = %d\n", z, x, astarPlanner.m_pGridState[x * GMapWidth + z].CurrentState);fflush(stdout);
                        }
                    }                    
                    return 0;
                } else {
                    int lasermapx = i + dx;
                    int lasermapy = j + dy;
                    if (lasermapx >= 0 && lasermapx < width && lasermapy >= 0 &&
                        lasermapy < height) {
                        int laservalue =
                            astarPlanner
                                .m_plaserGridState[lasermapy * width +
                                                   lasermapx]
                                .CurrentState;
                        if (laservalue != 0 && laservalue != 3 && laservalue != 5) {
                            printf("Blocked on Laser map at (x=%d, y=%d), value=%d\n", lasermapx, lasermapy, laservalue);fflush(stdout);
                            return 0;
                        }
                    }

                    int vmapx = i + ddx;
                    int vmapy = j + ddy;
                    if (vmapx >= 0 && vmapx < vmapwidth && vmapy >= 0 &&
                        vmapy < vmapheight) {
                        int visionvalue =
                            astarPlanner
                                .m_pvisionGridState[vmapy * vmapwidth + vmapx]
                                .CurrentState;
                        if (visionvalue != 0 && visionvalue != 5) {
                            printf("Blocked on Vision map at (x=%d, y=%d), value=%d\n", vmapx, vmapy, visionvalue);fflush(stdout);
                            return 0;
                        }
                    }
                }
            } else {
                printf("Out of bounds: (i=%d, j=%d), GMapWidth=%d, GMapHeight=%d\n", i, j, GMapWidth, GMapHeight);fflush(stdout);
                return 0;
            }
        }
    }

    return 1;
}

double CNaviInterface::doArrivelaseronly(vector<double> &p) {

    double metersperpix = m_pms->astarMap.metersPerPixel;
    // astarmap

    int ix = (p[0] - m_pms->astarMap.x0) / metersperpix;
    int iy = (p[1] - m_pms->astarMap.y0) / metersperpix;

    int GMapWidth = m_pms->astarMap.width;
    int GMapHeight = m_pms->astarMap.height;

    // lasermap
    int lx = (p[0] - m_pms->laserMap.x0) / metersperpix;
    int ly = (p[1] - m_pms->laserMap.y0) / metersperpix;

    int width = m_pms->laserMap.width;
    int height = m_pms->laserMap.height;
    // visionmap
    int vx = (p[0] - m_pms->visionMap.x0) / metersperpix;
    int vy = (p[1] - m_pms->visionMap.y0) / metersperpix;

    int vmapwidth = m_pms->visionMap.width;
    int vmapheight = m_pms->visionMap.height;

    int dx = lx - ix;
    int dy = ly - iy;

    int ddx = vx - ix;
    int ddy = vy - iy;

    int buffer = 0.15 / metersperpix;

    for (int i = ix - buffer; i <= ix + buffer; i++) {
        for (int j = iy - buffer; j <= iy + buffer; j++) {
            if (i >= 0 && i < GMapWidth && j >= 0 && j < GMapHeight) {

                int lasermapx = i + dx;
                int lasermapy = j + dy;
                if (lasermapx >= 0 && lasermapx < width && lasermapy >= 0 &&
                    lasermapy < height) {
                    int laservalue =
                        astarPlanner
                            .m_plaserGridState[lasermapy * width + lasermapx]
                            .CurrentState;
                    if (laservalue != 0 && laservalue != 5) {
                        return 0;
                    }
                }

                int vmapx = i + ddx;
                int vmapy = j + ddy;
                if (vmapx >= 0 && vmapx < vmapwidth && vmapy >= 0 &&
                    vmapy < vmapheight) {
                    int visionvalue =
                        astarPlanner
                            .m_pvisionGridState[vmapy * vmapwidth + vmapx]
                            .CurrentState;
                    if (visionvalue != 0 && visionvalue != 5) {
                        return 0;
                    }
                }

            } else {
                return 0;
            }
        }
    }

    return 1;
}

/*这段代码是在一个名为CNaviInterface的类中定义的一个方法，名为ifrobotsafe。这个方法接受一个vector<double>类型的引用参数p，并返回一个整数。
这个方法的主要目的是检查机器人是否处于安全状态。

首先，它从astarMap对象中获取每像素的米数（metersperpix），然后使用这个值和p向量中的元素来计算ix和iy的值。这些值表示机器人在astarMap中的位置。

然后，它获取astarMap的宽度和高度，并计算出机器人在laserMap中的位置（lx和ly）。接着，它获取laserMap的宽度和高度，并计算出dx和dy，这两个值表示机器人在astarMap
和laserMap之间的位置差。

接下来，它获取visionMap的宽度和高度，并计算出机器人在visionMap中的位置（vx和vy）。

然后，它检查机器人在astarMap中的位置是否在有效范围内。如果在，它会检查该位置的gmapvalue值。如果gmapvalue不等于0且不等于5，它会计算出机器人在laserMap中的位置，
并检查该位置的laservalue值。如果laservalue不等于0且不等于5，它会返回3。否则，它会检查机器人在visionMap中的位置，并检查该位置的visionvalue值。如果visionvalue
不等于0且不等于5，它会打印一条消息并返回3。否则，它会返回0。

如果gmapvalue等于0或等于5，它会执行类似的检查，但在laservalue等于0或等于5时，它会返回1而不是0。

如果机器人在astarMap中的位置不在有效范围内，它会检查机器人在laserMap中的位置。如果在有效范围内，它会检查laservalue值，并根据该值返回3或1。如果不在有效范围内，它会返回0。

总的来说，这个方法通过检查机器人在不同地图中的位置和状态，来判断机器人是否处于安全状态。*/
int CNaviInterface::ifrobotsafe(vector<double> &p) {

    double metersperpix = m_pms->astarMap.metersPerPixel;
    // astarmap
    int ix = (p[0] - m_pms->astarMap.x0) / metersperpix;
    int iy = (p[1] - m_pms->astarMap.y0) / metersperpix;

    int GMapWidth = m_pms->astarMap.width;
    int GMapHeight = m_pms->astarMap.height;
    // lasermap
    int lx = (p[0] - m_pms->laserMap.x0) / metersperpix;
    int ly = (p[1] - m_pms->laserMap.y0) / metersperpix;

    int width = m_pms->laserMap.width;
    int height = m_pms->laserMap.height;

    int dx = lx - ix;
    int dy = ly - iy;

    int vmapwidth = m_pms->visionMap.width;
    int vmapheight = m_pms->visionMap.height;

    int vx = (p[0] - m_pms->visionMap.x0) / metersperpix;
    int vy = (p[1] - m_pms->visionMap.y0) / metersperpix;
    // int buffer = 0.35/metersperpix;

    if (ix >= 0 && ix < GMapWidth && iy >= 0 && iy < GMapHeight) {
        int gmapvalue =
            astarPlanner.m_pGridState[iy * GMapWidth + ix].CurrentState;
            //TODO gmapvalue1
            printf("gmapvalue = %d\n", gmapvalue);fflush(stdout);
        if (gmapvalue != 0 && gmapvalue !=3 && gmapvalue != 5) {
            int lasermapx = ix + dx;
            int lasermapy = iy + dy;
            if (lasermapx >= 0 && lasermapx < width && lasermapy >= 0 &&
                lasermapy < height) {
                int laservalue =
                    astarPlanner
                        .m_plaserGridState[lasermapy * width + lasermapx]
                        .CurrentState;
                // TODO laservalue1
                printf("laservalue = %d\n", laservalue);fflush(stdout);
                /////////////////
                if (laservalue != 0 && laservalue != 5) {
                    // TODO
                    printf("1-1\n"); fflush(stdout);
                    /////////////////
                    return 3;
                } else {
                    //TODO
                    // if (laservalue == 0){
                    //     return 1;
                    // }
                    ////////////////
                    if (vx >= 0 && vx < vmapwidth && vy >= 0 &&
                        vy < vmapheight) {
                        int visionvalue =
                            astarPlanner.m_pvisionGridState[vy * vmapwidth + vx]
                                .CurrentState;
                        if (visionvalue != 0 && visionvalue != 5) {
                            printf("robot in vision expansion area\n");fflush(stdout);
                            return 3;
                        } else {
                            return 0;
                        }
                    }

                    return 0;
                }
            }

            return 0;
        } else {
            int lasermapx = ix + dx;
            int lasermapy = iy + dy;
            if (lasermapx >= 0 && lasermapx < width && lasermapy >= 0 &&
                lasermapy < height) {
                int laservalue =
                    astarPlanner
                        .m_plaserGridState[lasermapy * width + lasermapx]
                        .CurrentState;
                if (laservalue != 0 && laservalue != 5) {
                    printf("2-1");fflush(stdout);
                    return 3;
                } else {
                    if (vx >= 0 && vx < vmapwidth && vy >= 0 &&
                        vy < vmapheight) {
                        int visionvalue =
                            astarPlanner.m_pvisionGridState[vy * vmapwidth + vx]
                                .CurrentState;
                        if (visionvalue != 0 && visionvalue != 5) {
                            printf("robot in vision expansion area\n");fflush(stdout);
                            return 3;
                        } else {
                            return 1;
                        }
                    }
                    return 1;
                }
            }
        }
    } else {
        //TODO 
        printf("2");fflush(stdout);
        if (lx >= 0 && lx < width && ly >= 0 && ly < height) {
            int laservalue =
                astarPlanner.m_plaserGridState[ly * width + lx].CurrentState;
            if (laservalue != 0 && laservalue != 5) {
                printf("3-1");fflush(stdout);
                return 3;
            } else {
                return 1;
            }
        } else {
            return 0;
        }
    }
}

TargetCheckResult CNaviInterface::checkTargetLegal(vector<double> &p, bool includeDynamic) {

    if (m_pms == NULL) {
        printf("target error: map server is null\n");fflush(stdout);
        targetErr = true;
        setLastFullPathError(TARGET_STATIC_INVALID);
        return TARGET_CHECK_STATIC_INVALID;
    }

    double metersperpix = m_pms->astarMap.metersPerPixel;
    // astarmap
    int ix = (p[0] - m_pms->astarMap.x0) / metersperpix;
    int iy = (p[1] - m_pms->astarMap.y0) / metersperpix;

    int GMapWidth = m_pms->astarMap.width;
    int GMapHeight = m_pms->astarMap.height;
    // lasermap
    int lx = (p[0] - m_pms->laserMap.x0) / metersperpix;
    int ly = (p[1] - m_pms->laserMap.y0) / metersperpix;

    int width = m_pms->laserMap.width;
    int height = m_pms->laserMap.height;

    int dx = lx - ix;
    int dy = ly - iy;

    int vx = (p[0] - m_pms->visionMap.x0) / metersperpix;
    int vy = (p[1] - m_pms->visionMap.y0) / metersperpix;

    int vmapwidth = m_pms->visionMap.width;
    int vmapheight = m_pms->visionMap.height;

    // int buffer = 0.35/metersperpix;

    printf("ix = %d, iy = %d, GMapWidth = %d, GMapHeight = %d\n", ix, iy,
           GMapWidth, GMapHeight);fflush(stdout);
    if (ix >= 0 && ix < GMapWidth && iy >= 0 && iy < GMapHeight) {
        int gmapvalue =
            astarPlanner.m_pGridState[iy * GMapWidth + ix].CurrentState;
        if (gmapvalue != 0 && gmapvalue != 5) {
            printf("target error in astarmap\n");fflush(stdout);
            targetErr = true;
            setLastFullPathError(TARGET_STATIC_INVALID);
            return TARGET_CHECK_STATIC_INVALID;
        } else {
            if (!includeDynamic) {
                targetErr = false;
                setLastFullPathError(FULLPATH_OK);
                return TARGET_CHECK_OK;
            }

            int lasermapx = ix + dx;
            int lasermapy = iy + dy;
            if (lasermapx >= 0 && lasermapx < width && lasermapy >= 0 &&
                lasermapy < height) {
                int laservalue =
                    astarPlanner
                        .m_plaserGridState[lasermapy * width + lasermapx]
                        .CurrentState;
                if (laservalue != 0 && laservalue != 5) {
                    printf("target dynamic blocked in lasermap%d\n", laservalue);fflush(stdout);
                    targetErr = false;
                    setLastFullPathError(TARGET_DYNAMIC_BLOCKED_LASER);
                    return TARGET_CHECK_DYNAMIC_BLOCKED_LASER;
                } else {
                    if (vx >= 0 && vx < vmapwidth && vy >= 0 &&
                        vy < vmapheight) {
                        int visionvalue =
                            astarPlanner.m_pvisionGridState[vy * vmapwidth + vx]
                                .CurrentState;
                        if (visionvalue != 0 && visionvalue != 5) {
                            printf("target dynamic blocked in visionmap\n");fflush(stdout);
                            targetErr = false;
                            setLastFullPathError(TARGET_DYNAMIC_BLOCKED_VISION);
                            return TARGET_CHECK_DYNAMIC_BLOCKED_VISION;
                        } else {
                            targetErr = false;
                            setLastFullPathError(FULLPATH_OK);
                            return TARGET_CHECK_OK;
                        }
                    }
                    targetErr = false;
                    setLastFullPathError(FULLPATH_OK);
                    return TARGET_CHECK_OK;
                }
            }

            targetErr = false;
            setLastFullPathError(FULLPATH_OK);
            return TARGET_CHECK_OK;
        }
    } else {
        printf("target error out of astarmap\n");fflush(stdout);
        targetErr = true;
        setLastFullPathError(TARGET_STATIC_INVALID);
        return TARGET_CHECK_STATIC_INVALID;
    }
}

bool CNaviInterface::iftargetlegal(vector<double> &p) {
    return checkTargetLegal(p, true) == TARGET_CHECK_OK;
}

bool CNaviInterface::iftargetlegalStatic(vector<double> &p) {
    return checkTargetLegal(p, false) == TARGET_CHECK_OK;
}

int CNaviInterface::DWAplanlaseronly(vector<double> &goalvw, int &flag) {
    flag = 0;
    double MaxV = 0.1 * m_ucspeedlever;
    double MaxW = PI / 3;
    double MinW = -PI / 3;

    double curx = 0.0;
    double cury = 0.0;
    double curtheta = 0.0;
    pthread_mutex_lock(&m_csPose_mutex);

    curx = m_CurPos.x;
    cury = m_CurPos.y;
    curtheta = m_CurPos.theta;

    pthread_mutex_unlock(&m_csPose_mutex);

    double endposex;
    double endposey;

    int pnum = path.size();
    if (0 < pnum) {
        endposex = path[pnum - 1].at(0);
        endposey = path[pnum - 1].at(1);
    } else {
        printf("illegal path for dwa !\n");fflush(stdout);
        return 0;
    }

    double cur2enddist = sqrt((endposex - curx) * (endposex - curx) +
                              (endposey - cury) * (endposey - cury));

    if (1.6 >= cur2enddist) {
        vector<double> pathendpose = path[pnum - 1];

        double doarrive = doArrivelaseronly(pathendpose);
        if (0 == doarrive) {
            printf("can not arrive !\n");fflush(stdout);
            targetErr = false;
            setLastFullPathError(TARGET_DYNAMIC_BLOCKED_LASER);
            goalvw.push_back(0);
            goalvw.push_back(0);
            flag = 1;
            return 0;
        }
    }
    if (1 >= cur2enddist) {
        MaxV = 0.2;
        MaxW = PI / 4;
        MinW = -PI / 4;
    }

    double curv = Vcur;
    double curw = Wcur;
    double Vmax = min(Vcur + 0.08, MaxV);
    double Vmin = max(Vcur - 0.10, 0);

    if (0.3 <= Vmin) {
        Vmin = 0.2;
    }

    if (0.10 >= Vmax) {
        Vmax = 0.3;
    }

    if (Vmin >= Vmax) {
        Vmin = max(Vmax - 0.1, 0);
    }

    double Wmax = min(Wcur + PI / 3, MaxW); // PI/5   !!!!!!!!!!!!!!!!!!!!!!
    double Wmin = max(Wcur - PI / 3, MinW);
    if (Wmin >= Wmax) {
        Wmin = Wmax - PI / 4;
    }
    double Vstep = 0.02;
    double Wstep = 0.02;

    int numv = (Vmax - Vmin) / Vstep;
    int numw = (Wmax - Wmin) / Wstep;

    double v = 0;
    double w = 0;

    Pose subpos;

    double s = sin(m_CurPos.theta);
    double c = cos(m_CurPos.theta);

    vector<vector<Pose>> tempendpose;
    vector<Pose> tempsubpose;
    vector<Pose> endpose;
    vector<vector<double>> tempvw;
    vector<vector<double>> endvw;
    vector<double> goalthetascore;
    vector<double> goaldistscore;
    vector<double> goalthetascore_1;
    vector<double> goaldistscore_1;
    vector<double> savescore;
    vector<double> vibrascore;
    vector<double> speedscore;
    vector<double> goal;
    vector<double> goal_1;
    vector<double> goal_follow;

    if (ordi >= 0) {
        double x = path[ordi].at(0);
        double y = path[ordi].at(1);
        double dist = sqrt((x - curx) * (x - curx) + (y - cury) * (y - cury));
        if (1.3 < dist) // 0.75
        {
            m_ifastar = 1;
            goalvw.push_back(0);
            goalvw.push_back(0);
            flag = 0;
            return 0;
        }
    }

    if (ordi_1 >= 0) {
        double x = path[ordi_1].at(0);
        double y = path[ordi_1].at(1);
        double dist = sqrt((x - curx) * (x - curx) + (y - cury) * (y - cury));
        if (1.3 < dist) // 0.85
        {
            m_ifastar = 1;
            goalvw.push_back(0);
            goalvw.push_back(0);
            flag = 0;
            return 0;
        }
    }

    for (int i = ordi; i < path.size(); i++) {
        double x = path[i].at(0);
        double y = path[i].at(1);
        double dist = sqrt((x - curx) * (x - curx) + (y - cury) * (y - cury));

        ordi = i;

        if (0.5 <= dist) // 0.7
        {
            goal.push_back(path[i].at(0));
            goal.push_back(path[i].at(1));
            break;
        }
    }

    for (int i = ordi_1; i < path.size(); i++) {
        double x = path[i].at(0);
        double y = path[i].at(1);
        double dist = sqrt((x - curx) * (x - curx) + (y - cury) * (y - cury));

        ordi_1 = i;

        if (0.5 <= dist) // 0.8
        {
            goal_1.push_back(path[i].at(0));
            goal_1.push_back(path[i].at(1));
            break;
        }
    }

    for (int i = 0; i < 27; i++) {
        for (int j = testpath[i]; j < path.size(); j++) {
            double x = path[j].at(0);
            double y = path[j].at(1);
            double dist =
                sqrt((x - curx) * (x - curx) + (y - cury) * (y - cury));
            testpath[i] = j;

            if ((0.25 + i * 0.05) <= dist) {
                break;
            }
        }
        Pose tempforsafepose;
        tempforsafepose.x = path.at(testpath[i]).at(0);
        tempforsafepose.y = path.at(testpath[i]).at(1);
        tempforsafepose.theta = 0;
        double pscore = bifsavelaseronly(tempforsafepose);
        if (0 == pscore) {
            m_ifastar = 1;

            goalvw.push_back(0);
            goalvw.push_back(0);
            m_pLogCbFunc(2);
            printf("path changed !\n");fflush(stdout);
            return 0;
        }

        if (testpath[i] == path.size() - 1) {
            break;
        }
    }

    if ((ordi + 1) == path.size()) {
        goal.clear();
        goal.push_back(path[ordi].at(0));
        goal.push_back(path[ordi].at(1));
    }

    if ((ordi_1 + 1) == path.size()) {
        goal_1.clear();
        goal_1.push_back(path[ordi_1].at(0));
        goal_1.push_back(path[ordi_1].at(1));
    }

    Pose robot;

    robot.x = curx;
    robot.y = cury;
    robot.theta = curtheta;
    if (1 == istartgo) {
        double dtheta = 0.0;
        dtheta = DetaTheta(robot, goal);
        double goalDist =
            sqrt((goal.at(0) - robot.x) * (goal.at(0) - robot.x) +
                 (goal.at(1) - robot.y) * (goal.at(1) - robot.y));
        if (goalDist < 0.08 || (0.08 >= dtheta && -0.08 <= dtheta)) {
            goalvw.push_back(0.0);
            goalvw.push_back(0.0);

            istartgo = 0;
            return 1;
        }
        if (0.35 < dtheta) {
            goalvw.push_back(0.0);
            goalvw.push_back(PI / 8);
            return 1;
        }
        if (0.35 >= dtheta && 0.08 < dtheta) {
            goalvw.push_back(0.0);
            goalvw.push_back(PI / 16);
            return 1;
        }
        if (-0.35 > dtheta) {
            goalvw.push_back(0.0);
            goalvw.push_back(-PI / 8);
            return 1;
        }
        if (-0.35 <= dtheta && -0.08 > dtheta) {
            goalvw.push_back(0.0);
            goalvw.push_back(-PI / 16);
            return 1;
        }

        return 1;
    }

    for (int i = 0; i <= numv; i++) {
        v = Vmin + i * Vstep;

        for (int j = 0; j <= numw; j++) {
            w = Wmin + j * Wstep;
            tempsubpose.clear();
            if (w == 0) {
                for (int j = 2; j <= 24; j = j + 2) {
                    double i = (double)(j / 10);
                    subpos.x = curx + v * i * c;
                    subpos.y = cury + v * i * s;
                    subpos.theta = curtheta;
                    tempsubpose.push_back(subpos);
                }
                tempendpose.push_back(tempsubpose);
                tempsubpose.clear();
                vector<double> vw;
                vw.push_back(v);
                vw.push_back(w);
                tempvw.push_back(vw);

            } else {
                for (int j = 2; j <= 24; j = j + 2) {
                    double i = (double)(j / 10);
                    subpos.x =
                        curx - (v / w) * s + (v / w) * sin(curtheta + w * i);
                    subpos.y =
                        cury + (v / w) * c - (v / w) * cos(curtheta + w * i);
                    subpos.theta = curtheta + w * i;
                    tempsubpose.push_back(subpos);
                }
                tempendpose.push_back(tempsubpose);
                tempsubpose.clear();
                vector<double> vw;
                vw.push_back(v);
                vw.push_back(w);
                tempvw.push_back(vw);
            }
        }
    }

    int num = tempendpose.size();

    double sumthetascore = 0.0;
    double sumdistscore = 0.0;
    double sumthetascore_1 = 0.0;
    double sumdistscore_1 = 0.0;
    double sumsavescore = 0.0;
    double sumvibrascore = 0.0;
    double sumspeedscore = 0.0;

    int endnum = 0;

    for (int i = 0; i < num; i++) {
        vector<Pose> p = tempendpose[i];

        double isave = difsafelaseronly(p);

        if (0 < isave) {
            endvw.push_back(tempvw[i]);
            double thetascore = ThetaScore(p[4], goal);
            double distscore = DistScore(p[4], goal, 0);

            double thetascore_1 = ThetaScore(p[4], goal_1);
            double distscore_1 = DistScore(p[4], goal_1, 1);

            //算总分
            sumthetascore += thetascore;
            sumdistscore += distscore;
            sumsavescore += isave;

            sumthetascore_1 += thetascore_1;
            sumdistscore_1 += distscore_1;

            if (1 <= ifirstpublish) {
                double difvibra = 0.0;
                difvibra = tempvw[i].at(1) * dvibration;
                if (0.0 <= difvibra) {
                    vibrascore.push_back(2);
                    sumvibrascore += 2;
                } else {
                    vibrascore.push_back(1);
                    sumvibrascore += 1;
                }
            } else {
                vibrascore.push_back(1);
            }

            //各个分压栈
            goalthetascore.push_back(thetascore);
            goaldistscore.push_back(distscore);
            savescore.push_back(isave);

            goalthetascore_1.push_back(thetascore_1);
            goaldistscore_1.push_back(distscore_1);

            endnum++;
        }
    }

    double bestscore = 0.0;
    double a = 3.8; // theta
    double b = 2.0; // dist
    double d = 2.0; // safe
    double e = 0.2; // vib  0.5

    double f = 3.4; // theta 1.2 !!!!!!!!!!!!!!
    double g = 2.4; // dist 2.0
    int bestid = 0;

    if (1 > endnum) {
        printf("endnum = %d\n", endnum);fflush(stdout);
        goalvw.push_back(0);
        goalvw.push_back(0);
        m_ifastar = 1;
        flag = 0;
        m_pLogCbFunc(3);
        return 0;
    }

    if (sumthetascore == 0 || 0 == sumdistscore) {
        goalvw.push_back(0);
        goalvw.push_back(0);

        m_ifastar = 1;
        return 0;
    }

    for (int i = 0; i < endnum; i++) {

        //各个分算比例
        goalthetascore[i] = goalthetascore[i] / sumthetascore;
        goaldistscore[i] = goaldistscore[i] / sumdistscore;
        savescore[i] = savescore[i] / sumsavescore;

        goalthetascore_1[i] = goalthetascore_1[i] / sumthetascore_1;
        goaldistscore_1[i] = goaldistscore_1[i] / sumdistscore_1;

        if (1 <= ifirstpublish) {
            vibrascore[i] = vibrascore[i] / sumvibrascore;
        } else {
            vibrascore[i] = 0;
        }

        double tempbestscore = 0.0;
        tempbestscore = a * goalthetascore[i] + b * goaldistscore[i] +
                        d * savescore[i] + e * vibrascore[i] +
                        f * goalthetascore_1[i] + g * goaldistscore_1[i];

        //找到最高分
        if (bestscore < tempbestscore) {
            bestscore = tempbestscore;
            bestid = i;
        }
    }
    ifirstpublish = 1;

    //最高分对应的v,w
    goalvw = endvw[bestid];
    if (0 <= goalvw[1]) {
        dvibration = 1;
    } else {
        dvibration = -1;
    }

    flag = 0;
    m_ifastar = 0;
    return 1;
}

double CNaviInterface::max(double a, double b) {
    if (a >= b) {
        return a;
    } else {
        return b;
    }
}
double CNaviInterface::min(double a, double b) {
    if (a >= b) {
        return b;
    } else {
        return a;
    }
}

double CNaviInterface::ThetaScore(Pose &From, vector<double> &To) {
#if 1
    double detax = To[0] - From.x;
    double detay = To[1] - From.y;
    double dist2 = detax * detax + detay * detay;
    double dist = sqrt(dist2);
    if (!(dist > 0.05)) {
        return 0.0;
    }

    double as = detay / dist;
    as = ((as > 0.99) ? 0.99 : (as < -0.99) ? -0.99 : as);
    double pptheta = asin(as);

    double detatheta = 0.0;
    double thetascore = 0.0;

    if (0 <= detax) {
        if (0 <= detay) {
            pptheta = pptheta;
        } else {
            pptheta = pptheta;
        }
    } else {
        if (0 <= detay) {
            pptheta = PI - pptheta;

        } else {
            pptheta = (-PI) - pptheta;
        }
    }

    detatheta = From.theta - pptheta;
    detatheta = fabs(detatheta);
    if (PI <= detatheta) {
        detatheta = 2 * PI - detatheta;

        detatheta = fabs(detatheta);
    }

    thetascore = MathUtil::toAngle(detatheta);
    thetascore = 180 - thetascore;

    return thetascore;
#endif
}

double CNaviInterface::DetaTheta(Pose &From, vector<double> &To) {
#if 1
    double detax = To[0] - From.x;
    double detay = To[1] - From.y;
    double dist2 = detax * detax + detay * detay;
    double dist = sqrt(dist2);

    double as = detay / dist;
    as = ((as > 0.99) ? 0.99 : (as < -0.99) ? -0.99 : as);
    double pptheta = asin(as);

    double detatheta = 0.0;
    double thetascore = 0.0;

    if (0 <= detax) {
        if (0 <= detay) {
            pptheta = pptheta;
        } else {
            pptheta = pptheta;
        }
    } else {
        if (0 <= detay) {
            pptheta = PI - pptheta;

        } else {
            pptheta = (-PI) - pptheta;
        }
    }

    detatheta = pptheta - From.theta;

    if (PI <= detatheta) {
        detatheta = detatheta - 2 * PI;
    }

    if (-PI > detatheta) {
        detatheta = detatheta + 2 * PI;
    }

    return detatheta;
#endif
}

double CNaviInterface::DistScore(Pose &p, vector<double> &q, int flag) {
    double x = p.x - q[0];
    double y = p.y - q[1];
    double dis;
    if (0 == flag) {
        dis = 0.9 - sqrt(x * x + y * y); // 0.4
    } else {
        dis = 0.9 - sqrt(x * x + y * y); // 0.7
    }
    return dis;
}

void CNaviInterface::setLocationType(LocationType type) {
    pthread_mutex_lock(&m_csLoc_mutex);
    if (m_eLocationType == SLAM) {
        if (type == SCANMATCH) {

            // try
            search_x_m = 1.0;
            search_y_m = 1.0;
            search_theta_rad = PI / 4;

            m_bRevise = true;
        }
    }
    m_eLocationType = type;
    pthread_mutex_unlock(&m_csLoc_mutex);
}

bool CNaviInterface::createMap(double metersPerPixel, bool forceNewMap) {

    bool hasWaypoints = false;
    pthread_mutex_lock(&m_goal_mutex);
    hasWaypoints = (m_waypoints.size() > 0);
    pthread_mutex_unlock(&m_goal_mutex);

    bool activeNavigation = enableCoverage || m_coverageActive || hasWaypoints ||
                            m_mapState == MAP_STATE_NAVIGATING;
    bool existingMap = defaultPublishedMapExists() ||
                       m_mapState == MAP_STATE_MAP_READY;

    if (m_mapState == MAP_STATE_MAPPING ||
        m_mapState == MAP_STATE_SAVING ||
        (!forceNewMap && activeNavigation)) {
        printf("Ignore create map: state=%d activeNavigation=%d existingMap=%d force=%d\n",
               (int)m_mapState, activeNavigation ? 1 : 0,
               existingMap ? 1 : 0, forceNewMap ? 1 : 0);
        fflush(stdout);
        return false;
    }

    clearGeneratedMapFilesForNewMapping();
    clearNavigationStateAndStop();
    enableCoverage = false;
    resetCoverageState();

    setMapLifecycleState(MAP_STATE_MAPPING);

    m_laser1only = true;
    Pose pos;

    // MUTEX
    pthread_mutex_lock(&m_csLoc_mutex);
    if (m_pscanMatcher != NULL) {
        delete m_pscanMatcher;
        m_pscanMatcher = NULL;
    }
    if (m_pmsSLAM != NULL) {
        delete m_pmsSLAM;
        m_pmsSLAM = NULL;
    }
#if 1
    if (m_pOptimizeMap != NULL) {
        delete m_pOptimizeMap;
        m_pOptimizeMap = NULL;
    }
#endif
    /**********/
    if (m_pmsSLAMtest != NULL) {
        delete m_pmsSLAMtest;
        m_pmsSLAMtest = NULL;
    }
    /***************/
    m_pscanMatcher = new ScanMatcher();
    m_pOptimizeMap = new OptimizeMap();
    m_pscanMatcher->matcher.initWeight();
    m_pscanMatcher->initprobmap();
    // m_pmsSLAM要建图用，MapServer
    // *m_pmsSLAM;metersPerPixel是输入的值，此处调用的是带参数列表的构造函数，不是使用默认的
    // 0.05,一直往上层找这个参数，在最上层，有个0.04-0.16的限制条件，此处如果pad输入0.02，越界使用默认的0.05
    //修改时要注意
    m_pmsSLAM = new MapServer(
        15, metersPerPixel); // 15：range；metersPerPixel：resolution分辨率

    /*********/
    m_pmsSLAMtest = new MapServer(15, metersPerPixel);
    /**************/
    g_count = 0;
    pos.x = 0.0;
    pos.y = 0.0;
    pos.theta = 0.0;

    if (m_pscanMatcher != NULL) {
        m_pscanMatcher->SetXyt(pos); //初始建图，xyt=0,0,0
        m_pscanMatcher->m_poselastxyt = pos;
    }

    m_eNaviType = MANUAL; //——>转到update

    pthread_mutex_unlock(&m_csLoc_mutex);
    return true;
}
void CNaviInterface::initLoc(Pose &pos, Pose &range) //执行底盘定位
{
    pthread_mutex_lock(&m_csLoc_mutex);
    pthread_mutex_lock(&m_initloc_mutex);

    m_eNaviType = LOCALIZATION;
    m_bPoseError = false;
    m_bRevise = false;
    m_laser1only = false;
    m_bfind = false;

    m_ifirstinitloc = 1;

    m_CurPos.x = pos.x;
    m_CurPos.y = pos.y;
    m_CurPos.theta = pos.theta;

    printf("initloc: x=%f,y=%f,t=%f\n", m_CurPos.x, m_CurPos.y, m_CurPos.theta);fflush(stdout);
    search_x_m = range.x;
    search_y_m = range.y;
    search_theta_rad = degrees_to_radians(range.theta);

    /*********/
    m_imode = 0; // refinefunc 无限制
    /***********/
    m_bautocharge = false;
    m_bcreateautochargemap = false;

    m_bIsLoc = true; //置成true后，才能执行updata
    m_bConverged = false;
    m_bslam = false;
    m_eLocationType = SCANMATCH; //选择 SCANMATCH 模式

    m_pmatcher->initWeight();
    printf("initloc,mode = %d\n", m_imode);fflush(stdout);
#if 0
		for (int i = 0;i<81;i++)
		{
			printf("%d : %f\n",i,m_pmatcher->VdHistogramFilterWeight[i]);fflush(stdout);
		}
#endif
    m_poselastxyt.x = pos.x;
    m_poselastxyt.y = pos.y;
    m_poselastxyt.theta = pos.theta;

    // m_poselastxyt.x = 0.8;
    // m_poselastxyt.y = 0.8;
    // m_poselastxyt.theta = 0.52;

    lastupdatepose.x = pos.x;
    lastupdatepose.y = pos.y;
    lastupdatepose.theta = pos.theta;

    lastfixpose.x = pos.x;
    lastfixpose.y = pos.y;
    lastfixpose.theta = pos.theta;

    lastAmapupdatepose.x = pos.x;
    lastAmapupdatepose.y = pos.y;
    lastAmapupdatepose.theta = pos.theta;

    chargemappose.x = pos.x;
    chargemappose.y = pos.y;
    chargemappose.theta = pos.theta;

    lastchargemappose.x = pos.x;
    lastchargemappose.y = pos.y;
    lastchargemappose.theta = pos.theta;

    updateg.nodes.clear();

    srand((unsigned)time(NULL));
    m_bupdatemap = false;
    m_bexpandmap = false;
    m_bmanualupdate = false;

    pthread_mutex_unlock(&m_initloc_mutex);
    pthread_mutex_unlock(&m_csLoc_mutex);
}

void CNaviInterface::particleFilterLoc(Pose pos, Pose range, int particlenum) {

    double region[2][2];
    region[0][0] = pos.x - range.x;
    region[0][1] = pos.y - range.y;
    region[1][0] = pos.x + range.x;
    region[1][1] = pos.y + range.y;
    pthread_mutex_lock(&m_pf_mutex);

    if (m_eLocationType == SCANMATCH) {
        m_eLocationType = PARTICLEFILTER;

        if (m_pPf != NULL) {
            delete m_pPf;
            m_pPf = NULL;
        }

        if (m_pms != NULL) {
            m_pPf = new ParticleFilter(m_pms->globalBinaryMap,
                                       m_pms->globalGaussianMap, region, true,
                                       particlenum);
            m_lastxyt2.x = m_nowxyt.x;
            m_lastxyt2.y = m_nowxyt.y;
            m_lastxyt2.theta = m_nowxyt.theta;
            m_pf_count = 0;
        } else {
            printf("m_pms == null   error\n");fflush(stdout);
        }
    }
    pthread_mutex_unlock(&m_pf_mutex);
}

void CNaviInterface::saveMap(const char *strMapName) {
    bool saved = false;
    setMapLifecycleState(MAP_STATE_SAVING);
    pthread_mutex_lock(&m_csLoc_mutex);
    if (m_eNaviType == MANUAL) {
        drawMap();
        saved = m_pmsSLAM->saveMap(strMapName);
    }
    pthread_mutex_unlock(&m_csLoc_mutex);
    if (saved) {
        setMapLifecycleState(MAP_STATE_MAP_READY);
    } else {
        setMapLifecycleState(MAP_STATE_IDLE);
    }
}

void CNaviInterface::saveModifyMap(void) { m_pms->saveMap_Modify(); }
bool CNaviInterface::saveMapCallBack(void) { return m_pms->saveMap_CallBack(); }
void CNaviInterface::SetSaveMapDone(int status) {
    m_pms->setSaveMapDoneStatus(status);
}
void CNaviInterface::drawMap() {
    if (m_pmsSLAM->globalBinaryMap.data != NULL) {
        delete[] m_pmsSLAM->globalBinaryMap.data;
        m_pmsSLAM->globalBinaryMap.data = NULL;
    }

    m_pmsSLAM->globalBinaryMap.data =
        new BYTE[m_pmsSLAM->globalBinaryMap.width *
                 m_pmsSLAM->globalBinaryMap.height];
    m_pmsSLAM->globalBinaryMap.fill(0);

    int N = m_pscanMatcher->g.nodes.size();

    for (int i = 0; i < N; i++) {

        int prop = i % 1;
        if (0 == prop) {

            GNode gn =
                m_pscanMatcher->g.nodes.at(i); //把g.node向量中的GNode挨个取出来
            vector<Pose> tmp_p;

            gn.getAttribute(
                "points",
                tmp_p); //把当前GNode中机器人位姿下的激光点的机器人坐标系坐标取出来

            vector<vector<Pose>> test_rpoints;
            m_pmsSLAM->contourExtractor.getContours_Pose(tmp_p, test_rpoints);

            for (int i = 0; i < test_rpoints.size(); i++) {
                vector<Pose> contour_points = test_rpoints.at(i);
                if (contour_points.size() <= 1) {
                    continue;
                }

                m_pmsSLAM->addGridMap(
                    m_pmsSLAM->globalBinaryMap, gn.state,
                    contour_points); //激光点转世界坐标
                                     //画线，间隔大于0.3就断开，否则连线，细线
            }
        }
    }
}

bool CNaviInterface::setGoal(Pose &goal) {
    printf("Set goal start!\n");fflush(stdout);
    pthread_mutex_lock(&m_goal_mutex);
    if (m_waypoints.size() >= 1) {
        m_waypoints.pop();
        m_waypoints.push(goal);
        printf("Set goal finished (case 1)!\n");fflush(stdout);
    } else {
        m_waypoints.push(goal);
        printf("Set goal finished (case 2)!\n");fflush(stdout);
    }
    m_ifastar = 1;

    rrset = 0;

    pthread_mutex_unlock(&m_goal_mutex);
    setMapLifecycleState(MAP_STATE_NAVIGATING);
    printf("Set goal success!\n");fflush(stdout);
    printf("m_waypoints is %d\n", m_waypoints.size());fflush(stdout);
    printf("waypoints.x = %lf, waypoints.y = %lf\n", m_waypoints.front().x, m_waypoints.front().y);fflush(stdout);

    return true;
}

void CNaviInterface::clearNavigationStateAndStop() {
    pthread_mutex_lock(&m_goal_mutex);
    while (m_waypoints.size() > 0) {
        m_waypoints.pop();
    }
    m_ifastar = 1;
    m_blaserdwa = false;
    m_blaserastar = false;
    istartgo = 0;
    targetErr = false;
    rrset = 0;
    pthread_mutex_unlock(&m_goal_mutex);

    double pathdot[4] = {0, 0, 0, 0};
    if (m_pPathCbFunc != NULL) {
        m_pPathCbFunc(pathdot, 2, 1);
    }
    if (m_mapState == MAP_STATE_NAVIGATING) {
        setMapLifecycleState(MAP_STATE_MAP_READY);
    }
}

void CNaviInterface::publishZeroVelocity(void) {
    double pathdot[4] = {0, 0, 0, 0};
    if (m_pPathCbFunc != NULL) {
        m_pPathCbFunc(pathdot, 2, 1);
    }
}

bool CNaviInterface::getCurrentGoal(Pose &goal) {
    bool hasGoal = false;
    pthread_mutex_lock(&m_goal_mutex);
    if (m_waypoints.size() > 0) {
        goal = m_waypoints.front();
        hasGoal = true;
    }
    pthread_mutex_unlock(&m_goal_mutex);
    return hasGoal;
}

void CNaviInterface::sendCoopPoseRequest(int seq, int reason) {
    if (coop_lcm == NULL) {
        return;
    }
    Pose goal;
    bool hasGoal = getCurrentGoal(goal);
    double dparams[6] = {m_CurPos.x, m_CurPos.y, m_CurPos.theta, 0, 0, 0};
    if (hasGoal) {
        dparams[3] = goal.x;
        dparams[4] = goal.y;
        dparams[5] = goal.theta;
    }
    int8_t iparams[3] = {(int8_t)robotId, (int8_t)seq, (int8_t)hasGoal};
    robot_control_t cmd;
    cmd.utime = coopNowMs();
    cmd.commandid = COOP_AVOID_CMD_POSE_REQUEST;
    cmd.robotid = (int8_t)(1 - robotId);
    cmd.ndparams = 6;
    cmd.dparams = dparams;
    cmd.niparams = 3;
    cmd.iparams = iparams;
    cmd.nsparams = 0;
    cmd.sparams = NULL;
    cmd.nbparams = 1;
    uint8_t bparams[1] = {(uint8_t)reason};
    cmd.bparams = bparams;
    robot_control_t_publish(coop_lcm, COOP_AVOID_CHANNEL, &cmd);
}

void CNaviInterface::sendCoopStopRequest(int targetRobotId, int seq, int reason) {
    if (coop_lcm == NULL) {
        return;
    }
    Pose goal;
    bool hasGoal = getCurrentGoal(goal);
    double dparams[6] = {m_CurPos.x, m_CurPos.y, m_CurPos.theta, 0, 0, 0};
    if (hasGoal) {
        dparams[3] = goal.x;
        dparams[4] = goal.y;
        dparams[5] = goal.theta;
    }
    int8_t iparams[3] = {(int8_t)robotId, (int8_t)seq, (int8_t)hasGoal};
    uint8_t bparams[1] = {(uint8_t)reason};
    robot_control_t cmd;
    cmd.utime = coopNowMs();
    cmd.commandid = COOP_AVOID_CMD_STOP_REQUEST;
    cmd.robotid = (int8_t)targetRobotId;
    cmd.ndparams = 6;
    cmd.dparams = dparams;
    cmd.niparams = 3;
    cmd.iparams = iparams;
    cmd.nsparams = 0;
    cmd.sparams = NULL;
    cmd.nbparams = 1;
    cmd.bparams = bparams;
    robot_control_t_publish(coop_lcm, COOP_AVOID_CHANNEL, &cmd);
}

void CNaviInterface::sendCoopStopAck(int targetRobotId, int seq) {
    if (coop_lcm == NULL) {
        return;
    }
    int8_t iparams[2] = {(int8_t)robotId, (int8_t)seq};
    uint8_t bparams[1] = {1};
    robot_control_t cmd;
    cmd.utime = coopNowMs();
    cmd.commandid = COOP_AVOID_CMD_STOP_ACK;
    cmd.robotid = (int8_t)targetRobotId;
    cmd.ndparams = 0;
    cmd.dparams = NULL;
    cmd.niparams = 2;
    cmd.iparams = iparams;
    cmd.nsparams = 0;
    cmd.sparams = NULL;
    cmd.nbparams = 1;
    cmd.bparams = bparams;
    robot_control_t_publish(coop_lcm, COOP_AVOID_CHANNEL, &cmd);
}

void CNaviInterface::sendCoopResumeRequest(int targetRobotId, int seq) {
    if (coop_lcm == NULL) {
        return;
    }
    int8_t iparams[2] = {(int8_t)robotId, (int8_t)seq};
    robot_control_t cmd;
    cmd.utime = coopNowMs();
    cmd.commandid = COOP_AVOID_CMD_RESUME_REQUEST;
    cmd.robotid = (int8_t)targetRobotId;
    cmd.ndparams = 0;
    cmd.dparams = NULL;
    cmd.niparams = 2;
    cmd.iparams = iparams;
    cmd.nsparams = 0;
    cmd.sparams = NULL;
    cmd.nbparams = 0;
    cmd.bparams = NULL;
    robot_control_t_publish(coop_lcm, COOP_AVOID_CHANNEL, &cmd);
}

void CNaviInterface::sendCoopResumeAck(int targetRobotId, int seq) {
    if (coop_lcm == NULL) {
        return;
    }
    int8_t iparams[2] = {(int8_t)robotId, (int8_t)seq};
    robot_control_t cmd;
    cmd.utime = coopNowMs();
    cmd.commandid = COOP_AVOID_CMD_RESUME_ACK;
    cmd.robotid = (int8_t)targetRobotId;
    cmd.ndparams = 0;
    cmd.dparams = NULL;
    cmd.niparams = 2;
    cmd.iparams = iparams;
    cmd.nsparams = 0;
    cmd.sparams = NULL;
    cmd.nbparams = 0;
    cmd.bparams = NULL;
    robot_control_t_publish(coop_lcm, COOP_AVOID_CHANNEL, &cmd);
}

void CNaviInterface::respondCoopPoseRequest(int sourceRobotId, int seq) {
    if (coop_lcm == NULL) {
        return;
    }
    Pose goal;
    bool hasGoal = getCurrentGoal(goal);
    double dparams[6] = {m_CurPos.x, m_CurPos.y, m_CurPos.theta, 0, 0, 0};
    if (hasGoal) {
        dparams[3] = goal.x;
        dparams[4] = goal.y;
        dparams[5] = goal.theta;
    }
    int8_t iparams[3] = {(int8_t)robotId, (int8_t)seq, (int8_t)hasGoal};
    robot_control_t cmd;
    cmd.utime = coopNowMs();
    cmd.commandid = COOP_AVOID_CMD_POSE_RESPONSE;
    cmd.robotid = (int8_t)sourceRobotId;
    cmd.ndparams = 6;
    cmd.dparams = dparams;
    cmd.niparams = 3;
    cmd.iparams = iparams;
    cmd.nsparams = 0;
    cmd.sparams = NULL;
    cmd.nbparams = 0;
    cmd.bparams = NULL;
    robot_control_t_publish(coop_lcm, COOP_AVOID_CHANNEL, &cmd);
}

bool CNaviInterface::isStoppedForCoopPeer(void) {
    bool ret;
    pthread_mutex_lock(&m_coop_mutex);
    ret = (m_coopState == COOP_AVOID_STOPPED_FOR_PEER);
    pthread_mutex_unlock(&m_coop_mutex);
    return ret;
}

bool CNaviInterface::isPeerPausedByMe(void) {
    bool ret;
    pthread_mutex_lock(&m_coop_mutex);
    ret = (m_coopState == COOP_AVOID_PEER_PAUSED_BY_ME);
    pthread_mutex_unlock(&m_coop_mutex);
    return ret;
}

void CNaviInterface::markCoverageIndex(int index) {
    pthread_mutex_lock(&m_coop_mutex);
    m_coverageActive = true;
    m_coverageTurnIndex = index;
    pthread_mutex_unlock(&m_coop_mutex);
}

void CNaviInterface::resetCoverageState(void) {
    pthread_mutex_lock(&m_coop_mutex);
    m_coverageActive = false;
    m_coverageTurnIndex = -1;
    pthread_mutex_unlock(&m_coop_mutex);
}

void CNaviInterface::triggerCoopAvoidance(int reason) {
    if (coop_lcm == NULL || robotId < 0 || robotId > 1) {
        return;
    }
    int seq;
    pthread_mutex_lock(&m_coop_mutex);
    if (m_coopState != COOP_AVOID_NORMAL) {
        pthread_mutex_unlock(&m_coop_mutex);
        return;
    }
    m_coopSeq++;
    if (m_coopSeq > 120) {
        m_coopSeq = 1;
    }
    seq = m_coopSeq;
    m_coopActiveSeq = seq;
    m_coopActivePeer = 1 - robotId;
    m_coopTriggerReason = reason;
    m_coopState = COOP_AVOID_WAIT_PEER_POSE;
    m_coopStateTimeMs = coopNowMs();
    m_coopPeerPoseValid = false;
    pthread_mutex_unlock(&m_coop_mutex);

    publishZeroVelocity();
    sendCoopPoseRequest(seq, reason);
}

bool CNaviInterface::pauseForCoopPeer(int sourceRobotId, int seq) {
    Pose savedGoal;
    bool hasGoal = false;
    pthread_mutex_lock(&m_goal_mutex);
    if (m_waypoints.size() > 0) {
        savedGoal = m_waypoints.front();
        hasGoal = true;
    }
    while (m_waypoints.size() > 0) {
        m_waypoints.pop();
    }
    m_ifastar = 1;
    m_blaserdwa = false;
    m_blaserastar = false;
    istartgo = 0;
    targetErr = false;
    rrset = 0;
    pthread_mutex_unlock(&m_goal_mutex);

    pthread_mutex_lock(&m_coop_mutex);
    m_coopSavedGoalValid = hasGoal;
    if (hasGoal) {
        m_coopSavedGoal = savedGoal;
    }
    m_coopSavedCoverageIndex = m_coverageTurnIndex;
    m_coopActivePeer = sourceRobotId;
    m_coopActiveSeq = seq;
    m_coopState = COOP_AVOID_STOPPED_FOR_PEER;
    m_coopStateTimeMs = coopNowMs();
    m_coopPendingStopRequest = false;
    pthread_mutex_unlock(&m_coop_mutex);

    publishZeroVelocity();
    sendCoopStopAck(sourceRobotId, seq);
    return true;
}

void CNaviInterface::resumeAfterCoopPeer(void) {
    Pose savedGoal;
    bool hasGoal;
    pthread_mutex_lock(&m_coop_mutex);
    hasGoal = m_coopSavedGoalValid;
    savedGoal = m_coopSavedGoal;
    m_coopSavedGoalValid = false;
    m_coopState = COOP_AVOID_NORMAL;
    m_coopActivePeer = -1;
    m_coopPendingStopRequest = false;
    pthread_mutex_unlock(&m_coop_mutex);

    if (hasGoal) {
        setGoal(savedGoal);
    }
}

bool CNaviInterface::peerLikelyBlocksCurrentRoute(int reason) {
    Pose peerPose;
    Pose peerGoal;
    bool peerHasGoal;
    bool peerValid;
    pthread_mutex_lock(&m_coop_mutex);
    peerPose = m_coopPeerPose;
    peerGoal = m_coopPeerGoal;
    peerHasGoal = m_coopPeerHasGoal;
    peerValid = m_coopPeerPoseValid;
    pthread_mutex_unlock(&m_coop_mutex);
    if (!peerValid) {
        return false;
    }

    vector<Pose> route;
    if (path.size() >= 2) {
        for (size_t i = 0; i < path.size(); ++i) {
            if (path[i].size() >= 2) {
                Pose p;
                p.x = path[i][0];
                p.y = path[i][1];
                p.theta = 0;
                route.push_back(p);
            }
        }
    }
    if (route.size() < 2) {
        Pose goal;
        if (!getCurrentGoal(goal)) {
            return false;
        }
        route.push_back(m_CurPos);
        route.push_back(goal);
    }

    double safeRadius = m_config.robotconfig.radius + kCoopRobotMargin;
    if (safeRadius < kCoopMinSafeRadius) {
        safeRadius = kCoopMinSafeRadius;
    }

    bool blocksRoute = false;
    for (size_t i = 1; i < route.size(); ++i) {
        if (coopPointToSegmentDistance(peerPose, route[i - 1], route[i]) <= safeRadius) {
            blocksRoute = true;
            break;
        }
        if (peerHasGoal &&
            coopSegmentDistance(route[i - 1], route[i], peerPose, peerGoal) <= safeRadius) {
            blocksRoute = true;
            break;
        }
    }
    if (!blocksRoute) {
        return false;
    }
    if (reason != COOP_AVOID_TRIGGER_MATCH_JUMP) {
        return true;
    }

    vector<Pose> evidence = m_VisionWallDataCopy;
    evidence.insert(evidence.end(), m_CollisionWallDataCopy.begin(), m_CollisionWallDataCopy.end());
    double evidenceRadius = safeRadius + kCoopLaserEvidenceMargin;
    for (size_t i = 0; i < evidence.size(); ++i) {
        if (LinAlg::DistancePose(evidence[i], peerPose) <= evidenceRadius) {
            return true;
        }
        if (peerHasGoal && coopPointToSegmentDistance(evidence[i], peerPose, peerGoal) <= evidenceRadius) {
            return true;
        }
    }
    return false;
}

void CNaviInterface::updateCoopAvoidance(void) {
    long now = coopNowMs();
    int resumeTarget = -1;
    int resumeSeq = 0;
    bool shouldCheckArrival = false;

    pthread_mutex_lock(&m_coop_mutex);
    if ((m_coopState == COOP_AVOID_WAIT_PEER_POSE ||
         m_coopState == COOP_AVOID_WAIT_STOP_ACK) &&
        now - m_coopStateTimeMs > kCoopMessageTimeoutMs) {
        printf("COOP_AVOID timeout in state %d\n", m_coopState);
        fflush(stdout);
        m_coopState = COOP_AVOID_NORMAL;
        m_coopActivePeer = -1;
        m_coopPendingStopRequest = false;
    }
    if (m_coopState == COOP_AVOID_PEER_PAUSED_BY_ME) {
        shouldCheckArrival = true;
        resumeTarget = m_coopActivePeer;
        resumeSeq = m_coopActiveSeq;
    }
    pthread_mutex_unlock(&m_coop_mutex);

    if (shouldCheckArrival) {
        Pose goal;
        if (!getCurrentGoal(goal)) {
            sendCoopResumeRequest(resumeTarget, resumeSeq);
            pthread_mutex_lock(&m_coop_mutex);
            if (m_coopState == COOP_AVOID_PEER_PAUSED_BY_ME &&
                m_coopActivePeer == resumeTarget) {
                m_coopState = COOP_AVOID_NORMAL;
                m_coopActivePeer = -1;
            }
            pthread_mutex_unlock(&m_coop_mutex);
        }
    }
}

void CNaviInterface::handleCoopAvoidMessage(int commandId, int targetRobotId,
                                            int sourceRobotId, int seq,
                                            const double *dparams, int ndparams,
                                            const int8_t *iparams, int niparams,
                                            const uint8_t *bparams, int nbparams) {
    if (targetRobotId != robotId || sourceRobotId == robotId) {
        return;
    }
    switch (commandId) {
    case COOP_AVOID_CMD_POSE_REQUEST:
        respondCoopPoseRequest(sourceRobotId, seq);
        break;
    case COOP_AVOID_CMD_POSE_RESPONSE:
    {
        if (ndparams < 3) {
            break;
        }
        bool matched = false;
        int reason = 0;
        bool pendingStop = false;
        int pendingSource = -1;
        int pendingSeq = 0;
        pthread_mutex_lock(&m_coop_mutex);
        if (m_coopState == COOP_AVOID_WAIT_PEER_POSE &&
            m_coopActiveSeq == seq &&
            m_coopActivePeer == sourceRobotId) {
            m_coopPeerPose.x = dparams[0];
            m_coopPeerPose.y = dparams[1];
            m_coopPeerPose.theta = dparams[2];
            m_coopPeerHasGoal = (niparams >= 3 && iparams[2] != 0 && ndparams >= 6);
            if (m_coopPeerHasGoal) {
                m_coopPeerGoal.x = dparams[3];
                m_coopPeerGoal.y = dparams[4];
                m_coopPeerGoal.theta = dparams[5];
            }
            m_coopPeerPoseValid = true;
            reason = m_coopTriggerReason;
            pendingStop = m_coopPendingStopRequest;
            pendingSource = m_coopPendingStopSource;
            pendingSeq = m_coopPendingStopSeq;
            matched = true;
        }
        pthread_mutex_unlock(&m_coop_mutex);
        if (!matched) {
            break;
        }
        if (pendingStop && robotId == 0) {
            pauseForCoopPeer(pendingSource, pendingSeq);
            break;
        }
        if (peerLikelyBlocksCurrentRoute(reason)) {
            pthread_mutex_lock(&m_coop_mutex);
            m_coopState = COOP_AVOID_WAIT_STOP_ACK;
            m_coopStateTimeMs = coopNowMs();
            pthread_mutex_unlock(&m_coop_mutex);
            sendCoopStopRequest(sourceRobotId, seq, reason);
        } else {
            pthread_mutex_lock(&m_coop_mutex);
            if (m_coopState == COOP_AVOID_WAIT_PEER_POSE &&
                m_coopActiveSeq == seq) {
                m_coopState = COOP_AVOID_NORMAL;
                m_coopActivePeer = -1;
            }
            pthread_mutex_unlock(&m_coop_mutex);
        }
        break;
    }
    case COOP_AVOID_CMD_STOP_REQUEST:
    {
        bool shouldPause = true;
        pthread_mutex_lock(&m_coop_mutex);
        if ((m_coopState == COOP_AVOID_WAIT_PEER_POSE ||
             m_coopState == COOP_AVOID_WAIT_STOP_ACK) && robotId == 1) {
            m_coopPendingStopRequest = true;
            m_coopPendingStopSource = sourceRobotId;
            m_coopPendingStopSeq = seq;
            shouldPause = false;
        }
        pthread_mutex_unlock(&m_coop_mutex);
        if (shouldPause) {
            pauseForCoopPeer(sourceRobotId, seq);
        }
        break;
    }
    case COOP_AVOID_CMD_STOP_ACK:
        pthread_mutex_lock(&m_coop_mutex);
        if (m_coopState == COOP_AVOID_WAIT_STOP_ACK &&
            m_coopActiveSeq == seq &&
            m_coopActivePeer == sourceRobotId) {
            m_coopState = COOP_AVOID_PEER_PAUSED_BY_ME;
            m_coopStateTimeMs = coopNowMs();
        }
        pthread_mutex_unlock(&m_coop_mutex);
        break;
    case COOP_AVOID_CMD_RESUME_REQUEST:
        resumeAfterCoopPeer();
        sendCoopResumeAck(sourceRobotId, seq);
        break;
    case COOP_AVOID_CMD_RESUME_ACK:
        pthread_mutex_lock(&m_coop_mutex);
        if (m_coopState == COOP_AVOID_PEER_PAUSED_BY_ME &&
            m_coopActivePeer == sourceRobotId) {
            m_coopState = COOP_AVOID_NORMAL;
            m_coopActivePeer = -1;
        }
        pthread_mutex_unlock(&m_coop_mutex);
        break;
    default:
        break;
    }
}

void CNaviInterface::deleteGoal() {
    pthread_mutex_lock(&m_goal_mutex);

    if (m_waypoints.size() >= 1) {

        m_waypoints.pop();
        //TODO  delete goal
        printf("after delete goal\n");fflush(stdout);
        ///////////////////////////////
    }
    m_blaserdwa = false;
    m_blaserastar = false;
    m_ifastar = 1;
    pthread_mutex_unlock(&m_goal_mutex);

    if (m_pms != NULL) {
        if (m_pms->visionMap.data != NULL) {
            m_pms->visionMap.fill(0);
        }
        if (0 == nopathflag) {
            vpose.clear();
        }
        cpose.clear();
    }
    if (m_mapState == MAP_STATE_NAVIGATING) {
        setMapLifecycleState(MAP_STATE_MAP_READY);
    }
}

void CNaviInterface::createProbMap(const char *fileName) {

    bool saved = false;
    pthread_mutex_lock(&m_csLoc_mutex);
    if (m_eNaviType == MANUAL) {
        setMapLifecycleState(MAP_STATE_SAVING);

        if (m_pOptimizeMap != NULL) {

            delete m_pOptimizeMap;

            m_pOptimizeMap = NULL;
        }
        m_pOptimizeMap = new OptimizeMap();
        if (m_pOptimizeMap->OpMapServer->globalBinaryMap.data != NULL) {
            delete[] m_pOptimizeMap->OpMapServer->globalBinaryMap.data;
            m_pOptimizeMap->OpMapServer->globalBinaryMap.data = NULL;
        }

        //将最终的二值地图复制过来并清零，保留下来的是原地图的大小信息
        m_pOptimizeMap->OpMapServer->globalBinaryMap.x0 =
            m_pmsSLAM->globalBinaryMap.x0;
        m_pOptimizeMap->OpMapServer->globalBinaryMap.y0 =
            m_pmsSLAM->globalBinaryMap.y0;
        m_pOptimizeMap->OpMapServer->globalBinaryMap.metersPerPixel =
            m_pmsSLAM->globalBinaryMap.metersPerPixel;
        m_pOptimizeMap->OpMapServer->globalBinaryMap.width =
            m_pmsSLAM->globalBinaryMap.width;
        m_pOptimizeMap->OpMapServer->globalBinaryMap.height =
            m_pmsSLAM->globalBinaryMap.height;
        m_pOptimizeMap->OpMapServer->globalBinaryMap.defaultFill =
            m_pmsSLAM->globalBinaryMap.defaultFill;
        m_pOptimizeMap->OpMapServer->globalBinaryMap.data =
            new BYTE[m_pOptimizeMap->OpMapServer->globalBinaryMap.width *
                     m_pOptimizeMap->OpMapServer->globalBinaryMap.height];
        m_pOptimizeMap->OpMapServer->globalBinaryMap.fill(0);

        //将所有帧复制过来，单独处理，不影响原始的数据
        int num = m_pscanMatcher->g.nodes.size();
        for (int i = 0; i < num; i++) {
            GNode gn = m_pscanMatcher->g.nodes.at(i);
            m_pOptimizeMap->OpScanMatcher->g.nodes.push_back(gn);
        }

        //最后，所有校正的位姿重新画图
        int N = m_pOptimizeMap->OpScanMatcher->g.nodes.size();

        m_pOptimizeMap->OpMapServer->globalProbMap.makePixels(
            m_pOptimizeMap->OpMapServer->globalBinaryMap.x0,
            m_pOptimizeMap->OpMapServer->globalBinaryMap.y0,
            m_pOptimizeMap->OpMapServer->globalBinaryMap.width,
            m_pOptimizeMap->OpMapServer->globalBinaryMap.height,
            m_pOptimizeMap->OpMapServer->globalBinaryMap.metersPerPixel, 0.5,
            false);

        for (int i = 0; i < N; i++) {

            int prop = i % 1;
            if (0 == prop) {

                GNode gn = m_pOptimizeMap->OpScanMatcher->g.nodes.at(
                    i); //把g.node向量中的GNode挨个取出来
                vector<Pose> tmp_p;

                gn.getAttribute(
                    "points",
                    tmp_p); //把当前GNode中机器人位姿下的激光点的机器人坐标系坐标取出来

                vector<vector<Pose>> test_rpoints;
                m_pOptimizeMap->OpMapServer->contourExtractor.getContours_Pose(
                    tmp_p, test_rpoints);

                for (int i = 0; i < test_rpoints.size(); i++) {
                    vector<Pose> contour_points = test_rpoints.at(i);
                    if (contour_points.size() <= 1) {
                        continue;
                    }

                    //激光点转世界坐标
                    //画线，间隔大于0.3就断开，否则连线，细线
                    m_pOptimizeMap->OpMapServer->addProbMap(
                        m_pOptimizeMap->OpMapServer->globalProbMap, gn.state,
                        contour_points);
                }
            }
        }

        saved = m_pOptimizeMap->OpMapServer->saveProbMap(fileName);

        printf("***save ProbMap***\n");fflush(stdout);
    }
    pthread_mutex_unlock(&m_csLoc_mutex);
    if (saved) {
        setMapLifecycleState(MAP_STATE_MAP_READY);
    } else if (m_mapState == MAP_STATE_SAVING) {
        setMapLifecycleState(MAP_STATE_IDLE);
    }

    return;
}

void CNaviInterface::updateProbMap(const char *fileName) {
    if (m_pms == NULL) {
        printf("do nothing !\n");fflush(stdout);
        return;
    }
    int N = updateg.nodes.size();
    for (int i = 0; i < N; i++) {

        int prop = i % 1;
        if (0 == prop) {

            GNode gn = updateg.nodes.at(i); //把g.node向量中的GNode挨个取出来
            vector<Pose> tmp_p;

            gn.getAttribute(
                "points",
                tmp_p); //把当前GNode中机器人位姿下的激光点的机器人坐标系坐标取出来

            vector<vector<Pose>> test_rpoints;
            m_pms->contourExtractor.getContours_Pose(tmp_p, test_rpoints);

            for (int i = 0; i < test_rpoints.size(); i++) {
                vector<Pose> contour_points = test_rpoints.at(i);
                if (contour_points.size() <= 1) {
                    continue;
                }

                //激光点转世界坐标
                //画线，间隔大于0.3就断开，否则连线，细线
                m_pms->addProbMap(m_pms->globalProbMap, gn.state,
                                  contour_points);
            }
        }
    }
    updateg.nodes.clear();

    m_pms->saveProbMap(fileName);

    lastupdatepose.x = 0;
    lastupdatepose.y = 0;
    lastupdatepose.theta = 0;

    printf("update probmap done !\n");fflush(stdout);
    return;
}

void CNaviInterface::RegisterGetPosCallback(PGetPosCallBackFunc pPosFunc) {
    m_pPosCbFunc = pPosFunc;
}
void CNaviInterface::RegisterGetPathCallback(PGetPathCallBackFunc pPathFunc) {

    m_pPathCbFunc = pPathFunc;
}
void CNaviInterface::RegisterNoPathCallback(PHaveNoPathCallBackFunc pFunc) {

    m_pNoPathCbFunc = pFunc;
}

void CNaviInterface::RegisterCmdCallback(PCmdCallBackFunc pFunc) {
    m_pCmdCbFunc = pFunc;
}
/****************************************************************************/
#if 1
void CNaviInterface::RegisterLogInfoCallback(PLogInfoCallBackFunc pLogFunc) {
    m_pLogCbFunc = pLogFunc;
}
#endif
/****************************************************************************/
void CNaviInterface::SetRefineMode(int mode) {
    autochargemapcount = 0;
    m_bcreateautochargemap = true;
    m_ifirstinitloc = 1;
    m_imode = mode;
    // m_ifirstinitloc = 1;
}

void CNaviInterface::Setupdatemode(bool mode) { m_bupdatemap = mode; }

void CNaviInterface::clearupdateg() {
    updateg.nodes.clear();
    return;
}

bool CNaviInterface::Getupdatemode() {
    bool mode = m_bupdatemap;
    return mode;
}

void CNaviInterface::Setexpandmode(bool mode) {
    if (mode) {
        m_ifirstinitloc = 1;
    }
    m_bexpandmap = mode;
}

bool CNaviInterface::Getexpandmode() {
    bool mode = m_bexpandmap;
    return mode;
}

void CNaviInterface::SetVW(vector<double> &vw) {

    Vcur = vw[0];
    Wcur = vw[1];
}

void CNaviInterface::setvisionrecenter() { m_bifrecenter = true; }

void CNaviInterface::Setmanualupdate(bool mode) { m_bmanualupdate = mode; }

bool CNaviInterface::Getmanualmode() {
    bool mode = m_bmanualupdate;
    return mode;
}
void CNaviInterface::SetBackforward(bool mode) { backforward = mode; }

void CNaviInterface::SetFind() { m_bfind = false; }

void CNaviInterface::SetOdmSwitch(bool mode) { m_bodmswitch = mode; }
void CNaviInterface::Setmodifymap(bool mode) { m_ifmodifymap = mode; }
void CNaviInterface::SetSpeedLevle(unsigned char speedlever) {
    m_ucspeedlever = speedlever;
}
void CNaviInterface::Setautocharge(bool mode) { m_bautocharge = mode; }

void CNaviInterface::exit() {
    /*	m_bRunning = false;
            ResetEvent(m_hEvent);
            if(WaitForSingleObject(m_planThread,2000)==WAIT_ABANDONED_0)
            {
                    DWORD dwExitCode;
                    GetExitCodeThread(m_planThread,&dwExitCode);
                    ExitThread(dwExitCode);
            }*/
    pthread_mutex_lock(&m_csLoc_mutex);
    if (m_pscanMatcher != NULL) {
        if (m_pscanMatcher->gm.data != NULL) {
            delete[] m_pscanMatcher->gm.data;
            m_pscanMatcher->gm.data = NULL;
        }
        if (m_pscanMatcher->matcher.gm.data != NULL) {
            delete[] m_pscanMatcher->matcher.gm.data;
            m_pscanMatcher->matcher.gm.data = NULL;
        }
        if (m_pscanMatcher->matcher.dgm.data != NULL) {
            delete[] m_pscanMatcher->matcher.dgm.data;
            m_pscanMatcher->matcher.dgm.data = NULL;
        }

        delete m_pscanMatcher;
        m_pscanMatcher = NULL;
    }
    pthread_mutex_unlock(&m_csLoc_mutex);
    pthread_mutex_lock(&m_csPlan_mutex);
    if (m_pms != NULL) {
        if (m_pms->globalBinaryMap.data != NULL) {
            delete[] m_pms->globalBinaryMap.data;
            m_pms->globalBinaryMap.data = NULL;
        }
        if (m_pms->globalGaussianMap.data != NULL) {
            delete[] m_pms->globalGaussianMap.data;
            m_pms->globalGaussianMap.data = NULL;
        }
        if (m_pms->globalWallMap.data != NULL) {
            delete[] m_pms->globalWallMap.data;
            m_pms->globalWallMap.data = NULL;
        }

        delete m_pms;
        m_pms = NULL;
    }
    pthread_mutex_unlock(&m_csPlan_mutex);
    pthread_mutex_lock(&m_csLoc_mutex);
    if (m_pmsSLAM != NULL) {
        if (m_pmsSLAM->globalBinaryMap.data != NULL) {
            delete[] m_pmsSLAM->globalBinaryMap.data;
            m_pmsSLAM->globalBinaryMap.data = NULL;
        }
        if (m_pmsSLAM->globalGaussianMap.data != NULL) {
            delete[] m_pmsSLAM->globalGaussianMap.data;
            m_pmsSLAM->globalGaussianMap.data = NULL;
        }
        if (m_pmsSLAM->globalWallMap.data != NULL) {
            delete[] m_pmsSLAM->globalWallMap.data;
            m_pmsSLAM->globalWallMap.data = NULL;
        }

        delete m_pmsSLAM;
        m_pmsSLAM = NULL;
    }

    /************/
    if (m_pmsSLAMtest != NULL) {
        if (m_pmsSLAMtest->globalBinaryMap.data != NULL) {
            delete[] m_pmsSLAMtest->globalBinaryMap.data;
            m_pmsSLAMtest->globalBinaryMap.data = NULL;
        }
        if (m_pmsSLAMtest->globalGaussianMap.data != NULL) {
            delete[] m_pmsSLAMtest->globalGaussianMap.data;
            m_pmsSLAMtest->globalGaussianMap.data = NULL;
        }
        if (m_pmsSLAMtest->globalWallMap.data != NULL) {
            delete[] m_pmsSLAMtest->globalWallMap.data;
            m_pmsSLAMtest->globalWallMap.data = NULL;
        }

        delete m_pmsSLAMtest;
        m_pmsSLAMtest = NULL;
    }
    /**********/

    if (m_pmatcher != NULL) {
        if (m_pmatcher->dgm.data != NULL) {
            delete[] m_pmatcher->dgm.data;
            m_pmatcher->dgm.data = NULL;
        }

        if (m_pmatcher->gm.data != NULL) {
            delete[] m_pmatcher->gm.data;
            m_pmatcher->gm.data = NULL;
        }
        delete m_pmatcher;
        m_pmatcher = NULL;
    }
    pthread_mutex_unlock(&m_csLoc_mutex);
}

CNaviInterface g_NaviInterface;

void NAVI_PutLaserData(int nranges, float *ranges, int nintensities,
                       float *intensities) {
    laser_st laserdata;
    int i = 0;

    // laserdata.rad0 = rad0;
    // laserdata.radstep = radstep;
    laserdata.nranges = nranges;
    laserdata.nintensities = nintensities;

    for (i = 0; i < nranges; i++) {
        laserdata.ranges.push_back(ranges[i]);
    }

    for (i = 0; i < nintensities; i++) {
        laserdata.intensities.push_back(intensities[i]);
    }
    g_NaviInterface.putLaserData(&laserdata);
}

void NAVI_PutEncoderData(Pose *pPos) { g_NaviInterface.putEncoderData(pPos); }
/************************************************************************************************************************************************************************************************************/
void NAVI_GetVisionData(float *x, float *y, int length) {
    vector<Pose> visionData;
    for (int i = 0; i < length; i++) {
        Pose temdata;
        if ((NULL == x) || (NULL == y)) {
            break;
        }
        temdata.x = x[i] / 1000;
        temdata.y = y[i] / 1000;
        temdata.theta = 0.0;
        visionData.push_back(temdata);
    }
    if ((true == visionswitch) && (1 == iVisionDateFlag)) {
        g_NaviInterface.GetVisionData(&visionData);
    }
}
void NAVI_GetCollisionData(double **wallxy, int length) {
    vector<Pose> CollisionData;
    for (int i = 0; i < length; i++) {
        Pose temdata;
        if (NULL == *wallxy[i]) {
            break;
        }
        temdata.x = wallxy[i][0] / 1000;
        temdata.y = wallxy[i][1] / 1000;
        temdata.theta = 0.0;
        CollisionData.push_back(temdata);
    }

    g_NaviInterface.GetCollisionData(&CollisionData);
}
void NAVI_ClearVisionData(void) { g_NaviInterface.ClearVisionData(); }
void NAVI_ClearCollisionData(void) { g_NaviInterface.ClearCollisionData(); }
/************************************************************************************************************************************************************************************************************/
bool NAVI_LoadMapAndLoc(const char *strMapName, Pose initPos, Pose initRange,
    vector<Pose> &vtWallPos) {

g_NaviInterface.deleteGoal();
while (g_NaviInterface.m_iplanend == 0) {
printf("waiting for load map\n");fflush(stdout);
}
if (!g_NaviInterface.loadMap(strMapName, vtWallPos)) {
return false;
}
g_NaviInterface.initLoc(initPos, initRange);

return true;
}

void NAVI_SetLocationType(LocationType type) {
    g_NaviInterface.setLocationType(type);
}

void NAVI_SetConfig(RobotConfig config) { g_NaviInterface.setConfig(config); }
void NAVI_CreateMap(double metersPerPixel) {
    g_NaviInterface.createMap(metersPerPixel, false);
}
bool NAVI_CreateMapWithMode(double metersPerPixel, bool forceNewMap) {
    return g_NaviInterface.createMap(metersPerPixel, forceNewMap);
}
void NAVI_SaveMap(const char *strMapName) {fflush(stdout);
    g_NaviInterface.saveMap(strMapName);
}
void NAVI_SaveModifyMap(void) { g_NaviInterface.saveModifyMap(); }

void NAVI_SetGoalPoint(Pose goal) { g_NaviInterface.setGoal(goal); }
void NAVI_DeleteGoal() {
    if (0 == nopathstatus) {
        nopathflag = 0;
    }
    g_NaviInterface.deleteGoal();
}

void NAVI_RevisePose(Pose &posRevise, int flag) {

    g_NaviInterface.RevisePose(posRevise, flag);
}

void NAVI_RevisePoseAndRange(Pose &posRevise, double xSearchRange,
                             double ySearchRange, double thetaSearchRange,
                             unsigned char flag) {

    g_NaviInterface.RevisePoseAndRange(posRevise, xSearchRange, ySearchRange,
                                       thetaSearchRange, flag);
}
void NAVI_RegisterGetPosCallback(PGetPosCallBackFunc pPosFunc) {

    g_NaviInterface.RegisterGetPosCallback(pPosFunc);
}
void NAVI_ReginsterGetPathCallback(PGetPathCallBackFunc pPathFunc) {

    g_NaviInterface.RegisterGetPathCallback(pPathFunc);
}
void NAVI_RegisterNoPathCallback(PHaveNoPathCallBackFunc pFunc) {
    g_NaviInterface.RegisterNoPathCallback(pFunc);
}
void NAVI_RegisterCmdCallback(PCmdCallBackFunc pFunc) {
    g_NaviInterface.RegisterCmdCallback(pFunc);
}

void NAVI_RegisterLogInfoCallback(PLogInfoCallBackFunc pFunc) {
    g_NaviInterface.RegisterLogInfoCallback(pFunc);
}

void NAVI_ParticleFilter(Pose pos, Pose range, int particlenum) {

    g_NaviInterface.particleFilterLoc(pos, range, particlenum);
}

void NAVI_SetRefineMode(int mode) { g_NaviInterface.SetRefineMode(mode); }

void NAVI_Setupdatemode(bool mode) { g_NaviInterface.Setupdatemode(mode); }

bool NAVI_Getupdatemode() { return g_NaviInterface.Getupdatemode(); }

void NAVI_Setexpandmode(bool mode) { g_NaviInterface.Setexpandmode(mode); }

bool NAVI_Getexpandmode() { return g_NaviInterface.Getexpandmode(); }

void NAVI_SetVW(vector<double> &vw) { g_NaviInterface.SetVW(vw); }
void NAVI_SetSpeedLevle(unsigned char ucSpeedLever) {
    g_NaviInterface.SetSpeedLevle(ucSpeedLever);
}

void NAVI_UpdateProbMap(const char *fileName) {
    g_NaviInterface.updateProbMap(fileName);
}

void NAVI_OptimizeMap(const char *strMapName) {
    g_NaviInterface.createProbMap(strMapName);
    printf("save probmap done !\n");fflush(stdout);
}

void NAVI_VisionRecenter() { g_NaviInterface.setvisionrecenter(); }

void NAVI_clearupdateg() { g_NaviInterface.clearupdateg(); }

void NAVI_Exit() { g_NaviInterface.exit(); }

void NAVI_Setdrawvisionmode(bool mode) {
    g_NaviInterface.setdrawvisionmode(mode);
}

void NAVI_Setmanualupdate(bool mode) { g_NaviInterface.Setmanualupdate(mode); }

bool NAVI_Getmanualmode() { return g_NaviInterface.Getmanualmode(); }
void NAVI_SetBackforward(bool mode) { g_NaviInterface.SetBackforward(mode); }
void NAVI_CloseVisionDate() {
    printf("close vision !\n");fflush(stdout);
    iVisionDateFlag = 0;
}
void NAVI_OpenVisionDate() {
    printf("open vision !\n");fflush(stdout);
    iVisionDateFlag = 1;
}
void NAVI_SetFind() { g_NaviInterface.SetFind(); }
void NAVI_SetOdmSwitch(bool mode) { g_NaviInterface.SetOdmSwitch(mode); }
void NAVI_Setmodifymap(bool mode) { g_NaviInterface.Setmodifymap(mode); }
void NAVI_Setlimitdis(int dis) { iLimitDis = dis; }
void NAVI_Setautocharge(bool mode) { g_NaviInterface.Setautocharge(mode); }
void NAVI_Savevisiondate(int mode) { g_iSaveDateFlag = mode; }

// AnXin：2025-2-11 2025-5-10 2025-6-10
void CNaviInterface::setSearchType(int searchType) {
    g_NaviInterface.searchType = searchType;
    cout << "g_NaviInterface.searchType = " << g_NaviInterface.searchType << endl;
    switch (g_NaviInterface.searchType)
    {
        case 1:
            cout << "DStar!" << endl;
            break;
        case 2:
            cout << "AStar!" << endl;
            break;
        case 3:
            cout << "BFS!" << endl;
            break;
        default:
            cout << "Unknown searchType! Default DStar!" << endl;
            break;
    }
}
void CNaviInterface::setPlanFullPath(int algNum) {
    g_NaviInterface.fullCoverageAlg.algNum = algNum;
    cout << "algNum1 = " << algNum << endl;
    cout << "g_NaviInterface.fullCoverageAlg.algNum = " << g_NaviInterface.fullCoverageAlg.algNum << endl;
    g_NaviInterface.enableCoverage = true;
}
void CNaviInterface::cancelPlanFullPath(void) {
    g_NaviInterface.enableCoverage = false;
    while (m_waypoints.size() > 0)
    {
        m_waypoints.pop();
    }
}
void CNaviInterface::setRoomVertex(int order, double x, double y) {
    fullCoverageAlg.roomVertex[order * 2] = x;
    fullCoverageAlg.roomVertex[order * 2 + 1] = y;
}
void CNaviInterface::subGetMapFromMain(int* ip) {
    // 构造 IP 字符串
    char ipStr[32] = {0};
    snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", (uint8_t)ip[0], (uint8_t)ip[1], (uint8_t)ip[2], (uint8_t)ip[3]);fflush(stdout);

    printf("Sub-device is fetching map from main device, IP:%s\n", ipStr);fflush(stdout);

    // 假设地图服务运行在主机 8000 端口
    const char* remoteFile = "/defultMap.txt";
    const char* localMapFile = "/data/test/defultMap.txt";
    const char* localMapTmpFile = "/data/test/defultMap.txt.tmp";

    // === 下载地图文件 ===
    remove(localMapTmpFile);
    char wgetCmd[256];
    snprintf(wgetCmd, sizeof(wgetCmd),
             "wget http://%s:8000%s -O %s --timeout=3 --tries=1",
             ipStr, remoteFile, localMapTmpFile);fflush(stdout);

    //printf("Executing command：%s\n", wgetCmd);
    cout << "Executing command: " << wgetCmd << endl;
    int ret = system(wgetCmd);
    if (ret == 0 && fileHasContent(localMapTmpFile) &&
        replaceFileAtomic(localMapTmpFile, localMapFile)) {
        printf("Map fetched successfully.\n");fflush(stdout);
    } else {
        fprintf(stderr, "Failed to fetch map. Error code: %d\n", ret);
        remove(localMapTmpFile);
    }
    // === 下载路径文件 ===
    const char* remotePathFile = "/roadFile.txt";
    const char* localPathFile = "/data/test/roadFile.txt";
    const char* localPathTmpFile = "/data/test/roadFile.txt.tmp";
    const int maxRetries = 10;
    const int delaySeconds = 2;

    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
        remove(localPathTmpFile);
        char pathCmd[256];
        snprintf(pathCmd, sizeof(pathCmd),
                 "wget http://%s:8000%s -O %s --timeout=3 --tries=1",
                 ipStr, remotePathFile, localPathTmpFile);

        cout << "Attempt " << attempt << ": Executing command: " << pathCmd << endl;
        int retPath = system(pathCmd);

        if (retPath == 0) {
            if (fileHasContent(localPathTmpFile)) {
                if (replaceFileAtomic(localPathTmpFile, localPathFile)) {
                    printf("Path file fetched successfully.\n"); fflush(stdout);
                    break; // 成功退出循环
                } else {
                    printf("Failed to replace path file. Retrying...\n"); fflush(stdout);
                }
            } else {
                printf("Path file is empty, retrying in %d seconds...\n", delaySeconds); fflush(stdout);
            }
        } else {
            fprintf(stderr, "Failed to fetch path file. Error code: %d. Retrying...\n", retPath);
        }

        remove(localPathTmpFile);
        sleep(delaySeconds);
    }
}
void *CNaviInterface::CoverageThreadProc(LPVOID pPara) {
    CNaviInterface *pObject = (CNaviInterface *)pPara;
    cout << "pObject->fullCoverageAlg.algNum = " << pObject->fullCoverageAlg.algNum << endl;

    while (pObject->fullCoverageAlg.algNum == -1)
    {
        ;
    }
    switch (pObject->fullCoverageAlg.algNum)
    {
        case 0:
        {
            /*
             * * 牛耕法 + D* 规划
             */
            while (pObject->m_bRunning) {
                // 等待条件变量
                // pthread_mutex_lock(&(pObject->m_mutex));
                // pthread_cond_wait(&(pObject->m_condPathPlan), &(pObject->m_mutex));
        
                // 执行全覆盖路径规划
                if (pObject->enableCoverage) {
                    // 清除上一次全覆盖路径
                    // 清空队列
                    while (!pObject->fullCoverageAlg.pathIPoint.empty()) {
                        pObject->fullCoverageAlg.pathIPoint.pop();  // 移除队列头部元素
                    }
                    
                    // 探测方向
                    //  0: 向前
                    //  1: 向后
                    //  2: 向左
                    //  3: 向右
                    int direction = 0;
        
                    bool initFlag = true;
                    // 划定房间范围
                    vector<IPoint> convexHull;
                    if (GET_ROOM_BOUNDARY == 0)
                    {
                        // 使用自动划分方式（凸包算法）
                        convexHull = pObject->fullCoverageAlg.autoGetRoomBoundary(
                            pObject->astarPlanner.GMapLength,
                            pObject->astarPlanner.GMapWidth,
                            pObject->astarPlanner.m_pGridState,
                            initFlag
                        );
                    }
                    else if (GET_ROOM_BOUNDARY == 1)
                    {
                        // 使用手动划分方式（自主选点）
                        convexHull = pObject->fullCoverageAlg.manualGetRoomBoundary(
                            pObject->astarPlanner,
                            initFlag
                        );
                    }
                    else
                    {
                        printf("Invalid GET_ROOM_BOUNDARY: %d!\n", GET_ROOM_BOUNDARY);fflush(stdout);
                        pObject->setLastFullPathError(FULLPATH_ROOM_INIT_FAILED);
                        // 将所有障碍物点都归为墙壁（测试版未做）
                        initFlag = false;
                    }
                    printf("Convex hull points num is %d\n", convexHull.size());fflush(stdout);
        
                    // 初始化覆盖地图
                    if (initFlag)
                    {
                        pObject->fullCoverageAlg.initCoverageState(
                            pObject->astarPlanner.GMapLength,
                            pObject->astarPlanner.GMapWidth,
                            pObject->astarPlanner.m_pGridState,
                            convexHull,
                            initFlag
                        );
                    }
                    else
                    {
                        break;
                    }
        
                    // 生成初始覆盖地图
                    cout << "Generating initial coverage map ..." << endl;
                    pObject->fullCoverageAlg.writeCoverageMapToFile(
                        pObject->astarPlanner.GMapLength,
                        pObject->astarPlanner.GMapWidth,
                        "tmpcoverageMap.txt"
                    );
                    cout << "Initial coverage map generation successful!" << endl;
        
                    printf("Start full coverage planning...\n");fflush(stdout);
                    pObject->fullCoverageAlg.pathIPoint.push(
                        pObject->astarPlanner.GlobalToGrid(
                            pObject->m_CurPosForPath.x, pObject->m_CurPosForPath.y
                        )
                    );
                    // Pose endpose = pObject->m_CurPosForPath;
                    // 使用 D* 规划探测路径
                    // Pose endpose = pObject->m_CurPosForPath;
                    while (initFlag && !pObject->fullCoverageAlg.isFinished)
                    {
                        // 进入新区域时，将行进方向更改为前向
                        direction = 0;
                        while (1)
                        {
                            while (pObject->m_waypoints.size() > 0)
                            {
                                ;
                            }
                            Pose endpose = pObject->m_CurPosForPath;
                            bool success = false;
                            switch (direction)
                            {
                            case 0:
                                success = pObject->fullCoverageAlg.zigzagCoverage.detectFrontPlan(
                                    pObject->m_pms->astarMap,
                                    pObject->m_CurPosForPath,
                                    endpose,
                                    pObject->m_pms->laserMap,
                                    pObject->m_pms->visionMap,
                                    pObject->astarPlanner,
                                    pObject->fullCoverageAlg.coverageState
                                );
                                break;
                            case 1:
                                success = pObject->fullCoverageAlg.zigzagCoverage.detectBackPlan(
                                    pObject->m_pms->astarMap,
                                    pObject->m_CurPosForPath,
                                    endpose,
                                    pObject->m_pms->laserMap,
                                    pObject->m_pms->visionMap,
                                    pObject->astarPlanner,
                                    pObject->fullCoverageAlg.coverageState
                                );
                                break;
                            case 2:
                                success = pObject->fullCoverageAlg.zigzagCoverage.detectLeftPlan(
                                    pObject->m_pms->astarMap,
                                    pObject->m_CurPosForPath,
                                    endpose,
                                    pObject->m_pms->laserMap,
                                    pObject->m_pms->visionMap,
                                    pObject->astarPlanner,
                                    pObject->fullCoverageAlg.coverageState
                                );
                                break;
                            case 3:
                                success = pObject->fullCoverageAlg.zigzagCoverage.detectRightPlan(
                                    pObject->m_pms->astarMap,
                                    pObject->m_CurPosForPath,
                                    endpose,
                                    pObject->m_pms->laserMap,
                                    pObject->m_pms->visionMap,
                                    pObject->astarPlanner,
                                    pObject->fullCoverageAlg.coverageState
                                );
                                break;
                            }
                            printf("Detect Path finished.\n");fflush(stdout);
        
                            if (success) {
                                switch (direction)
                                {
                                case 0:
                                    printf("Detect Front Path success!\n");fflush(stdout);
                                    break;
                                case 1:
                                    printf("Detect Back Path success!\n");fflush(stdout);
                                    break;
                                case 2:
                                    printf("Detect Left Path success!\n");fflush(stdout);
                                    break;
                                case 3:
                                    printf("Detect Right Path success!\n");fflush(stdout);
                                    break;
                                }
                                // printf("Detect Front Path success!\n");
                                pObject->setvisionrecenter();
                                pObject->setGoal(endpose);
                                pObject->fullCoverageAlg.setValue(
                                    pObject->astarPlanner.GMapLength,
                                    pObject->astarPlanner.GMapWidth,
                                    pObject->astarPlanner.GlobalToGrid(endpose.x, endpose.y)
                                );
                                pObject->fullCoverageAlg.pathIPoint.push(pObject->astarPlanner.GlobalToGrid(endpose.x, endpose.y));
        
                                // 保持探测方向
                                direction = direction;
                            }
                            else
                            {
                                printf("Failed!\n");fflush(stdout);
                                if (direction == 0)
                                {
                                    // 前进方向且前方遇到障碍物，则向右转一段距离
                                    success = pObject->fullCoverageAlg.zigzagCoverage.detectRightPlan(
                                        pObject->m_pms->astarMap,
                                        pObject->m_CurPosForPath,
                                        endpose,
                                        pObject->m_pms->laserMap,
                                        pObject->m_pms->visionMap,
                                        pObject->astarPlanner,
                                        pObject->fullCoverageAlg.coverageState
                                    );
                                    if (success)
                                    {
                                        // 右转成功，之后方向变为向后后退
                                        pObject->setvisionrecenter();
                                        pObject->setGoal(endpose);
                                        pObject->fullCoverageAlg.setValue(
                                            pObject->astarPlanner.GMapLength,
                                            pObject->astarPlanner.GMapWidth,
                                            pObject->astarPlanner.GlobalToGrid(endpose.x, endpose.y)
                                        );
                                        pObject->fullCoverageAlg.pathIPoint.push(pObject->astarPlanner.GlobalToGrid(endpose.x, endpose.y));
                                        // 切换探测方向为后退
                                        direction = (direction + 1) % 2;
                                    }
                                    else
                                    {
                                        // 右转失败，表示区域为狭长走廊形，无法左右行进
                                        // 直接后转
                                        // 切换探测方向为后退
                                        direction = (direction + 1) % 2;
                                    }
                                }
                                else
                                {
                                    // 后退方向且后方遇到障碍物，则向右转一段距离
                                    success = pObject->fullCoverageAlg.zigzagCoverage.detectRightPlan(
                                        pObject->m_pms->astarMap,
                                        pObject->m_CurPosForPath,
                                        endpose,
                                        pObject->m_pms->laserMap,
                                        pObject->m_pms->visionMap,
                                        pObject->astarPlanner,
                                        pObject->fullCoverageAlg.coverageState
                                    );
                                    if (success)
                                    {
                                        // 右转成功，之后方向变为向前前进
                                        pObject->setvisionrecenter();
                                        pObject->setGoal(endpose);
                                        pObject->fullCoverageAlg.setValue(
                                            pObject->astarPlanner.GMapLength,
                                            pObject->astarPlanner.GMapWidth,
                                            pObject->astarPlanner.GlobalToGrid(endpose.x, endpose.y)
                                        );
                                        pObject->fullCoverageAlg.pathIPoint.push(pObject->astarPlanner.GlobalToGrid(endpose.x, endpose.y));
                                        // 切换探测方向为前进
                                        direction = (direction + 1) % 2;
                                    }
                                    else
                                    {
                                        // 右转失败，表示区域已探测完毕
                                        // 退出循环
                                        break;
                                    }
                                }
        
                                // 切换探测方向
                                // direction = (direction + 1) % 2;
                            }
        
                            sleep(2);
                        }
        
                        cout << "The current area has been fully covered!" << endl;
                        // 检测是否还有未覆盖区域
                        IPoint curIPoint = pObject->astarPlanner.GlobalToGrid(
                            pObject->m_CurPosForPath.x,
                            pObject->m_CurPosForPath.y
                        );
                        pObject->fullCoverageAlg.findNextGoal_BFS_LimitDistance(
                            curIPoint,
                            pObject->astarPlanner,
                            100
                        );
                        if (pObject->fullCoverageAlg.isFinished)
                        {
                            cout << "All areas are covered!" << endl;
                        }
                        else
                        {
                            cout << "Navigating to the new area, please wait ..." << endl;
                            pObject->setvisionrecenter();
                            pObject->setGoal(pObject->fullCoverageAlg.nextGoal);
                            pObject->fullCoverageAlg.pathIPoint.push(
                                pObject->astarPlanner.GlobalToGrid(
                                    pObject->fullCoverageAlg.nextGoal.x, pObject->fullCoverageAlg.nextGoal.y
                                )
                            );
                            sleep(2);
                        }
                    }
        
                    cout << "Generating coverage map, please wait ..." << endl;
                    pObject->enableCoverage = false;
                    pObject->fullCoverageAlg.writeCoverageMapToFile(
                        pObject->astarPlanner.GMapLength,
                        pObject->astarPlanner.GMapWidth,
                        "coverageMap.txt"
                    );
                    cout << "Coverage map generated successfully!" << endl;
                }
        
                // cout << "Full coverage plan finish!" << endl;
        
                // pthread_mutex_unlock(&(pObject->m_mutex));
            }
            break;
        }
        case 1:
        {
            /*
             * * STC + D* 规划
             */
            while (pObject->m_bRunning) {
                // 执行全覆盖路径规划
                if (pObject->enableCoverage) {
                    // 清除上一次全覆盖路径
                    // 清空队列
                    while (!pObject->fullCoverageAlg.pathIPoint.empty()) {
                        pObject->fullCoverageAlg.pathIPoint.pop();  // 移除队列头部元素
                    }
        
                    bool initFlag = true;
                    // 划定房间范围
                    vector<IPoint> convexHull;
                    if (GET_ROOM_BOUNDARY == 0)
                    {
                        // 使用自动划分方式（凸包算法）
                        convexHull = pObject->fullCoverageAlg.autoGetRoomBoundary(
                            pObject->astarPlanner.GMapLength,
                            pObject->astarPlanner.GMapWidth,
                            pObject->astarPlanner.m_pGridState,
                            initFlag
                        );
                    }
                    else if (GET_ROOM_BOUNDARY == 1)
                    {
                        // 使用手动划分方式（自主选点）
                        convexHull = pObject->fullCoverageAlg.manualGetRoomBoundary(
                            pObject->astarPlanner,
                            initFlag
                        );
                    }
                    else
                    {
                        printf("Invalid GET_ROOM_BOUNDARY: %d!\n", GET_ROOM_BOUNDARY);fflush(stdout);
                        pObject->setLastFullPathError(FULLPATH_ROOM_INIT_FAILED);
                        // 将所有障碍物点都归为墙壁（测试版未做）
                        initFlag = false;
                    }
                    printf("Convex hull points num is %d\n", convexHull.size());fflush(stdout);
        
                    // 初始化覆盖地图
                    if (initFlag)
                    {
                        pObject->fullCoverageAlg.initCoverageState(
                            pObject->astarPlanner.GMapLength,
                            pObject->astarPlanner.GMapWidth,
                            pObject->astarPlanner.m_pGridState,
                            convexHull,
                            initFlag
                        );
                    }
                    else
                    {
                        break;
                    }
        
                    // 生成初始覆盖地图
                    cout << "Generating initial coverage map ..." << endl;
                    pObject->fullCoverageAlg.writeCoverageMapToFile(
                        pObject->astarPlanner.GMapLength,
                        pObject->astarPlanner.GMapWidth,
                        "initCoverageMap.txt"
                    );
                    cout << "Initial coverage map generation successful!" << endl;
        
                    // 开始执行生成树覆盖方法
                    IPoint originalStartIPoint = pObject->astarPlanner.GlobalToGrid(
                        pObject->m_CurPosForPath.x,
                        pObject->m_CurPosForPath.y
                    );
                    IPoint startIPoint;
                    pObject->fullCoverageAlg.mapToGrid(
                        originalStartIPoint.x,
                        originalStartIPoint.y,
                        ROBOTSIZE,
                        startIPoint.x,
                        startIPoint.y
                    );
                    printf("Start full coverage planning...\n");fflush(stdout);
                    pObject->fullCoverageAlg.convertToGridMap(
                        pObject->astarPlanner.GMapLength,
                        pObject->astarPlanner.GMapWidth,
                        ROBOTSIZE
                    );
                    pObject->fullCoverageAlg.stcCoverage.generateSTCPath(
                        pObject->fullCoverageAlg.coverageGridMap,
                        pObject->astarPlanner.GMapLength / ROBOTSIZE,
                        pObject->astarPlanner.GMapWidth / ROBOTSIZE,
                        startIPoint
                    );
                    pObject->fullCoverageAlg.stcCoverage.startDFSTraversal();
                    // 将 goPath 中的每一个坐标映射回原始地图坐标
                    for (auto& elem : pObject->fullCoverageAlg.stcCoverage.goPath)
                    {
                        IPoint curElem;
                        pObject->fullCoverageAlg.mapToOriginal(
                            elem.x,
                            elem.y,
                            ROBOTSIZE,
                            curElem.x,
                            curElem.y
                        );
                        elem = curElem;
                    }
                    // 生成遍历地图
                    for (IPoint val : pObject->fullCoverageAlg.stcCoverage.goPath)
                    {
                        pObject->fullCoverageAlg.pathIPoint.push(val);
                    }
                    pObject->fullCoverageAlg.writeCoverageMapToFile(
                        pObject->astarPlanner.GMapLength,
                        pObject->astarPlanner.GMapWidth,
                        "midMap.txt"
                    );
                    // 依次遍历 goPath 中的路径点
                    Pose endpose;
                    while (!pObject->fullCoverageAlg.stcCoverage.goPath.empty())
                    {
                        while (pObject->m_waypoints.size() > 0 && !(pObject->targetErr))
                        {
                            ;
                        }
                        if (pObject->targetErr == true)
                        {
                            if (pObject->m_waypoints.size() > 0)
                            {
                                pObject->m_waypoints.pop();
                            }
                        }
                        pObject->targetErr = false;
                        IPoint nextIPoint = pObject->fullCoverageAlg.stcCoverage.goPath.front();
                        pObject->fullCoverageAlg.stcCoverage.goPath.erase(
                            pObject->fullCoverageAlg.stcCoverage.goPath.begin()
                        );
                        endpose = pObject->astarPlanner.GridToGlobal(nextIPoint.x, nextIPoint.y);
                        vector<double> target;
                        target.push_back(endpose.x);
                        target.push_back(endpose.y);
                        if (pObject->iftargetlegalStatic(target) && pObject->fullCoverageAlg.ifIPointAroundLegal(nextIPoint, pObject->astarPlanner))
                        {
                            pObject->setvisionrecenter();
                            pObject->setGoal(endpose);
                            pObject->fullCoverageAlg.setValue(
                                pObject->astarPlanner.GMapLength,
                                pObject->astarPlanner.GMapWidth,
                                pObject->astarPlanner.GlobalToGrid(endpose.x, endpose.y)
                            );
                            pObject->fullCoverageAlg.pathIPoint.push(nextIPoint);
                        }
                        else
                        {
                            cout << "Error IPoint: (" << nextIPoint.x << ", " << nextIPoint.y << ")!" << endl;
                        }
                    }
                    // 生成覆盖地图
                    cout << "Generating coverage map ..." << endl;
                    pObject->fullCoverageAlg.writeCoverageMapToFile(
                        pObject->astarPlanner.GMapLength,
                        pObject->astarPlanner.GMapWidth,
                        "coverageMap.txt"
                    );
                    pObject->enableCoverage = false;
                }
            }
            break;
        }
        case 2:
        {
             while (pObject->m_bRunning) {
                // 执行全覆盖路径规划
                if (pObject->enableCoverage) {
                    printf("---------------------------2025.8.8---------------------------\n");fflush(stdout);
                    int robotId = pObject->robotId;
                    if (robotId == 1){ 
                        vector<IPoint> convexHull;
                        bool initFlag = true;
                        convexHull = pObject->fullCoverageAlg.manualGetRoomBoundary(pObject->astarPlanner,initFlag);
                        int safesize = 7; 
                        int minsize = 5; 
                        int rob[2] = {convexHull[2].x, convexHull[2].y};  // 机器人坐标
                        pObject->CreateFullPath(convexHull[0].x, convexHull[0].y, convexHull[1].x, convexHull[1].y, rob, safesize, minsize);
                    }
		    string filepath = "/data/test/roadFile.txt";
                    vector<IPoint> fullPath = pObject->ReadFullPathFromFile(filepath);
                    if (fullPath.size() < 2) {
                        printf("Invalid path file!\n");fflush(stdout);
                        pObject->setLastFullPathError(FULLPATH_ROAD_FILE_INVALID);
                        break;
                    }
                    vector<IPoint> subPath;
                    int mid = fullPath.size() / 2;
                    if (robotId == 1) {
                        subPath.assign(fullPath.begin(), fullPath.begin() + mid);  // 主机从前半段
                    } else {
                        subPath.assign(fullPath.rbegin(), fullPath.rbegin() + (fullPath.size() - mid));  // 副机从后半段逆序走
                    }
                    printf("Robot %d will follow path with %zu points.\n", robotId, subPath.size());fflush(stdout);
                    vector<IPoint> gridPath = subPath;
                    if (gridPath.size() < 2) {
                        printf("Path too short.\n");fflush(stdout);
                        pObject->setLastFullPathError(FULLPATH_ROAD_FILE_INVALID);
                    }

                    // 提取拐点
                    vector<IPoint> turnPoints;
                    turnPoints.push_back(gridPath[0]);  // 起点

                    for (int i = 1; i < gridPath.size() - 1; ++i)
                    {
                        double dx1 = gridPath[i].x - gridPath[i - 1].x;
                        double dy1 = gridPath[i].y - gridPath[i - 1].y;
                        double dx2 = gridPath[i + 1].x - gridPath[i].x;
                        double dy2 = gridPath[i + 1].y - gridPath[i].y;

                        // 判断方向是否变化（转角点）
                        if (dx1 * dy2 != dy1 * dx2) {
                            turnPoints.push_back(gridPath[i]);
                        }
                    }

                    turnPoints.push_back(gridPath.back());  // 终点

                    printf("Turn point count: %lu -> Reduced from %lu\n", turnPoints.size(), gridPath.size());
                    fflush(stdout);

                    // 导航到每个拐点（逐个下发）
                    size_t turnIndex = 0;
                    while (turnIndex < turnPoints.size())
                    {
                        pObject->updateCoopAvoidance();
                        if (pObject->isStoppedForCoopPeer()) {
                            usleep(100000);
                            continue;
                        }

                        if (pObject->targetErr) {
                            printf("Previous target failed, skipping.\n");fflush(stdout);
                            pObject->targetErr = false;
                            if (!pObject->m_waypoints.empty()) pObject->m_waypoints.pop();
                            turnIndex++;
                            continue;
                        }

                        if (pObject->m_waypoints.size() == 0) {
                            IPoint p_grid = turnPoints[turnIndex];
                            Pose p_real = pObject->astarPlanner.GridToGlobal((int)p_grid.x, (int)p_grid.y);

                            printf("[Turn %lu/%lu] Navigating to Grid (%.0f, %.0f) => World (%.2f, %.2f)\n",
                                turnIndex + 1, turnPoints.size(), p_grid.x, p_grid.y, p_real.x, p_real.y);
                            fflush(stdout);

                            pObject->markCoverageIndex((int)turnIndex);
                            pObject->setvisionrecenter();
                            pObject->setGoal(p_real);

                            IPoint iPoint = pObject->astarPlanner.GlobalToGrid(p_real.x, p_real.y);
                            pObject->fullCoverageAlg.pathIPoint.push(iPoint);
                        }

                        cout << "Waiting for current navigation to complete..." << endl;
                        while (pObject->m_waypoints.size() > 0 && !pObject->targetErr) {
                            pObject->updateCoopAvoidance();
                            if (pObject->isStoppedForCoopPeer()) {
                                break;
                            }
                            usleep(10000);
                        }
                        if (pObject->isStoppedForCoopPeer()) {
                            continue;
                        }
                        if (!pObject->targetErr && pObject->m_waypoints.size() == 0) {
                            turnIndex++;
                        }
                    }

                    // 返回起点
                    pObject->clearNavigationStateAndStop();
                    pObject->resetCoverageState();
                    pObject->enableCoverage = false;
                }
            }
        }
    }

    pObject->enableCoverage = false;
    pObject->fullCoverageAlg.algNum = -1;
    return NULL;
}
void CNaviInterface::CreateFullPath(int x1, int y1, int x2, int y2, int* rob, int safesize, int minsize) {
	// 从平板中接收到两个点，这两个点代表矩形区域的对角线
	// 矩形的边和坐标轴平行，因此对角线可以确定一个矩形
	// 定义矩形的较小/大坐标xs,ys/xb,yb，用于判断路径起点的选点
	if (x1 == x2 || y1 == y2) {
		std::cout << "Select Point Error! " << std::endl;
		return;
	}
	int xs = 0, ys = 0, xb = 0, yb = 0;
	xs = (x1 < x2) ? (x1) : (x2);
	ys = (y1 < y2) ? (y1) : (y2);
	xb = (xs == x1) ? (x2) : (x1);
	yb = (ys == y1) ? (y2) : (y1);
	// 定义矩形横纵向边长xlen, ylen
	int xlen = 0, ylen = 0;
	xlen = xb - xs;
	ylen = yb - ys;
	// xnum/ynum数组中数值分别表示网格点尺寸，最后一个网格点尺寸，该方向上网格点个数
	int xnum[50] = { -1, };
	int ynum[50] = { -1, };
	getsize_forFullRoad(xlen, safesize, minsize, xnum);
	getsize_forFullRoad(ylen, safesize, minsize, ynum);
	// 开始路径规划：如果xlen更长，则以左下角为起点纵向牛耕，否则左上角为起点横向牛耕
	// 优化，将数组传给变量
	if (xnum[0] == -1 || ynum[0] == -1) {
		std::cout << "matrix size is too small,can not create path for two robots" << std::endl;
		return;
	}
	else if (xnum[0] == -2 || ynum[0] == -2) {
		std::cout << "minsize is too big, can not create path for two robots" << std::endl;
		return;
	}
	int xn = 0;
	for (xn = 0; xnum[xn] != -1; xn++);
	int yn = 0;
	for (yn = 0; ynum[yn] != -1; yn++);
	std::cout << "Start getting full path road" << std::endl;
    const char *roadFileName = "/data/test/roadFile.txt";
    const char *roadTmpFileName = "/data/test/roadFile.txt.tmp";
    remove(roadTmpFileName);
	std::ofstream roadfile;
	roadfile.open(roadTmpFileName, ios::out);
    if (!roadfile.is_open()) {
        std::cout << "Cannot open path file for writing: " << roadTmpFileName << std::endl;
        return;
    }
	int xpoint = 0, ypoint = 0;
	int robx = rob[0], roby = rob[1];
	// 规划路径时的方向dirx,diry，dirx表示向x轴负/正向,diry同理
	int dirx = 0, diry = 0;
	if (abs(robx - xs) <= abs(robx - xb)) {
		xpoint = xs + xnum[0] / 2;
		dirx = 1;
	}
	else {
		xpoint = xb - xnum[0] / 2;
		dirx = -1;
	}
	if (abs(roby - ys) <= abs(robx - xb)) {
		ypoint = ys + ynum[0] / 2;
		diry = 1;
	}
	else {
		ypoint = yb - ynum[0] / 2;
		diry = -1;
	}
	// xnum和ynum表示导航点尺寸，此时用这两个数组计算相邻导航点中心之间的距离
	// 此时xn,yn转换成数组中数据的个数
	int xsize[50] = { 0 };
	int ysize[50] = { 0 };
	xn--;
	yn--;
	for (int x = 0; x < xn; x++) {
		xsize[x] = (xnum[x + 1] + xnum[x]) / 2;
		xnum[x] = 0;
	}
	if (xn == 3) {
		int temp = xsize[0];
		xsize[0] = xsize[1];
		xsize[1] = temp;
	}
	xnum[xn] = 0;
	xsize[xn] = 0;
	for (int y = 0; y < yn; y++) {
		ysize[y] = (ynum[y + 1] + ynum[y]) / 2;
		ynum[y] = 0;
	}
	if (yn == 3) {
		int temp = ysize[0];
		ysize[0] = ysize[1];
		ysize[1] = temp;
	}
	ynum[yn] = 0;
	ysize[yn] = 0;
	// 根据宽边转弯，窄边直线走的逻辑，循环将点写入，每次写入x,y点并推断下一点坐标
	if (xlen >= ylen) {
		// 外循环在内循环结束后跳转下一行，共yn+1行，需要运行yn+1次
		// 最后一次跳转后不会继续创建路径，因为跳转后没有下一次循环
		for (int x = 0; x <= xn; x++) {
			// 内循环走直线，一共yn+1个点，要走yn次
			for (int y = 0; y < yn; y++) {
				roadfile << xpoint << ',' << ypoint << std::endl;
				ypoint += (x % 2 == 0) ? (diry * ysize[y]) : (-diry * ysize[yn - 1 - y]);
			}
			roadfile << xpoint << ',' << ypoint << std::endl;
			xpoint += dirx * xsize[x];
		}
	}
	// ylen宽边同理
	else {
		for (int y = 0; y <= yn; y++) {
			for (int x = 0; x < xn; x++) {
				roadfile << xpoint << ',' << ypoint << std::endl;
				xpoint += (y % 2 == 0) ? (dirx * xsize[x]) : (-dirx * xsize[xn - 1 - x]);
			}
			roadfile << xpoint << ',' << ypoint << std::endl;
			ypoint += diry * ysize[y];
		}
	}
    if (!closeAndReplaceTextFile(roadfile, roadTmpFileName, roadFileName)) {
        printf("(CoverageThreadProc)Create full path failed while replacing %s\n",
               roadFileName);
        fflush(stdout);
        return;
    }
    printf("(CoverageThreadProc)Create full path from (%d, %d) to (%d, %d) with robot at (%d, %d), successfully\n",
               x1, y1, x2, y2, rob[0], rob[1]);fflush(stdout);

}
// CreateFullRoad的附加函数，用于判断某一方向上网格点尺寸，并更改数组list值
void CNaviInterface::getsize_forFullRoad(int len, int safe,int mins, int* list) {
	// 导航点尺寸由小到大，导航点个数num
	int num = 0;

	// 使用int除法舍入特性得到初始的num和差值delta
	num = len / mins;
	int delta = len % mins;
	if (num == 0 || num == 1 || (num == 2 && len / 2 < safe)) {
		list[0] = -1;
		return;
	}
	else if ((len - safe * 2) / mins <= 1) {
		if ((len - safe) / mins == 2) {
			list[1] = safe;
			list[0] - (len - safe) / 2;
			list[2] = len - list[0] - list[1];
			return;
		}
		list[0] = len / 2;
		list[1] = len - list[0];
		list[2] = -1;
		return;
	}
	else {
		num = (len - safe * 2) / mins;
		int nowsize = (len - safe * 2) / num;
		int endsize = len - safe * 2 - nowsize * (num - 1);
		int endnum = 1;
		for (int i = 0; i <= num + 2; i++) {
			list[i] = nowsize;
		}
		if (num % 2 == 0) {
			list[num / 2] = safe;
			list[num / 2 + 1] = safe;
			list[num / 2 + 2] = endsize;
		}
		else {
			list[num / 2] = endsize;
			list[num / 2 + 1] = safe;
			list[num / 2 + 2] = safe;
		}
		list[num + 2] = -1;
	}
	return;
}

/*void CNaviInterface::NavigatePathByGridPoints(const vector<Pose>& gridPath)
{
    if (gridPath.empty()) {
        printf("Path is empty, nothing to navigate.\n");
        return;
    }

    for (size_t i = 0; i < gridPath.size(); ++i)
    {
        const Pose& p_grid = gridPath[i];
        Pose p_real = astarPlanner.GridToGlobal((int)p_grid.x, (int)p_grid.y);

        printf("[Step %lu/%lu] Navigating to (Grid: %.0f, %.0f) => (World: %.2f, %.2f)\n",
               i + 1, gridPath.size(), p_grid.x, p_grid.y, p_real.x, p_real.y);
        fflush(stdout);

        // 等待前一个点完成导航
        while (m_waypoints.size() > 0 && !targetErr);

        if (targetErr) {
            printf("Navigation to last point failed. Skipping...\n");fflush(stdout);
            targetErr = false;
            if (!m_waypoints.empty()) m_waypoints.pop();
            continue;
        }

        setvisionrecenter();
        setGoal(p_real);  // 启动导航

        // 可选：记录路径点
        IPoint iPoint = astarPlanner.GlobalToGrid(p_real.x, p_real.y);
        fullCoverageAlg.pathIPoint.push(iPoint);
    }

    // 最后返回起点
    Pose start_grid = gridPath.front();
    Pose start_real = astarPlanner.GridToGlobal((int)start_grid.x, (int)start_grid.y);

    printf("Returning to start point (%.2f, %.2f)\n", start_real.x, start_real.y);
    fflush(stdout);

    setvisionrecenter();
    setGoal(start_real);
}*/
void CNaviInterface::NavigatePathByGridPoints(const vector<Pose>& gridPath)// 只导航到拐点
{
    if (gridPath.size() < 2) {
        printf("Path too short.\n");
        setLastFullPathError(FULLPATH_ROAD_FILE_INVALID);
        return;
    }

    // 提取拐点
    vector<Pose> turnPoints;
    turnPoints.push_back(gridPath[0]);  // 起点

    for (int i = 1; i < gridPath.size() - 1; ++i)
    {
        double dx1 = gridPath[i].x - gridPath[i - 1].x;
        double dy1 = gridPath[i].y - gridPath[i - 1].y;
        double dx2 = gridPath[i + 1].x - gridPath[i].x;
        double dy2 = gridPath[i + 1].y - gridPath[i].y;

        // 判断方向是否变化（转角点）
        if (dx1 * dy2 != dy1 * dx2) {
            turnPoints.push_back(gridPath[i]);
        }
    }

    turnPoints.push_back(gridPath.back());  // 终点

    printf("Turn point count: %lu -> Reduced from %lu\n", turnPoints.size(), gridPath.size());
    fflush(stdout);

    // 导航到每个拐点（逐个下发）
    for (int i = 0; i < turnPoints.size(); ++i)
    {
        Pose p_grid = turnPoints[i];
        Pose p_real = astarPlanner.GridToGlobal((int)p_grid.x, (int)p_grid.y);

        printf("[Turn %lu/%lu] Navigating to Grid (%.0f, %.0f) => World (%.2f, %.2f)\n",
               i + 1, turnPoints.size(), p_grid.x, p_grid.y, p_real.x, p_real.y);
        fflush(stdout);

        // 等待前一个导航完成
        while (m_waypoints.size() > 0 && !targetErr) ;

        if (targetErr) {
            printf("Previous target failed, skipping.\n");fflush(stdout);
            targetErr = false;
            if (!m_waypoints.empty()) m_waypoints.pop();
            continue;
        }

        g_NaviInterface.setvisionrecenter();
        g_NaviInterface.setGoal(p_real);

        // 可选：记录路径
        IPoint iPoint = astarPlanner.GlobalToGrid(p_real.x, p_real.y);
        fullCoverageAlg.pathIPoint.push(iPoint);
    }

    // 返回起点
    clearNavigationStateAndStop();
}

/*void CNaviInterface::ChangeGlobalToGrid(double x1, double y1, double x2, double y2, double robotX, double robotY, int safeSize, int minSize)
{
    CAstar astarPlanner;
    IPoint tempPose;
    
    // Convert coordinates to grid points
    tempPose = astarPlanner.GlobalToGrid(x1, y1);
    int gridX1 = tempPose.x;
    int gridY1 = tempPose.y;
    
    tempPose = astarPlanner.GlobalToGrid(x2, y2);
    int gridX2 = tempPose.x;
    int gridY2 = tempPose.y;
    
    // Get robot's grid position
    IPoint curPose = astarPlanner.GlobalToGrid(robotX, robotY);
    int rob[2] = { curPose.x, curPose.y };
    
    // Create the full path
    CreateFullPath(gridX1, gridY1, gridX2, gridY2, rob, safeSize, minSize);
    
    printf("Create full path from (%d, %d) to (%d, %d) with robot at (%d, %d), successfully\n",
           gridX1, gridY1, gridX2, gridY2, rob[0], rob[1]);
    fflush(stdout);
}*/

vector<IPoint> CNaviInterface::ReadFullPathFromFile(const string& filepath) {
    vector<IPoint> path;
    ifstream infile(filepath);
    if (!infile.is_open()) {
        cerr << "Cannot open path file: " << filepath << endl;
        return path;
    }

    int x, y;
    char delim;
    while (infile >> x >> delim >> y) {
        IPoint p;
        p.x = x;
        p.y = y;
        path.push_back(p);
    }
    return path;
}

void NAVI_SetPlanFullPath(int algNum) { g_NaviInterface.setPlanFullPath(algNum); }
void NAVI_SetRoomVertex(int order, double x, double y) { g_NaviInterface.setRoomVertex(order, x, y); }
void NAVI_CancelPlanFullPath(void) { g_NaviInterface.cancelPlanFullPath(); }
void NAVI_SetSearchType(int searchType) { g_NaviInterface.setSearchType(searchType); };
void NAVI_SubGetMapFromMain(int* ip) { g_NaviInterface.subGetMapFromMain(ip); };
void NAVI_CreateFullPath(int x1, int y1, int x2, int y2, int* rob, int safesize, int minsize){ g_NaviInterface.CreateFullPath(x1, y1, x2, y2, rob, safesize, minsize); };
void NAVI_getsize_forFullRoad(int len, int safe,int mins, int* list) { g_NaviInterface.getsize_forFullRoad(len, safe, mins, list); };
void NAVI_NavigatePathByGridPoints(const vector<Pose>& gridPath){ g_NaviInterface.NavigatePathByGridPoints(gridPath); };
void NAVI_SetrobotId(int robotId){ g_NaviInterface.robotId = robotId;}
IPoint NAVI_GlobalToGrid(double x, double y) {
    return g_NaviInterface.astarPlanner.GlobalToGrid(x, y);
}
void NAVI_HandleCoopAvoidMessage(int commandId, int targetRobotId, int sourceRobotId,
                                 int seq, const double *dparams, int ndparams,
                                 const int8_t *iparams, int niparams,
                                 const uint8_t *bparams, int nbparams) {
    g_NaviInterface.handleCoopAvoidMessage(commandId, targetRobotId, sourceRobotId,
                                           seq, dparams, ndparams,
                                           iparams, niparams, bparams, nbparams);
}
