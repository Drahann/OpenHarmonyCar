#pragma once
#include "../math/LinAlg.h"
#include "Gridmap.h"
#include "Astarplanner.h"

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

class ZigzagCoverage
{
public:

public:
    bool    detectFrontPlan(ProbMap &visMap, Pose &curpose, Pose &endpose, GridMap &lasermap, GridMap &visionMap, CAstar &astarPlanner, int* coverageState);
    bool    detectBackPlan(ProbMap &visMap, Pose &curpose, Pose &endpose, GridMap &lasermap, GridMap &visionMap, CAstar &astarPlanner, int* coverageState);
    bool    detectLeftPlan(ProbMap &visMap, Pose &curpose, Pose &endpose, GridMap &lasermap, GridMap &visionMap, CAstar &astarPlanner, int* coverageState);
    bool    detectRightPlan(ProbMap &visMap, Pose &curpose, Pose &endpose, GridMap &lasermap, GridMap &visionMap, CAstar &astarPlanner, int* coverageState);

    ZigzagCoverage();
    ~ZigzagCoverage();
};