#include "../type.h"
#include "Gridmap.h"
#include "Astarplanner.h"
#include <math.h>
#include <time.h>
#include <fstream>
#include <iostream>

#define C_up  10000
#define C_low  0
#define WALL 119 // int number of 'w' is 119
#define FREE 0
#define ITER_NUM 50 // iteration number
#define ALPHA 1
#define BETA 5
#define RHO 0.1
#define Q 100
#define AVE_DIST 350
#define random(x) random()%x
#define RandomZeroOne() (rand()%1000000)*0.000001
#define CLEAN_WIDTH 0.8
#define ASTAR_OPTIM 30


typedef struct {		// Interval structure
	int	in;
	int 	out;
}Interval;


class SliceConnect
{

public:
	//int free_num, obs_num;        // number of free and obstacle interval number;
	vector<Interval> free_interval, obs_interval; // free and obstacle interval	
};

class AntColony{

public:
	vector<int> route;  // route for each ant, no need to clear
	double dist; // route distance

	vector<int> parent; // parent region array, size region_num, no need to clear
	vector<int> visited; // visited regions array, size region_num, no need to clear

	vector<int> neighbour;// neighbour region, need to be cleared every time
};

void ChangeValue(int* ip);
void InitializeMatrix(int* ip);
void ChangeMatrix(int* ip);
void ShowMatrix(int* ip);
void SetMap(BYTE *map_input, BYTE *map_output, int width, int height);
void ShowMap(BYTE *map_input, int a, int b);
void SliceConnectChangePointer(SliceConnect *scp, int x, int y);
void SliceConnectChange(SliceConnect &sc, int x, int y);
void ShowSliceConnect(SliceConnect sc);
// height >= 2
// function to get the connectivity of the slice
// k is the number in the array
void SliceConnectivity(SliceConnect &sc, BYTE *map_data, int width, int height, int k);
void IntegerVectorChange(vector<int> iv);
void IntegerVectorChangePointer(vector<int> &ip);
// function to get the intersection of the intervals
int Intersection(Interval i1, Interval i2);
int FreeIntervalIntersectionNumber(vector<int> &iv, int i1, SliceConnect sc1, SliceConnect sc2);
int SliceDecomposition(BYTE *map_input,  int width, int height, int *map_output);
void ShowSliceDecomposition(int *map_input, int width, int height);
void CopySliceDecomposition(int *map_input, int width, int height, int *map_output);
void ComparasonSliceDecomposition(int *map1, int *map2, int width, int height);
void InitializationConnectivityMap(int region_num, int *mat_output);
void RegionConnectivity(int *map_input, int width, int height, int region_num, int *mat_output);
void ShowRegionConnectivity(int region_num, int *mat_output);
void ShowRegionDistance(int region_num, double *mat_output);
void GetCentroidPose(GridMap &map, int *sliced_map, int region_num, Pose *centroid);
void GetCentroidPoseGrid(GridMap &map, int *sliced_map, int region_num, Pose *centroid);
void ChangeCentroidToGlobal(GridMap &map, int region_num, Pose *centroid);
void ShowCentroidPose(int region_num, Pose *centroid);
void RegionDistance(GridMap &map, int *connectivity, int region_num, Pose *centroid, double *region_dist);
void ComparisonConnectDist(int region_num, int *connectivity, double *region_dist);
void IdentifyConnectDist(int region_num, int *connectivity, double *region_dist);
void TauMatrix(int region_num, double *region_dist, double ave_dist, double* mat_output);
void ShowTauMatrix(int region_num, double *mat_output);
void EtaMatrix(int region_num, double *mat_input, double *mat_output);
void ShowEtaMatrix(int region_num, double *mat_input);
void ShowParent(int region_num, vector<int> &parent);
void AntColonyResize(int region_num, AntColony *ant);
void InitializationAntColony(int region_num, AntColony *ant);
void ShowAntColony(int region_num, AntColony *ant);
void AntRandomWalk(AntColony *ant, int region_num, int* region_connect, double *region_dist, vector<int> &route);
void AntNeighbour(AntColony *ant, int region_num, double *region_dist);
void AntNeighbourConnect(AntColony *ant, int region_num, int *region_connect);
void ShowAntNeighbour(AntColony *ant, int region_num);
void InitializationFinished(int region_num, int *finished);
void VectorIntersection(vector<int> &v_input1, vector<int> &v_input2, vector<int> &v_output);
void NeighbourInterVisited(vector<int> &neighbour, vector<int> &visited, vector<int> &inter);
void ShowIntegerVector(vector<int> &vi);
void UnvisitedRegion(vector<int> &ur);
void ShowFinished(int region_num, int *finished);
void InitializationRestart(int region_num, int *restart);
void ShowRestart(int region_num, int *restart);
bool IsOneAntFinished(vector<int> &visited);
bool AllAntsFinished(int region_num, int *finished);
void ShowVisited(vector<int> &vi);
double PseudoRandomValue(double input);
int MaxValueNumInProbability(int length, double *vi);
int MinLengthAntNum(int region_num, AntColony *ant);
int MaxLengthAntNum(int region_num, AntColony *ant);
double MinDistAntNum(int region_num, AntColony *ant);
double MaxDistAntNum(int region_num, AntColony *ant);
void FindExtremeAnt(int num, int region_num, AntColony *ant, AntColony *best, AntColony *worst);
void FindBestRouteIteration(AntColony *best, vector<int> &vi);
void AntColonyRegionRoute(int region_num, int *region_connect, double *region_dist, vector<int> &route);
void RepeatAntColony(int repeat, int region_num, int *connectivity_mat, double *region_dist, vector<int> &route_best);
void InitializationCardinalPoints(int region_num, double *cp, double *bound_lr);
void InitializationCardinalPointsVertical(int region_num, double *cp_vertical, double *bound_ud);
void ShowCardinalPoints(int region_num, double *cp);
void RegionLeftAndRightPoints(int *slice_map, int width, int height, int region_num, double *cp);
void RegionCardinalPoints(int *slice_map, int width, int height, int region_num, double *cp);
/***************************v1.2 addition for vertical cp*******************************/
void RegionCardinalPointsVertical(int *slice_map, int width, int height, int region_num, double *cp_vertical);
void ChangeRegionCardinalPointsFromGridToGlobal(int region_num, double *cp, GridMap &map, double *cp_global);
void OutputTxtFile(const char* fileName, int width, int height, int *sliced_map);
void OutputMapToTxtFile(const char* fileName, int width, int height, BYTE *mapx);
void OutputRouteToTxtFile(const char* fileName, vector<vector<double> > route);
void OutputByteToTxtFile(const char* fileName, int width, int height, BYTE *map_data);
void OutputRegionConnectTxtFile(const char* fileName, int region_num, int *connect_mat);
void OutputFile(const char* fileName);
void OutputRouteTxtFile(const char* fileName, vector<int> &route);
void InitializationLeftAndRightBound(int region_num, double *cp);
void GetLeftAndRightBound(int *slice_map, int width, int height, int region_num, double *cp);
void InitializationUpAndDownBound(int region_num, double *cp);
void GetUpAndDownBound(int *slice_map, int width, int height, int region_num, double *cp);
void ShowBounds(int region_num, double *cp);
void GetSurface(int region_num, double *bound_lr, double *bound_ud, double *surface);
void ShowSurface(int region_num, double *surface);
void InitializationStartAndEndPoints(vector<int> &route, double *start, double *end);
void ShowStartAndEndPoints(vector<int> &route, double *start, double *end);
double GetDistanceBetweenTwoPoints(double x1, double y1, double x2, double y2);
void GetStartAndEndPointsGrid(vector<int> &route, double *cp, double *start, double *end);
void CopyArray(double *a, double *a_copy, int num);
void GetUpperAndLowerGrid(int num, vector<int> &route, int *sliced_map, double *start_grid, double *end_grid, 	vector<vector<double> > &upper, vector<vector<double> > &lower, int odd);
double sign(double f);

/***********************v1.1 correct bound error****************************/
void MaxMinYInOneRegionForX(int region, double *cp, int *sliced_map, GridMap &map, double x, vector<double> &y);

void MaxMinXInOneRegionForY(int region, double *cp_vertical, int *sliced_map, GridMap &map, double y, vector<double> &x);

// num, index of route
void GetPathOneRegionGridOptim(int num, vector<int> &route, double *start_grid, double *end_grid, double *cp, double *surface, Pose *centroid_grid, int step, int *sliced_map, GridMap &map, vector<vector<double> > &part);

/***********************v1.1 reduce the overlapping****************************/
/***********************v1.2 add determination direction****************************/
/***********************v1.3 neglect small region, go once each line****************************/
/***********************v1.4 optimize path between regions****************************/
// primary algorithm
void GetPathOneRegionGridPrime(int num, int cp_num, vector<int> &route, double *cp, double *cp_vertical, double *surface, int step, int *sliced_map, GridMap &map, vector<vector<double> > &part);
int FindNextNearestCardinalPoint(double x, double y, int region, double *cp);
void ShowArrayTypeDouble(double *array, int num);
void ChangeVectorVectorTypeDoubleFromGridToGlobal(vector<vector<double> > &part, GridMap &map);
void InsideOneRegion(int num, vector<int> &route, double *start, double *end, double *cp, double *surface, Pose *centroid, int *sliced_map, GridMap &map, int *visited, vector<vector<double> > &part);
void OptimAstarRoute(vector<vector<double> > &inter, vector<vector<double> > &inter_replace);
void DeleteRepteatedRegion(vector<int> &new_route, vector<int> &route, int region_num, int *visited);
/**************************************v1.2 change vertical points****************************************/
/**************************************v1.3 remove visited regions****************************************/
/**************************************v1.4 optimize between regions****************************************/
void AllRegionPrime(vector<int> &route, double *cp, double *cp_global, double *cp_vertical, double *cp_vertical_global, double *surface, int step, int *sliced_map, GridMap &map, vector<vector<double> > &route_global);
void ConnectVectorVectorTypeDouble(vector<vector<double> > &part, vector<vector<double> > &route);
void ShowVectorVectorTypeDouble(vector<vector<double> > part);
void InitializationVisited(int region_num, int *visited);
void ShowVisited(int region_num, int *visited);
void CoordinatesFromGridToGlobal(double *start, double *end, GridMap &map, vector<int> &route);

void RegionCoverage(GridMap &map,Pose &interface,vector<vector<double> > &route_global);



