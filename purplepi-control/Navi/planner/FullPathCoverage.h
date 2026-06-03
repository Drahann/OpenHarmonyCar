#pragma once
#include "Gridmap.h"
#include "Astarplanner.h"
#include "ZigzagCoverage.h"
#include "STCCoverage.h"

#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <math.h>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>

#define UNCOVERAGE_BUT_IN_ROOM 0
#define COVERGAED_AND_IN_ROOM 1
#define OBSTACLE 2
#define OUTOF_ROOM 3
#define WARNINGAREA 4

using namespace std;

class Graham
{
public:

public:
    int crossProduct(const IPoint& p, const IPoint& q, const IPoint& r);
    static bool compare(const IPoint& p1, const IPoint& p2);
    std::vector<IPoint> grahamScan(std::vector<IPoint>& points);
};

class FullCoverageAlg
{
public:
    // 全路径覆盖算法选择
    int algNum;
    // 手动方式选择房间顶点 - 分别对应左下点 x、左下点 y、右下点 x、右下点 y、右上点 x、右上点 y、左上点 x、左上点 y
    double roomVertex[8];
    // 区块化网格地图
    int* coverageGridMap;
    // 覆盖地图
    int* coverageState;
    // 是否已完成全覆盖
    bool isFinished;
    // 下一个导航目标点
    Pose nextGoal;
    // 全覆盖路径点记录序列
    queue<IPoint> pathIPoint;

    // 牛耕法类
    ZigzagCoverage zigzagCoverage;
    // 最小生成树算法类
    STCCoverage stcCoverage;

    // 凸包算法类
    Graham graham;
public:
    // 构造函数，用于初始化成员变量
    FullCoverageAlg();
    // 析构函数，在项目中并未用到，是空函数，后续可以增加一些功能
    ~FullCoverageAlg();
    // 识别地图中的障碍物，构建并返回障碍物点集
    vector<IPoint> getObstaclePoints(int GMapLength, int GMapWidth, GridState *m_pGridState);
    // 获取不同地图中的障碍物状态，构建并返回障碍物点集
    vector<IPoint> getObstaclePointsFromMultipleMaps(int GMapLength, int GMapWidth, GridState *m_pGridState, GridState *m_plaserGridState, GridState *m_pvisionGridState);
    // 使用凸包算法自动识别房间边界，返回房间顶点，由于实际房间可能不是凸多边形，因此项目中并未使用该方法识别房间边界
    vector<IPoint> autoGetRoomBoundary(int GMapLength, int GMapWidth, GridState* m_pGridState, bool &initFlag);
    // 手动输入房间顶点从而获取房间边界，注意输入顶点有顺序要求，且只能输入四个顶点，即房间区域只能为四边形
    vector<IPoint> manualGetRoomBoundary(CAstar& astarPlanner, bool& initFlag);
    // 计算向量的叉积（用于判断三角形的方向），该函数作为判断某一个点是否位于房间内的一个步骤
    int crossProduct(const IPoint& p1, const IPoint& p2, const IPoint& p3);
    // 判断点 p 是否在多边形 polygon 内部
    bool isPointInsideRoom(const std::vector<IPoint>& polygon, const IPoint& p);
    // 初始化覆盖地图，初始时覆盖地图中无已覆盖点
    bool initCoverageState(int GMapLength, int GMapWidth, GridState* m_pGridState, vector<IPoint> convexHull, bool &initFlag);
    // 用于标明某一点及附近区域已被覆盖
    void setValue(int GMapLength, int GMapWidth, IPoint currentIPoint);
    // 将覆盖状态从一维数组转换为二维地图，并将其写入 txt 文件，同时写入导航路径坐标序列
    void writeCoverageMapToFile(int GMapLength, int GMapWidth, const std::string& filename);
    // 通过定义最大搜索半径的方式寻找下一个未覆盖点
    bool findNextGoal_Radius(IPoint curIPoint, CAstar &astarPlanner);
    // 使用广度优先搜索算法寻找下一个未覆盖点
    bool findNextGoal_BFS(IPoint curIPoint, CAstar &astarPlanner);
    // 通过设定最大搜索深度的方式使用广度优先搜索算法寻找下一个未覆盖点
    bool findNextGoal_BFS_LimitDepth(IPoint curIPoint, CAstar &astarPlanner, int maxDepth);
    // 通过设定距离起始点最大距离的方式使用广度优先搜索算法寻找下一个未覆盖点
    bool findNextGoal_BFS_LimitDistance(IPoint curIPoint, CAstar &astarPlanner, int maxDistance);
    // 检查某一个点及其周围的点是否已被覆盖，项目中未使用，可作为扩展函数
    bool isCoveraged(Pose endpose, CAstar &astarPlanner);
    // 映射函数：将原始地图坐标（网格地图坐标）映射到区块化网格地图坐标
    void mapToGrid(int x, int y, int robotSize, int& gridX, int& gridY);
    // 逆映射函数：将区块化网格地图坐标映射回原始地图坐标（网格地图坐标）
    void mapToOriginal(int gridX, int gridY, int robotSize, int& x, int& y);
    // 将原始地图（网格地图）转换为区块化网格地图
    void convertToGridMap(int GMapLength, int GMapWidth, int robotSize);
    // 将区块化网格地图写入 txt 文件，但不写入路径点信息
    void saveGridMapToFile(int gridLength, int gridWidth, const std::string& filename);
    // 将 path 点序列及区块化网格地图写入 txt 文件
    void writeGridMapToFile(int gridMapLength, int gridMapWidth, std::vector<IPoint> path, const string& filename);
    // 检查网格地图中的某一坐标点是否合法
    bool ifIPointAroundLegal(IPoint &targetIPoint, CAstar &astarPlanner);
};
