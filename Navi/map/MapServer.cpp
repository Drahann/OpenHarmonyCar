
#include "MapServer.h"
int bSaveMapDone = 0;
MapServer::MapServer(void) {
    robotDiameter = 0.1;
    debug = false;

    range = 15;
    resolution = 0.05;
    bHaveWall = false;
}

MapServer::~MapServer(void) {}

void MapServer::SetConfig(double _range, double _resolution) {
    robotDiameter = 0.1;
    debug = false;
    range = _range;
    resolution = _resolution;
}

bool MapServer::loadMap(const char *fileName,
                        vector<Pose> &vtWallPos) //ƥ��֮ǰ��Ҫloadmap
{
    ifstream infile;
    infile.open(fileName, ios::in);

    if (!infile)
        return false;

    double _range;
    double _resolution;
    double meterPer;
    int nwidth;
    int nheigh;
    double x0;
    double y0;

    infile >> _range;
    infile >> _resolution;
    infile >> nheigh;
    infile >> nwidth;
    infile >> meterPer;
    infile >> x0;
    infile >> y0;

    if (nheigh <= 0 || nwidth <= 0 || meterPer < 0.00000001) {
        printf("ERR: Wrong Map File Format");
        return false;
    }

    range = _range;
    resolution = _resolution;

    int laserwidth = 6 / meterPer;
    int laserheigh = 6 / meterPer;
    int visionwidth = 6 / meterPer;
    int visionheigh = 6 / meterPer;

    globalBinaryMap.makePixels(x0, y0, nwidth, nheigh, meterPer, (BYTE)0,
                               false); // saving astar path data
    globalGaussianMap.makePixels(x0, y0, nwidth, nheigh, meterPer, (BYTE)0,
                                 false); // for scan match
    globalWallMap.makePixels(x0, y0, nwidth, nheigh, meterPer, (BYTE)0,
                             false); // useless
    laserMap.makePixels(x0, y0, laserwidth, laserheigh, meterPer, (BYTE)0,
                        false); // for astar and dwa
    astarMap.makePixels(x0, y0, nwidth, nheigh, meterPer, 0.5,
                        false); // for astar and dwa
    globalProbMap.makePixels(x0, y0, nwidth, nheigh, meterPer, 0.5,
                             false); // for autoupdate and manualupdate
    visionMap.makePixels(x0, y0, visionwidth, visionheigh, meterPer, (BYTE)0,
                         false); // for astar and dwa
    globalcorrectionMap.makePixels(x0, y0, nwidth, nheigh, meterPer, 0.5,
                                   false); // for correction
    globalcorrectionMap2.makePixels(x0, y0, nwidth, nheigh, meterPer, 0.5,
                                    false); // for correction
    autochargemap.makePixels(-30, -30, 1200, 1200, meterPer, 0.5,
                             false); // for autocharge
    globalVisionMap.makePixels(x0, y0, nwidth, nheigh, meterPer, (BYTE)0,
                               false); // saving visiondata

    testpathmap.makePixels(x0, y0, nwidth, nheigh, meterPer, (BYTE)0, false);

    LUT lut;
    globalGaussianMap.makeGaussianLUT(1.0, 0, 1.0 / LinAlg::sq(0.06),
                                      lut); // 255,127,15

    int index = 0;
    while (!infile.eof()) {

        int nstatus;
        infile >> nstatus;

        int i = (int)(index / nwidth);

        int iin = index - i * nwidth;

        i = i + 1;

        if (index < globalBinaryMap.width * globalBinaryMap.height) {
            if ((BYTE)nstatus == 1) // BYTE 255
            {
                globalBinaryMap.data[index] = 0;
                globalWallMap.data[index] = (BYTE)255;
                bHaveWall = true;

            } else if ((BYTE)nstatus == 2) {
                globalBinaryMap.data[index] = (BYTE)nstatus;
                globalWallMap.data[index] = (BYTE)255;
                astarMap.data[index] = (BYTE)nstatus;
                bHaveWall = true;

            } else {
                globalBinaryMap.data[index] = (BYTE)nstatus;
                globalVisionMap.data[index] = (BYTE)nstatus;
                testpathmap.data[index] = (BYTE)nstatus;
                // astarMap.data[index] = (BYTE)nstatus;
                if ((BYTE)nstatus == 0) {
                    globalProbMap.data[index] = 0.5;
                    astarMap.data[index] = 0.5;
                    globalcorrectionMap.data[index] = 0.5;
                    globalcorrectionMap2.data[index] = 0.5;
                } else {
                    globalProbMap.data[index] = 1;
                    astarMap.data[index] = 1;
                    globalcorrectionMap.data[index] = 1;
                    globalcorrectionMap2.data[index] = 1;
                }
            }

            if (globalBinaryMap.data[index] ==
                (BYTE)255) { //��ϸ��drawDot�������ڻ�ͼʱ�����ڱȽϵĵ���դ���е㣬���Դ˴����е㣬����߸�˹��ͼ�ľ���
                             //����������ĵ㣬��ʹ���ɵĸ�˹��ͼ��������255�ĵ㣬�һ������������ڵ�127�ĵ㣬�Ҹ�˹�����С����Ϊ127�Ǹ�˹���ĵ�2����
                globalGaussianMap.drawDot(
                    (iin + 0.5) * globalBinaryMap.metersPerPixel +
                        globalBinaryMap
                            .x0 //�����drawDot����(x0,y0)�����ô������ص�����½ǣ������޸�Ϊ���м�
                    ,
                    (i + 0.5) * globalBinaryMap.metersPerPixel +
                        globalBinaryMap.y0,
                    lut, lut.length);
            }
        }

        index++;
    }

    infile.close();

    if (bHaveWall) {

        for (int y = 0; y < globalWallMap.height; y++) {
            for (int x = 0; x < globalWallMap.width; x++) {
                if (globalWallMap.data[y * globalWallMap.width + x] ==
                    (BYTE)255) {
                    Pose wall;

                    wall.x =
                        globalWallMap.x0 + x * globalWallMap.metersPerPixel;
                    wall.y =
                        globalWallMap.y0 + y * globalWallMap.metersPerPixel;
                    vtWallPos.push_back(wall);
                }
            }
        }
    }
    return true;
}

void MapServer::returnLaserMap(Pose pos, vector<Pose> &points,
                               vector<Pose> &visionwall) {
    Pose lastp = pos;
    bool bFirst = true;

    int ix =
        (int)((pos.x - 3.0 - globalBinaryMap.x0) / laserMap.metersPerPixel);
    int iy =
        (int)((pos.y - 3.0 - globalBinaryMap.y0) / laserMap.metersPerPixel);
    laserMap.x0 = globalBinaryMap.x0 + ix * laserMap.metersPerPixel;
    laserMap.y0 = globalBinaryMap.y0 + iy * laserMap.metersPerPixel;
    laserMap.fill(0);

    //�����֮�仭��
    for (int i = 0; i < points.size(); i++) {

        Pose temp = points.at(i);
        /*
        if(0 >= temp.y )
        {
                continue;
        }
        */

        Pose p = LinAlg::transform(pos, temp);

        if (bFirst == true || LinAlg::DistancePose(p, lastp) > 0.3) {
            laserMap.setValue(p.x, p.y, (BYTE)255);
        } else {
            laserMap.setValue(p.x, p.y, (BYTE)255);
        }

        lastp = p;
        bFirst = false;
    }
}
void MapServer::returnTempVisMap(Pose pos, vector<Pose> points,
                                 GridMap &tempMap)

{
    globalBinaryMap.copy(tempMap); //���ƶ�ֵ��ͼ
    Pose lastp;
    bool bFirst = true;

    // Set 0  between robot and laser data
    for (int i = 0; i < points.size(); i++) {
        Pose temp = points.at(i);

        Pose p = LinAlg::transform(pos, temp);

        tempMap.drawLine(p.x, p.y, pos.x, pos.y, (BYTE)0);
    }

    vector<Pose> p;
    returnLaserMap(pos, points, p);

    combineMap(tempMap, globalWallMap);
    combineMap(tempMap, laserMap);

    double xyt[3];
    xyt[0] = pos.x;
    xyt[1] = pos.y;
    xyt[2] = pos.theta;
    GridMapRenderer::makeVisibilityMap(tempMap, xyt, range / 2);
    return;
}
void MapServer::returnCleanMap(GridMap &tempMap)

{
    globalBinaryMap.copy(tempMap);
    combineMap(tempMap, globalWallMap);

    return;
}

void MapServer::combineMap(GridMap &map1, GridMap &map2) {
    if (map1.data == NULL)
        return;
    if (map2.data == NULL)
        return;

    if (map1.x0 != map2.x0 || map1.y0 != map2.y0 || map1.width != map2.width ||
        map1.height != map2.height) {
        // System.out.println("combine resize "+map2.width+" "+map2.height);
        map1.resizeMeters(map2.x0, map2.y0, map2.width * map2.metersPerPixel,
                          map2.height * map2.metersPerPixel, false);
    }

    for (int y = 0; y < map2.height; y++) {
        for (int x = 0; x < map2.width; x++) {
            if (map2.data[y * map2.width + x] == (BYTE)255) {
                map1.data[y * map2.width + x] = (BYTE)255;
            }
        }
    }

    return;
}

GridMap &MapServer::getBinaryMap() { return globalBinaryMap; }

GridMap &MapServer::getGaussianMap() { return globalGaussianMap; }

bool MapServer::localUpdate(Pose robotpose, vector<Pose> &points) {
    return addToGlobalMap(robotpose, points);
}

//�˺���������doSLAM�е�m_pmsSLAM��m_pmsSLAM�Ǹ�MapServerָ��
bool MapServer::globalUpdate(Graph &g, GridMap &gm) {
    return makeNewGridMap(g, gm);
}

bool MapServer::globalUpdate_test(Graph &g, GridMap &gm) {
    return makeNewGridMap_test(g, gm);
}

bool MapServer::makeNewGridMap(
    Graph &g,
    GridMap &
        gm) // g��������Ԫ����GNode��ÿ��GNode�������㣬���һ��GNode�����µĻ�����λ��
{
    //�����Ƶ�ͼ����ͼ���ĵ�Ϊ������λ��
    if (globalBinaryMap.data ==
        NULL) //��������ڶ����Ƶ�ͼ�����½�һ�����½���ͼ�����ĵ㣬��g�����һ��GNode�еĻ�����λ��
    {
        globalBinaryMap.makePixels(
            g.nodes.at(g.nodes.size() - 1).state.x - range,
            g.nodes.at(g.nodes.size() - 1).state.y - range,
            (int)(2 * range / resolution), (int)(2 * range / resolution),
            resolution, 0, false);
    }
    return true;
}

bool MapServer::makeNewGridMap_test(
    Graph &g,
    GridMap &
        gm) // g��������Ԫ����GNode��ÿ��GNode�������㣬���һ��GNode�����µĻ�����λ��
{
    //�����Ƶ�ͼ����ͼ���ĵ�Ϊ������λ��
    if (globalBinaryMap.data ==
        NULL) //��������ڶ����Ƶ�ͼ�����½�һ�����½���ͼ�����ĵ㣬��g�����һ��GNode�еĻ�����λ��
    {
        globalBinaryMap.makePixels(
            g.nodes.at(g.nodes.size() - 1).state.x - range,
            g.nodes.at(g.nodes.size() - 1).state.y - range,
            (int)(2 * range / resolution), (int)(2 * range / resolution),
            resolution, 0, false);
    } else {
        //  globalBinaryMap.makePixels(globalBinaryMap.x0, globalBinaryMap.y0,
        //  globalBinaryMap.width, globalBinaryMap.height, resolution, (BYTE)0,
        //  false);
        globalBinaryMap.fill(0);
    }

    int num = g.nodes.size();

    for (int i = 0; i < num; i++) {
        GNode gn = g.nodes.at(i); //��g.node�����е�GNode����ȡ����
        vector<Pose> p;

        gn.getAttribute("points",
                        p); //�ѵ�ǰGNode�л�����λ���µļ����Ļ���������ϵ����ȡ����

        addGridMap(globalBinaryMap, gn.state, p);
    }
    return true;
}

bool MapServer::addToGlobalMap(Pose pose, vector<Pose> &points) {
    // expand global Map
    double width = globalBinaryMap.width *
                   globalBinaryMap.metersPerPixel; //�������е�ͼ�ĳ���
    double height = globalBinaryMap.height * globalBinaryMap.metersPerPixel;
    bool expandX = false, expandY = false; //�Ƿ���Ҫ��չ
    double x0 = globalBinaryMap.x0; //���е�ͼ�����½�����
    double y0 = globalBinaryMap.y0;

    double edge = 30;

    //�ж��Ƿ���Ҫ���е�ͼ��չ
    if (pose.x - globalBinaryMap.x0 >
        width -
            edge) { //��������X�������ߣ���X������߽�С��range
                    // if(debug)System.out.println("expand x");
        width = width + edge;
        expandX = true;
    }

    if (pose.y - globalBinaryMap.y0 >
        height -
            edge) { //��������Y�������ߣ���Y������߽�С��range
                    //	if(debug)System.out.println("expand Y");
        height = height + edge;
        expandY = true;
    }

    if (pose.x <
        globalBinaryMap.x0 +
            edge) //��������X�������ߣ���X������߽�С��range
    {

        //	if(debug)System.out.println("recenter x");

        // x0 -= range/2;
        //
        x0 -= edge;
        width = width + edge;
        expandX = true;
    }

    if (pose.y <
        globalBinaryMap.y0 +
            edge) { //��������Y�������ߣ���Y������߽�С��range

        //	if(debug)System.out.println("recenter Y");

        y0 -= edge;
        height = height + edge;
        expandY = true;
    }

    //��Ҫ��չ
    if (expandX || expandY) {
        // globalBinaryMap.resizeMeters(x0, y0, width, height,
        // true);//���½�����ͼ����ԭ����ͼ�еĵ�Ž�ȥ
        globalBinaryMap.x0 = x0;
        globalBinaryMap.y0 = y0;
        globalBinaryMap.width = width / globalBinaryMap.metersPerPixel;
        globalBinaryMap.height = height / globalBinaryMap.metersPerPixel;
    }

    // addGridMap(globalBinaryMap, pose, points);//��ɨ��ĵ����ӽ�ȥ
    return expandX || expandY;
}
bool MapServer::addGridMap(GridMap &gridMap, Pose pose, vector<Pose> &points) {
    Pose lastp;

    lastp.x = 0;
    lastp.y = 0;
    lastp.theta = 0;

    for (int i = 0; i < points.size(); i++) {
        Pose p = points.at(i); //�����Ѽ����Ļ���������ϵ����ȡ����

        p = LinAlg::transform(pose, p); //ͨ��������λ�ˣ�ת������������ϵ����

        if (LinAlg::DistancePose(p, lastp) > 0.3) //����һ����������0.3
        {
            gridMap.setValue(
                p.x, p.y,
                (BYTE)255); //���p��ûԽ�磬����gridMap�е�data��������Ӧ����λ255/�з�����-1
        } else              //���������0.3
        {
            gridMap.drawLine(
                p.x, p.y, lastp.x, lastp.y,
                (BYTE)255); //������仭�ߣ����data����������Ϊ��ϸ�ߣ��뻭�����Ǹ���һ��
        }

        lastp = p;
    }

    return true;
}

bool MapServer::addProbMap(ProbMap &probMap, Pose pose, vector<Pose> &points) {
    Pose lastp;

    lastp.x = 0;
    lastp.y = 0;
    lastp.theta = 0;

    for (int i = 0; i < points.size(); i++) {
        Pose p = points.at(i);

        p = LinAlg::transform(pose, p);

        if (LinAlg::DistancePose(p, lastp) > 0.15) {
            probMap.setValue(p.x, p.y, 2.0);
        } else {
            probMap.drawLine(p.x, p.y, lastp.x, lastp.y, 2.0);
        }
        probMap.drawLine(p.x, p.y, pose.x, pose.y, 0.7);
        probMap.setValue(p.x, p.y, 2.0);

        lastp = p;
    }

    return true;
}
/****************************************************************************************************************/
bool MapServer::ModifyProbMap(ProbMap &probMap, Pose pose,
                              vector<Pose> &points) {
    Pose lastp;

    lastp.x = 0;
    lastp.y = 0;
    lastp.theta = 0;

    for (int i = 0; i < points.size(); i++) {
        Pose p = points.at(i);

        p = LinAlg::transform(pose, p);

        if (LinAlg::DistancePose(p, lastp) > 0.15) {
            probMap.setValue(p.x, p.y, 0.2);
        } else {
            probMap.drawLine(p.x, p.y, lastp.x, lastp.y, 0.1);
        }
        probMap.drawLine(p.x, p.y, pose.x, pose.y, 0.1);
        probMap.setValue(p.x, p.y, 0.2);
        lastp = p;
    }

    return true;
}
/****************************************************************************************************************/
bool MapServer::saveMap(const char *fileName) {

    ofstream outFile;
    printf("save = %s\n", fileName);
    outFile.open(fileName, ios::out);

    if (!outFile) {
        return false;
    }

    outFile << range;
    outFile << ' ';
    outFile << resolution;
    outFile << ' ';
    outFile << globalBinaryMap.height;
    outFile << ' ';
    outFile << globalBinaryMap.width;
    outFile << ' ';
    outFile << globalBinaryMap.metersPerPixel;
    outFile << ' ';
    outFile << globalBinaryMap.x0;
    outFile << ' ';
    outFile << globalBinaryMap.y0;
    outFile << endl;

    for (int j = 0; j < globalBinaryMap.height; j++) {
        for (int i = 0; i < globalBinaryMap.width; i++) {
            int status = 0;
            status = globalBinaryMap.data[j * globalBinaryMap.width + i];
            if (status == 255) {
                outFile << -1;
            } else {
                outFile << 0;
            }

            outFile << ' ';
        }
        outFile << endl;
    }

    outFile.close();
    return true;
}

bool MapServer::saveMap_Astar() {

    ofstream outFile;
    outFile.open("/mnt/cf/mapfile/testmap_astar.txt", ios::out);

    if (!outFile) {
        printf("open error\n");
        return false;
    }

    outFile << range;
    outFile << ' ';
    outFile << resolution;
    outFile << ' ';
    outFile << globalBinaryMap.height;
    outFile << ' ';
    outFile << globalBinaryMap.width;
    outFile << ' ';
    outFile << globalBinaryMap.metersPerPixel;
    outFile << ' ';
    outFile << globalBinaryMap.x0;
    outFile << ' ';
    outFile << globalBinaryMap.y0;
    outFile << endl;

    for (int j = 0; j < globalBinaryMap.height; j++) {
        for (int i = 0; i < globalBinaryMap.width; i++) {
            int status = 0;
            status = globalBinaryMap.data[j * globalBinaryMap.width + i];
            if (status == 255) {
                outFile << -1;
            } else {
                if (2 == status) {
                    outFile << 2;
                } else {
                    outFile << 0;
                }
            }

            outFile << ' ';
        }
        outFile << endl;
    }

    outFile.close();
    return true;
}

bool MapServer::saveMap_Vision() {

    ofstream outFile;
    outFile.open("/mnt/cf/mapfile/vision.txt", ios::out);

    if (!outFile) {
        printf("open error\n");
        return false;
    }

    outFile << range;
    outFile << ' ';
    outFile << resolution;
    outFile << ' ';
    outFile << globalVisionMap.height;
    outFile << ' ';
    outFile << globalVisionMap.width;
    outFile << ' ';
    outFile << globalVisionMap.metersPerPixel;
    outFile << ' ';
    outFile << globalVisionMap.x0;
    outFile << ' ';
    outFile << globalVisionMap.y0;
    outFile << endl;

    for (int j = 0; j < globalVisionMap.height; j++) {
        for (int i = 0; i < globalVisionMap.width; i++) {
            int status = 0;
            status = globalVisionMap.data[j * globalVisionMap.width + i];
            if (status == 255) {
                outFile << -1;
            } else {
                if (2 == status) {
                    outFile << 2;
                } else {
                    outFile << 0;
                }
            }

            outFile << ' ';
        }
        outFile << endl;
    }

    outFile.close();
    return true;
}

bool MapServer::saveMap_gauss() {

    ofstream outFile;
    outFile.open("/mnt/cf/mapfile/testmap_gauss.txt", ios::out);

    if (!outFile) {
        printf("open error\n");
        return false;
    }

    outFile << range;
    outFile << ' ';
    outFile << resolution;
    outFile << ' ';
    outFile << globalBinaryMap.height;
    outFile << ' ';
    outFile << globalBinaryMap.width;
    outFile << ' ';
    outFile << globalBinaryMap.metersPerPixel;
    outFile << ' ';
    outFile << globalBinaryMap.x0;
    outFile << ' ';
    outFile << globalBinaryMap.y0;
    outFile << endl;

    for (int j = 0; j < globalGaussianMap.height; j++) {
        for (int i = 0; i < globalGaussianMap.width; i++) {
            int status = 0;
            status = globalGaussianMap.data[j * globalGaussianMap.width + i];
            if (status == 255) {
                outFile << -1;
            } else {
                outFile << 0;
            }

            outFile << ' ';
        }
        outFile << endl;
    }

    outFile.close();
    return true;
}

bool MapServer::saveMap() {

    ofstream outFile;
    // printf("save = %s\n",fileName);
    outFile.open("/mnt/cf/mapfile/optmap.txt", ios::out);

    if (!outFile) {
        printf("open error\n");
        return false;
    }

    outFile << range;
    outFile << ' ';
    outFile << resolution;
    outFile << ' ';
    outFile << globalBinaryMap.height;
    outFile << ' ';
    outFile << globalBinaryMap.width;
    outFile << ' ';
    outFile << globalBinaryMap.metersPerPixel;
    outFile << ' ';
    outFile << globalBinaryMap.x0;
    outFile << ' ';
    outFile << globalBinaryMap.y0;
    outFile << endl;

    for (int j = 0; j < globalBinaryMap.height; j++) {
        for (int i = 0; i < globalBinaryMap.width; i++) {
            int status = 0;
            status = globalBinaryMap.data[j * globalBinaryMap.width + i];
            if (status == 255) {
                outFile << -1;
            } else {
                outFile << 0;
            }

            outFile << ' ';
        }
        outFile << endl;
    }

    outFile.close();
    return true;
}

bool MapServer::saveProbMap(const char *fileName) {
    ofstream outFile;
    outFile.open(fileName, ios::out);

    printf("save = %s\n", fileName);

    if (!outFile.is_open()) {
        printf("openerror\n");
        return false;
    }

    outFile << range;
    outFile << ' ';
    outFile << resolution;
    outFile << ' ';
    outFile << globalProbMap.height;
    outFile << ' ';
    outFile << globalProbMap.width;
    outFile << ' ';
    outFile << globalProbMap.metersPerPixel;
    outFile << ' ';
    outFile << globalProbMap.x0;
    outFile << ' ';
    outFile << globalProbMap.y0;
    outFile << endl;

    for (int j = 0; j < globalProbMap.height; j++) {
        for (int i = 0; i < globalProbMap.width; i++) {
            double status = 0;
            status = globalProbMap.data[j * globalProbMap.width + i];
            if (status > 0.5) {
                outFile << -1;
            } else {
                outFile << 0;
            }

            outFile << ' ';
        }
        outFile << endl;
    }

    outFile.close();

    // TODO
    string name = fileName;
    name += ".txt";
    outFile.open(name.c_str(), ios::out);
    if (!outFile) {
        printf("open1error\n");
        return false;
    }

    outFile << range;
    outFile << ' ';
    outFile << resolution;
    outFile << ' ';
    outFile << globalProbMap.height;
    outFile << ' ';
    outFile << globalProbMap.width;
    outFile << ' ';
    outFile << globalProbMap.metersPerPixel;
    outFile << ' ';
    outFile << globalProbMap.x0;
    outFile << ' ';
    outFile << globalProbMap.y0;
    outFile << endl;

    for (int j = 0; j < globalProbMap.height; j++) {
        for (int i = 0; i < globalProbMap.width; i++) {
            double status = 0;
            status = globalProbMap.data[j * globalProbMap.width + i];
            if (status > 0.5) {
                outFile << 1;
            } else {
                outFile << 0;
            }
        }
        outFile << endl;
    }
    outFile.close();
    return true;
    ////////////////
}

bool MapServer::saveMap_pathcheck() {

    ofstream outFile;
    outFile.open("/mnt/cf/mapfile/pathcheck.txt", ios::out);

    if (!outFile) {
        printf("openerror\n");
        return false;
    }

    outFile << range;
    outFile << ' ';
    outFile << resolution;
    outFile << ' ';
    outFile << testpathmap.height;
    outFile << ' ';
    outFile << testpathmap.width;
    outFile << ' ';
    outFile << testpathmap.metersPerPixel;
    outFile << ' ';
    outFile << testpathmap.x0;
    outFile << ' ';
    outFile << testpathmap.y0;
    outFile << endl;

    for (int j = 0; j < testpathmap.height; j++) {
        for (int i = 0; i < testpathmap.width; i++) {
            int status = 0;
            status = testpathmap.data[j * testpathmap.width + i];
            if (status == 255) {
                outFile << -1;
            } else {
                if (status == 2) {
                    outFile << 2;
                } else {
                    outFile << 0;
                }
            }

            outFile << ' ';
        }
        outFile << endl;
    }

    outFile.close();
    return true;
}

bool MapServer::saveMap_Modify() {
    if ((NULL == globalcorrectionMap.data) ||
        (0 == globalcorrectionMap.height) || (0 == globalcorrectionMap.width) ||
        (NULL == globalcorrectionMap2.data) ||
        (0 == globalcorrectionMap2.height) ||
        (0 == globalcorrectionMap2.width)) {
        printf("MapB is empty\n");
        return false;
    }
    ofstream outFile;
    outFile.open("/mnt/cf/mapfile/updateCopymapB.txt", ios::out);

    if (!outFile) {
        printf("open error\n");
        return false;
    }

    outFile << range;
    outFile << ' ';
    outFile << resolution;
    outFile << ' ';
    outFile << globalcorrectionMap.height;
    outFile << ' ';
    outFile << globalcorrectionMap.width;
    outFile << ' ';
    outFile << globalcorrectionMap.metersPerPixel;
    outFile << ' ';
    outFile << globalcorrectionMap.x0;
    outFile << ' ';
    outFile << globalcorrectionMap.y0;
    outFile << endl;

    for (int j = 0; j < globalcorrectionMap.height; j++) {
        for (int i = 0; i < globalcorrectionMap.width; i++) {
            double status = 0;
            status =
                globalcorrectionMap.data[j * globalcorrectionMap.width + i];
            if (status > 0.5) {
                outFile << -1;
            } else {
                outFile << 0;
            }

            outFile << ' ';
        }
        outFile << endl;
    }

    outFile.close();

    outFile.open("/mnt/cf/mapfile/updateCopymapC.txt", ios::out);

    if (!outFile) {
        printf("open error\n");
        return false;
    }

    outFile << range;
    outFile << ' ';
    outFile << resolution;
    outFile << ' ';
    outFile << globalcorrectionMap2.height;
    outFile << ' ';
    outFile << globalcorrectionMap2.width;
    outFile << ' ';
    outFile << globalcorrectionMap2.metersPerPixel;
    outFile << ' ';
    outFile << globalcorrectionMap2.x0;
    outFile << ' ';
    outFile << globalcorrectionMap2.y0;
    outFile << endl;

    for (int j = 0; j < globalcorrectionMap2.height; j++) {
        for (int i = 0; i < globalcorrectionMap2.width; i++) {
            double status = 0;
            status =
                globalcorrectionMap2.data[j * globalcorrectionMap2.width + i];
            if (status > 0.5) {
                outFile << -1;
            } else {
                outFile << 0;
            }

            outFile << ' ';
        }
        outFile << endl;
    }

    outFile.close();

    bSaveMapDone = 1;
    return true;
}
bool MapServer::saveMap_CallBack() {
    if (1 == bSaveMapDone) {
        return true;
    } else {
        return false;
    }
}
void MapServer::setSaveMapDoneStatus(int status) { bSaveMapDone = status; }
