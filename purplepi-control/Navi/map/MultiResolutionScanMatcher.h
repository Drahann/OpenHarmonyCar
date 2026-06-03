#pragma once

#include "../type.h"
#include <vector>

#include "Gridmap.h"
#include "../gaussestimator/MultiGaussionEstimator.h"
#include "../gaussestimator/CMultiGauss.h"
#include <math.h>
#include "mymath.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "LinAlg.h"
#include <algorithm>

using namespace std;


#if 0
class particles
{
public:
	particles(void);
	~particles(void);

	Pose pose;
	double weight;
	double tempweight;
	double prop;
};
#endif
bool compareweight(const particles &a,const particles &b);

class MultiResolutionScanMatcher
{
public:
	MultiResolutionScanMatcher(void);
	~MultiResolutionScanMatcher(void);
	
	GridMap gm;
	GridMap dgm;

	void config();
	void setModel(GridMap &tmp);
	int  bestIndices(IntArray2D* pScores,int *bestidx);
	int  bestIndices(vector<IntArray2D*> &vtScores,int *bestidx);
	int  bestIndices(vector<IntArray2D*> &vtScores,int *bestidx,int &bestscore, int &similar_num,int &bestnum);
	int  HistogramFilterBestIndices(vector<IntArray2D*> &vtScores,int *bestidx,int &bestscore, int &similar_num,int &bestnum);
	void matchRaw(vector<Pose> &points,  Pose& posepriorxyt, double **pinv,
                           double xrange, double yrange, double thetaRange, double thetaResolution,Pose &resPose,vector<double> &data_flag,int mode);
	void HistogramFilter_matchRaw(vector<Pose> &points,  Pose& posepriorxyt,Pose &resPose,vector<double> &data_flag, int mode,vector<int> &id,int &navimode);
	void refineFunc(vector<Pose> &points,  double x, double y, double t, Pose& posepriorxyt, double **pinv,Pose &poseRes,int mode);
	void HistogramRefineFunc(vector<Pose> &points,  double x, double y, double t, Pose& posepriorxyt, double **pinv,Pose &poseRes,int mode);
	double rand_back(double i, double j);
	void initWeight();

	int decimate;
	bool debug ;

    	bool   refine;
    	double refine_initial_stepsize[3];
    	double refine_minimum_stepsize[3];
    	double refine_shrink_ratio;
    	int    refine_max_iterations;
		double VdHistogramFilterWeight[81];
		double VdHistogramFilterTempWeight[81];
		int ViHistogramFilterPoseTheta[81];
		int ratio;
		int m_ifrefine;
};

