#pragma once
#include "Gridmap.h"

#include <vector>
#include <math.h>
#include <sstream>
#include <fstream>

using namespace std;




#define LESS(a1, a2, b1, b2) ((a1) < (b1) ? 1 : ((a1) == (b1)) && ((a2) < (b2)) ? 1 : 0)
#define LESSEQ(a1, a2, b1, b2) ((a1) < (b1) ? 1 : ((a1) == (b1)) && ((a2) <= (b2)) ? 1 : 0)


typedef struct {
	int	LastState;				
	int	CurrentState;			
} GridState;



typedef struct {
	int	state;				
	double	posibility;			
} MapFormat;



typedef struct {
 	int	x;										
 	int	y;										
} IPoint;


typedef struct {
	long  id;
	int  state;   		// {OPEN, NEW, CLOSED}
	double g;
	double h;
	double f;
	double k;		// States are placed on the OPEN list by their key value, k(x)
	void *parent;		// D* backpointer
	void *next;		// used for linked list connections
	void *prev;		// used for linked list connections
	void *nodeInfo;		// x and y coordinate
} Node;


typedef struct {		// NodeInfo structure
	int	x;
	int 	y;
} NodeInfo;




#define	MaxOccupied		0.9
#define	MinOccupied		0.2	
#define MAXNODES		200000     //10000000
#define MAXNEIGHBORS		25
#define NEW			0
#define OPEN			1
#define CLOSED			2


#define FreeSpace		0
#define Occupied		2
#define Near_Obstacle		3
#define heighcost       5
#define neardanger      10
#define danger          15





class  CAstar  
{
public:
	double 		x0;	// 地图左上角坐标的 x 分量
	double		y0;	// 地图左上角坐标的 y 分量
	double		GGridSize;
	int 		GMapLength; 
	int		GMapWidth;
	int		m_gblGoal[2] ;
	int		m_gblRobot[2] ;
	int		m_numInitial;
	double		m_costR[2] ;
	double		m_Magnitude;
	bool		m_ReachTraverseGoal;

	Pose		m_RobotCur,m_RobotEnd,m_RobotInit;
	IPoint		m_subgoal;
	GridState	*m_pGridState;
	GridState   *m_plaserGridState;
	GridState   *m_pvisionGridState;
	Node		*m_gblGrid;
	NodeInfo	*m_gblInfo;
	Node		*m_initial[100];
	int		m_expandGridNum;
	

public:

	int mallocspace(GridMap &map);
	int initLaserMap(GridMap &map);

	int	mallocvisionspace(GridMap &map);
	int initVisionMap(GridMap &map);

	int	InitializeGoalPoint(double x, double y,double theta);
	int	InitializeStartPoint(double x, double y,double theta);	
	int	PositionInMapOrNot(double x, double y);
	int 	GridInMapOrNot(int x, int y);
	int	GlobalPlanner(vector<NodeInfo> &vtPath,GridMap &map,GridMap &visionMap, int type);
	int	GetNextGoal();
	int	ComputeSubGoal(Pose pose);
	int 	getNeighbors(Node *parent, Node **neighbor,GridMap &map,GridMap &visionmap) ;
	int	robot(Node *p) ;
	int	inObstacle(int x, int y) ;
	
	double	hfunction(Node *p) ;
	double	cost(Node *to, Node *from,GridMap &map,GridMap &visionmap);

	void	freeNode(Node *p) ;
	void	SetStartPose(Pose tmpPose);
	void	SetEndPose(Pose tmpPose);
	void	InitializeCellRelation();
	void	InitializeCellTotal(ProbMap &map);
	void	PlannerInitial(ProbMap &map);
	void	FreePlanner();
	void	SetConfig(double radiusRobot);
	bool	IsOccupied(int i, int j);
	bool 	plan(Pose thegoal, ProbMap &visMap, Pose curpose, vector<vector<double> > &path,GridMap &lasermap,GridMap &visionMap, int type);
	// new function for pixel a star plan
	int 	PlanPixel(Pose thegoal, ProbMap &visMap, Pose curpose, vector<NodeInfo> &vtPlanPath);

    	Node*	GetNode(int x, int y);
	Node*	DStarSearch(Node ** initial, int numInitial, 
		double costR[2],GridMap &map ,GridMap &visionmap);
	Node*	AStarSearch(Node** initial, int numInitial, 
                  double costR[2], 
                  GridMap& map, GridMap& visionmap);
	Node*	BFSSearch(Node** initial, int numInitial, 
                double costR[2], 
                GridMap& map, GridMap& visionmap);
	Node*	insertOPEN(Node * openList, Node * newnode);
	IPoint  GlobalToGrid(double x , double y);
	Pose    GridToGlobal(int i , int j);

	/*******/
    //����GGridSize
    void	set_GGridSize(double m);
	/*******/

	CAstar();
	~CAstar();

};



