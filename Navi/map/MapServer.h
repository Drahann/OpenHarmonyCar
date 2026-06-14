#pragma once

#include "../type.h"
#include "Gridmap.h"

#include "../planner/GridMapRenderer.h"
#include <sstream>
#include <fstream>
#include "../math/LinAlg.h"
#include <vector>

#include "Graph.h"

#include "ScanMatcher.h"


using namespace std;

class MapServer
{
public:
	GridMap globalBinaryMap;
	GridMap globalGaussianMap;
	GridMap globalWallMap;
    GridMap laserMap;
	ProbMap astarMap;
	ProbMap globalProbMap;
	GridMap visionMap;
	ProbMap globalcorrectionMap;
    ProbMap globalcorrectionMap2;
    ProbMap autochargemap;
    GridMap globalVisionMap;

    GridMap testpathmap;
    
	ProbMap emptyMap;

	
	double range;
	double resolution;
	double robotDiameter ;
	bool debug ;
	bool  bHaveWall;

	ContourExtractor contourExtractor;


public:
	MapServer(void);
	MapServer(double _range, double _resolution){
		robotDiameter = 0.1;
		debug = false;
		range = _range;
		resolution = _resolution;
		bHaveWall = false;
	};
	~MapServer(void);
	bool loadMap(const char* fileName,vector<Pose> &vtWallPos);
	void SetConfig(double _range, double _resolution);

	GridMap& getBinaryMap();
	GridMap& getGaussianMap();

	bool localUpdate(Pose robotpose, vector<Pose> &points);
	bool addToGlobalMap(Pose pose, vector<Pose> &points) ;

	bool addProbMap(ProbMap &probMap, Pose pose, vector<Pose> &points);
	bool ModifyProbMap(ProbMap &probMap, Pose pose, vector<Pose> &points);
	bool addGridMap(GridMap &gridMap, Pose pose, vector<Pose> &points);
	void returnTempVisMap(Pose pos,vector<Pose> points, GridMap &tempMap);
	void returnLaserMap(Pose pos, vector<Pose> &points,vector<Pose> &visionwall);
	void returnCleanMap( GridMap &tempMap);
	bool makeNewGridMap(Graph &g,GridMap &gm);

	/****************/
	bool makeNewGridMap_test(Graph &g,GridMap &gm);
	
	/******************/
	void combineMap(GridMap &map1, GridMap &map2);
	bool globalUpdate(Graph &g, GridMap &gm);
	
   /****************/
	bool globalUpdate_test(Graph &g, GridMap &gm);
   /**********************/
	
	bool saveMap(const char* fileName);
	
	bool saveMap();

	
	bool saveProbMap(const char* fileName);
	bool saveZipedMap(const char* fileName);
	bool loadZipedMap(const char* fileName, const char* outMapFileName);


	/*****************/
	bool saveMap_Astar();
    bool saveMap_Vision();
	/**************/
	bool saveMap_gauss();
	
	bool saveMap_Modify();
	
	bool saveMap_CallBack();

    bool saveMap_pathcheck();
	
	void setSaveMapDoneStatus(int status);

};

