#include "ZigzagCoverage.h"

ZigzagCoverage::ZigzagCoverage()
{

}

ZigzagCoverage::~ZigzagCoverage()
{

}

bool ZigzagCoverage::detectFrontPlan(ProbMap &visMap, Pose &curpose, Pose &endpose, GridMap &lasermap, GridMap &visionMap, CAstar &astarPlanner, int* coverageState) {
    vector<NodeInfo> vtPlanPath;
    // endpose = curpose;

    // 检查前方是否有障碍物
    Pose tmppose = endpose;
    tmppose.x += astarPlanner.GGridSize * 10; // 假设前进十个网格的距离
    // 计算当前位置到目标点的距离
    double distance = LinAlg::DistancePose(endpose, tmppose);
    printf("The distance of curpose to endpose: %lf\n", distance);
    while (distance < 0.25)
    {
        printf("Too close! Add 5 GridSize!\n");
        tmppose.x += astarPlanner.GGridSize * 5;
    }
    IPoint robot = astarPlanner.GlobalToGrid(tmppose.x, tmppose.y);
    int deltax[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    int deltay[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    bool obstacleFront = false;
    printf("robot.x = %d, robot.y = %d\n", robot.x, robot.y);

    printf("detectFrontPlan Loading ...\n");
    for (int i = 0; i < 8; i++) {
        int posx = robot.x + deltax[i];
        int posy = robot.y + deltay[i];

        if (posx >= 0 && posx < astarPlanner.GMapWidth && posy >= 0 && posy < astarPlanner.GMapLength) {
            if (astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == Occupied ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == Near_Obstacle ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == danger ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == neardanger ||
                coverageState[posy * astarPlanner.GMapWidth + posx] == 3) {
                obstacleFront = true;
                break;
            }
        }
    }

    if (!obstacleFront) {
        // 前方没有障碍物，前进
        endpose.x = tmppose.x; // 前进十个网格的距离
        // 本轮检测结束，前进成功
        printf("Goal.x = %lf, Goal.y = %lf\n", endpose.x, endpose.y);
        printf("Go front\n");
        // path.push_back({newPose.x, newPose.y});
        // SetStartPose(newPose); // 更新当前位置
        return true;
    } else {
        // 前方有障碍物，结束函数
        printf("Obstacle front\n");
        return false;
    }
}

bool ZigzagCoverage::detectBackPlan(ProbMap &visMap, Pose &curpose, Pose &endpose, GridMap &lasermap, GridMap &visionMap, CAstar &astarPlanner, int* coverageState) {
    vector<NodeInfo> vtPlanPath;
    // endpose = curpose;

    // 检查后方是否有障碍物
    Pose tmppose = endpose;
    tmppose.x -= astarPlanner.GGridSize * 10; // 假设后退十个网格的距离
    // 计算当前位置到目标点的距离
    double distance = LinAlg::DistancePose(endpose, tmppose);
    printf("The distance of curpose to endpose: %lf\n", distance);
    while (distance < 0.25)
    {
        printf("Too close! Add 5 GridSize!\n");
        tmppose.x -= astarPlanner.GGridSize * 5;
    }
    IPoint robot = astarPlanner.GlobalToGrid(tmppose.x, tmppose.y);
    int deltax[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    int deltay[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    bool obstacleBack = false;
    printf("robot.x = %d, robot.y = %d\n", robot.x, robot.y);

    printf("detectBackPlan Loading ...\n");
    for (int i = 0; i < 8; i++) {
        int posx = robot.x + deltax[i];
        int posy = robot.y + deltay[i];

        if (posx >= 0 && posx < astarPlanner.GMapWidth && posy >= 0 && posy < astarPlanner.GMapLength) {
            if (astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == Occupied ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == Near_Obstacle ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == danger ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == neardanger ||
                coverageState[posy * astarPlanner.GMapWidth + posx] == 3) {
                obstacleBack = true;
                break;
            }
        }
    }

    if (!obstacleBack) {
        // 后方没有障碍物，前进
        endpose.x = tmppose.x; // 后退十个网格的距离
        // 本轮检测结束，后退成功
        printf("Goal.x = %lf, Goal.y = %lf\n", endpose.x, endpose.y);
        printf("Go back\n");
        // path.push_back({newPose.x, newPose.y});
        // SetStartPose(newPose); // 更新当前位置
        return true;
    } else {
        // 后方有障碍物，结束函数
        printf("Obstacle back\n");
        return false;
    }
}

bool ZigzagCoverage::detectLeftPlan(ProbMap &visMap, Pose &curpose, Pose &endpose, GridMap &lasermap, GridMap &visionMap, CAstar &astarPlanner, int* coverageState) {
    vector<NodeInfo> vtPlanPath;
    // endpose = curpose;

    // 检查左方是否有障碍物
    Pose tmppose = endpose;
    tmppose.y += astarPlanner.GGridSize * 10; // 假设左进十个网格的距离
    // 计算当前位置到目标点的距离
    double distance = LinAlg::DistancePose(endpose, tmppose);
    printf("The distance of curpose to endpose: %lf\n", distance);
    while (distance < 0.25)
    {
        printf("Too close! Add 5 GridSize!\n");
        tmppose.y += astarPlanner.GGridSize * 5;
    }
    IPoint robot = astarPlanner.GlobalToGrid(tmppose.x, tmppose.y);
    int deltax[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    int deltay[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    bool obstacleLeft = false;
    printf("robot.x = %d, robot.y = %d\n", robot.x, robot.y);

    printf("detectLeftPlan Loading ...\n");
    for (int i = 0; i < 8; i++) {
        int posx = robot.x + deltax[i];
        int posy = robot.y + deltay[i];

        if (posx >= 0 && posx < astarPlanner.GMapWidth && posy >= 0 && posy < astarPlanner.GMapLength) {
            if (astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == Occupied ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == Near_Obstacle ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == danger ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == neardanger ||
                coverageState[posy * astarPlanner.GMapWidth + posx] == 3) {
                obstacleLeft = true;
                break;
            }
        }
    }

    if (!obstacleLeft) {
        // 左方没有障碍物，前进
        endpose.y = tmppose.y; // 左进十个网格的距离
        // 本轮检测结束，左进成功
        printf("Goal.x = %lf, Goal.y = %lf\n", endpose.x, endpose.y);
        printf("Go left\n");
        // path.push_back({newPose.x, newPose.y});
        // SetStartPose(newPose); // 更新当前位置
        return true;
    } else {
        // 前方有障碍物，结束函数
        printf("Obstacle left\n");
        return false;
    }
}

bool ZigzagCoverage::detectRightPlan(ProbMap &visMap, Pose &curpose, Pose &endpose, GridMap &lasermap, GridMap &visionMap, CAstar &astarPlanner, int* coverageState) {
    vector<NodeInfo> vtPlanPath;
    // endpose = curpose;

    // 检查右方是否有障碍物
    Pose tmppose = endpose;
    tmppose.y -= astarPlanner.GGridSize * 10; // 假设右进十个网格的距离
    // 计算当前位置到目标点的距离
    double distance = LinAlg::DistancePose(endpose, tmppose);
    printf("The distance of curpose to endpose: %lf\n", distance);
    while (distance < 0.25)
    {
        printf("Too close! Add 5 GridSize!\n");
        tmppose.y -= astarPlanner.GGridSize * 5;
    }
    IPoint robot = astarPlanner.GlobalToGrid(tmppose.x, tmppose.y);
    int deltax[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    int deltay[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    bool obstacleRight = false;
    printf("robot.x = %d, robot.y = %d\n", robot.x, robot.y);

    printf("detectRightPlan Loading ...\n");
    for (int i = 0; i < 8; i++) {
        int posx = robot.x + deltax[i];
        int posy = robot.y + deltay[i];

        if (posx >= 0 && posx < astarPlanner.GMapWidth && posy >= 0 && posy < astarPlanner.GMapLength) {
            if (astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == Occupied ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == Near_Obstacle ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == danger ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == neardanger ||
                coverageState[posy * astarPlanner.GMapWidth + posx] == 3) {
                obstacleRight = true;
                break;
            }
        }
    }

    if (!obstacleRight) {
        // 右方没有障碍物，前进
        endpose.y = tmppose.y; // 右进十个网格的距离
        // 本轮检测结束，右进成功
        printf("Goal.x = %lf, Goal.y = %lf\n", endpose.x, endpose.y);
        printf("Go right\n");
        // path.push_back({newPose.x, newPose.y});
        // SetStartPose(newPose); // 更新当前位置
        return true;
    } else {
        // 右方有障碍物，结束函数
        printf("Obstacle right\n");
        return false;
    }
}