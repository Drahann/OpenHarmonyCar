#pragma once
#include "PathPlanner.h"
#include "../type.h"
#include <math.h>


typedef struct CleanLine
{
	int j1;
	int j2;
	int i;
	bool bCleaned;
	int areaInx;

};

typedef struct CleanArea
{
	int inx;
	vector<CleanLine> vtArea;
	POINT cor[4];

	bool bFind;

};





class CleanPlanner :
	public PathPlanner
{
public:
	CleanPlanner(void);
	~CleanPlanner(void);


	void setConfig(plannerconfig config,double robot_width, double robot_radius, unsigned int cleangridwidth);

	void getBoundingBox(GridMap &map,MyRect &bounding);
	int creatcleanamap(GridMap &map, Pose curpose,GridMap &resMap);

	bool cleanmapplan(GridMap &cleanmap, Pose curpose,Pose goal,vector<vector<double> > &path,int curx,int cury);

	void PlanBorderPath(GridMap &map,Pose &beginPos,vector<Pose> &path);
    void GetCleanArea(GridMap &map,vector< vector<CleanLine> > &vtAllCleanLine,vector<CleanArea> &vtArea);
	bool GetCleanAllLine(GridMap &map,vector< vector<CleanLine> > &vtAllCleanLine);
	int FindLeftNeighbour(GridMap &map,POINT &ptCur, POINT &ptLeft,bool bj1,POINT ptPre, POINT ptNext, int LineGap );
	void PlanAreaPath(GridMap &map,Pose &curPose,vector<CleanArea> &vtArea,vector<Pose> &path);
	int PlanCleanPath(GridMap &rawmap,Pose& curPose,  vector<vector<double> > &path);
	void saveMap(const char* fileName, GridMap &map);
	
	int  getline(GridMap &tmpmap,int x0,int y0 ,int data);
	bool GetMapLine(GridMap &origmap);
	int findnextpoint(GridMap &tmpmap,int x0,int y0,int x,int y,int data,int &nextpoint);
	int IfvaildPoint(GridMap &tmpmap,int x0,int y0,int data,int size);

	int getmappath(GridMap &origmap,Pose &p1,Pose &p2);
	void  dividecleanmap(GridMap &origmap,Pose &cur);
	void SetPosePath(BYTE *connect,GridMap &origmap);
	void GetPath(int start,int end);
	int GetNextPos(vector<Pose> &cleanpath);
	int Ppath(vector<int> &pMid,int x,int y,int q);
	inline void GetLinePos(vector<Pose> &pos)
	{
		pos = linepos;
	}
	inline void GetPathPos(vector<Pose> &pos)
	{
		pos = pathpos;
	}
	inline void GetPath(vector< Pose > &tmppath)
	{
		tmppath = connectpath;
	}

private:

	unsigned int LINEDIS;
	vector<int> posvaild;
	vector<Pose> linepos;
	vector<Pose> pathpos;
	vector<int> posdis;
	vector<int> connectpos;
	vector<Pose> connectpath;
	WavefrontResults result;
	//WavefrontResults result;

/**************add end*******************************************************/
};

