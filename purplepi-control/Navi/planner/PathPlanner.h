#pragma once


#include "../map/Gridmap.h"
#include "../math/LinAlg.h"
#include "WaveFront.h"


#include <vector>

using namespace std;

typedef struct tagRect
{
	int x0;
	int y0;
	int x1;
	int y1;
}MyRect;
class ConfigurationSpaceData
    {
	public:
        int maxCost;

        // GridMap representation (bigger than the other elements)
        GridMap gm;

        // dimensions below apply for:
        BYTE *cfgSpace;

        int width;
        int height;
    };


 class WavefrontResults
    {
	public:
        ConfigurationSpaceData cspace;
        double radius;
        double *wavefront;
        double target[2];
        double targetDist;
        BYTE *reachable;
        vector<vector<double> > path;
    };
    

typedef struct tagpathplanconfig
	{

		double troughWidth;
		double upsample ;
		double maxCost ;
		double minCellCost;

	}plannerconfig;

class PathPlanner
{
private:


	


public:

	double robot_geometry_radius; 
	double robot_geometry_width;
	double pathPlanning_troughWidth;

	double pathPlanning_upsample ;

	double pathPlanning_maxCost ;

	double pathPlanning_minCellCost;

	PathPlanner(void);
	~PathPlanner(void);

	void setConfig(plannerconfig config,double robot_width, double robot_radius);
	void getVisBoundingBox(GridMap &map,MyRect &bounding);
	bool plan2(Pose thegoal, GridMap &visMap, Pose curpose);
	bool calculateBasePath(GridMap& terrain_map,
                                                   Pose goal,
                                                   double troughWidth,
                                                   double radius,
                                                   Pose &centerxy,WavefrontResults& res);
	bool renderConfigurationSpace(GridMap &om,
                                                          Pose &centerxy,
                                                           Pose &goal,
                                                           double troughWidth,
                                                           double cfgRadius,ConfigurationSpaceData &res);
	 void drawTrough(GridMap& gm,
            int map[],
            int width,
            int height,
            double mpp,
            Pose &centerxy,
             Pose &goal,
            double troughWidth);
	 bool computeGoalTarget(GridMap &gm, Pose centerxy,
                                     BYTE reachable[], Pose goal,double res[3]);
	 bool getWavefront(GridMap &gm, double target[], Pose centerxy,double **wavefront);

	 bool descendWavefront(int pos[], int goal[],
            int width, int height,
            double wave[],vector<vector<int> > &bestPath);
	 bool descendWavefront(int pos[], int goal[] ,
                                                   int width, int height,
                                                   double wave[],
                                                  int neighbors[8][2],vector<vector<int> > &bestPath);

	 bool computeWavefrontPath(ConfigurationSpaceData& cspace,
                                      double target[],
                                      double centerxy[],
                                      double wavefront[],vector<vector<double> > &path);

	 bool plan2(Pose thegoal, GridMap &visMap, Pose curpose, vector<vector<double> > &path);


	// bool SaveMap(GridMap &gm ,const char* strFileName);

};

