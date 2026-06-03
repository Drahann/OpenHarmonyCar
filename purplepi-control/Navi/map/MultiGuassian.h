#pragma once

#include "../type.h"
#include <vector>

#include "Gridmap.h"

using namespace std;
class MultiGuassian
{
public:
	MultiGuassian(void);
	~MultiGuassian(void);
	
	GridMap gm;
	GridMap dgm;
	int decimate ;
	void setModel(GridMap &gm);
	int  bestIndices(IntArray2D* pScores,int *bestidx);
	int  bestIndices(vector<IntArray2D*> &vtScores,int *bestidx);
	void matchRaw(vector<Pose> &points,  Pose& posepriorxyt, double **pinv,
                           double xrange, double yrange, double thetaRange, double thetaResolution,Pose &resPose);
};

