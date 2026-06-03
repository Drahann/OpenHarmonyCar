#pragma once


#include "type.h"
#include "map/ScanMatcher.h"
#include "math/LinAlg.h"
#include "math/mymath.h"
#include "map/Gridmap.h"
#include "planner/PathPlanner.h"
#include "map/MapServer.h"
#include "map/MultiResolutionScanMatcher.h"
#include "CleanPlanner.h"
#include "particlefilter/ParticleFilter.h"
#include <queue>
#include "Astarplanner.h"
#include "FullPathCoverage.h"
#include "Eigen/LU"

#define GET_ROOM_BOUNDARY 1
#define COVERAGEALG 1
#define ROBOTSIZE 7


using namespace std;

using namespace Eigen;

typedef void *LPVOID;


typedef enum tagNavigationType
{
	MANUAL,
	LOCALIZATION,


}NavigationType;

typedef enum tagLocationType
{
	SCANMATCH,
	SLAM,
	PARTICLEFILTER,
	INIT,
}LocationType;


typedef  void (*PGetPosCallBackFunc)(Pose *pos,double flag,vector<double> &rate_flag);
typedef  void (*PGetPathCallBackFunc)(double *pathxyr,int pnum,int type);
typedef  void (*PHaveNoPathCallBackFunc)(int &a);
typedef  void (*PCmdCallBackFunc)(int);

typedef  void (*PLogInfoCallBackFunc)(int logtype);

struct laser_st
{
    
       
        int    nranges;
        std::vector< float > ranges;
        int    nintensities;
        std::vector< float > intensities;
        float      rad0;
        float      radstep;
};


typedef struct tagRobotConfig
{
	struct taglaserconfig
	{
		double positionX;
		double positionY;
		double positionZ;// Robot coords of the servo (main axis) at attachment
   
		double min_range;
		double max_range ;
	

	}laserconfig;
	struct tagrobotconfig
	{
		double width;
        double radius;
        double circles_x;
        double circles_y;


	}robotconfig;
	
	struct tagScanMatchconfig
	{
		double xScanMatchRange;
		double yScanMatchRange;
		double thetaScanMatchRange;

	}ScanMatchconfig;

	plannerconfig pathplannerconfig;

	double range;
	double resolution;

}RobotConfig;

class OptimizeMap
{
public:
	OptimizeMap(void);
	~OptimizeMap(void);

	ScanMatcher    *OpScanMatcher;
	MapServer      *OpMapServer;
	MultiResolutionScanMatcher	 *OpMatcher;

};

class Scanlinkmatch
{
public:
	Scanlinkmatch(void);
	~Scanlinkmatch(void);

	ScanMatcher	*linkScanMatcher;
	MultiResolutionScanMatcher	 *linkMatcher;
};


class CNaviInterface
{
public:
	CNaviInterface(void);
	~CNaviInterface(void);

	ScanMatcher		*m_pscanMatcher;
	MapServer			*m_pms;
	MapServer			*m_pmsSLAM;

    ScanMatcher		*pscanMatcher;

	OptimizeMap     *m_pOptimizeMap;
	
	Scanlinkmatch	*m_pscanlinkmatch;	

	CAstar astarPlanner;
	FullCoverageAlg fullCoverageAlg;
	char			mapfilename[512];
	vector<Pose>	vtwallpose;

	bool			m_bfind;
	bool			m_bfind2rc;
	bool		    m_bodmswitch;
	
	bool			m_blaserdwa;
	bool			m_blaserastar;

	bool			enableCoverage;
	bool			targetErr;
	int				searchType;
	/**********************/
    MapServer			*m_pmsSLAMtest;
	/**********************/
	MultiResolutionScanMatcher	 *m_pmatcher;
	NavigationType		m_eNaviType;
	Pose			globalOdoT;
	ParticleFilter  	*m_pPf;

	laser_st		m_stlaserdata;
	laser_st        *m_plaserdata;
	Pose			m_CurPosForPath;
	
	Pose			m_lastxyt2;
				

	queue<Pose>		m_waypoints;
	Pose			m_CurPos;
	bool			m_bIsLoc;
	bool			m_bRunning;
	bool			m_bConverged;
	bool 			m_bPoseError;			
	bool 			m_bRevise ;
	bool			m_bslam;

	bool			m_laser1only;	
	bool			m_bifrecenter;

	bool			m_bexpandmap;	

	bool			drawvision;

	bool			backforward;
	
	bool 			m_ifmodifymap;
    bool            m_bautocharge;
    bool            m_bcreateautochargemap;
    int             autochargemapcount;
    Pose            chargemappose;
    Pose            lastchargemappose;
	
	int				backforwardnum;
	bool			m_bmanualupdate;

	LocationType		m_eLocationType;
	int 				m_pf_count;

	bool			m_bupdatemap;
	Pose			lastupdatepose;
	Pose			lastfixpose;
	Graph			updateg;

	Pose			lastAmapupdatepose;

	double 			search_x_m ;
	double 			search_y_m ;
	double 			search_theta_rad;

	double			revise_Range_x;
	double			revise_Range_y;
	double			revise_Range_theta;

	bool			bpfsetpose;
	Pose			m_lastxyt;
	Pose			m_nowxyt;

	Pose 			scanlinkpose;

	Pose			navipose;
	Pose			scanlinktest;

	PGetPosCallBackFunc 	 	m_pPosCbFunc;
	PGetPathCallBackFunc	 	m_pPathCbFunc;
	PHaveNoPathCallBackFunc  	m_pNoPathCbFunc;
	PCmdCallBackFunc         	m_pCmdCbFunc;	
	PLogInfoCallBackFunc    	m_pLogCbFunc;

    int             m_imode;

	int 			m_ifirstinitloc;
	int 			m_iplanend;
	int				m_ijumpnum;
	int				m_ilowscorenum;
	unsigned char	m_ucfindmode;
	unsigned char	m_ucspeedlever;

	pthread_mutex_t  	m_csPose_mutex;
	pthread_mutex_t  	m_csLoc_mutex;
	pthread_mutex_t  	m_csPlan_mutex;
	pthread_mutex_t  	m_csLaser_mutex;
	pthread_mutex_t  	m_initloc_mutex;

	pthread_t 			m_thrdPathPlan;
	pthread_t			m_thrdFullPathCoveragePlan;
	pthread_attr_t   	m_thread_attribute;
	pthread_cond_t		m_condPathPlan;
	pthread_mutex_t		m_mutex;
	pthread_mutex_t		m_goal_mutex;
	pthread_mutex_t		m_pf_mutex;

	pthread_mutex_t		m_vw_mutex;

	


	RobotConfig		m_config;
	double			S2B[4][4];
	double			max_range;

	vector<vector<double> > path;
	int ordi;
	int ordi_1;
	vector<int> testpath;
	int m_ifastar;
	int vw_0;
	int rrset;
	int curastar;
	int istartgo;
	int pfright;
	double Vcur;
	double Wcur;

	double dvibration;
	int ifirstpublish;
	double m_laseronlyflag;
	Pose m_poselastxyt;
	int m_ifpf;
	Pose m_CurPos_pf;

	vector<Pose> global2pointstrans;
	
	vector<Pose> m_VisionWallData;
	vector<Pose> m_VisionWallDataCopy;
	
	vector<Pose> m_CollisionWallData;
	vector<Pose> m_CollisionWallDataCopy;

	vector<Pose> gridPath;

	int robotId;

	static void * PlanThreadProc(LPVOID pPara);
	static void * CoverageThreadProc(LPVOID pPara);
	vector< vector<Pose> >	vpose;
	vector< vector<Pose> >	cpose;
	MultiResolutionScanMatcher pmatcher;

	void	setLocationType(LocationType type);
	void	setConfig(RobotConfig &config);
	void	putLaserData(laser_st* plaserData);
	/*****************************************************************************************************/
	void    GetVisionData(vector<Pose> *pvisionData);
	/*****************************************************************************************************/
	void    GetCollisionData(vector<Pose> *pcollisionData);
	/*****************************************************************************************************/
	void    putEncoderData(Pose *pPos);
	void	laser2Points(laser_st &ldata, double omitRange, double maxRange, double *mask_out_rad,int masklength,vector<vector<double> > &points,vector<vector<double> > &forelaser,vector<vector<double> > &limitlaser,int limitdis);
	void	update(vector<Pose> &points,vector<Pose> &forelaserPoints,vector<Pose> &limitlaserPoints);
	void	find2rc(double *dErrorPose);
	void 	SaveMapCallBack(void);
	void	manualupdatemap(Pose &m_CurPos,vector<Pose> &forelaserPoints,Pose &res,Pose &lastupdatepose,vector<Pose> &limitlaserPoints,double &search_theta_res_m);
	void	saveupdateg(Pose &lastupdatepose,Pose &m_CurPos,Graph &updateg,double &h2,vector<Pose> &limitlaserPoints);
	void	doSLAM(vector<Pose> &bodyPoints);
	void	planPath(ProbMap& map, Pose cur, int type, int ratio);
	int     DWAplan(vector<double> &goalvw,int &flag);
	int     DWAplanlaseronly(vector<double> &goalvw,int &flag);
	int 	choose_goal(Pose &robot,vector<double> &goal,vector<double> &goal_1,vector<double> &goal_follow);
	double  difsafe(vector<Pose> &path);
	double  difsafelaseronly(vector<Pose> &path);
	double  bifsave(Pose &p);
	double  bifsavelaseronly(Pose &p);
	int 	ifrobotsafe(vector<double> &p);
    bool 	iftargetlegal(vector<double> &p);
	double  doArrive(vector<double> &p);
	double  doArrivelaseronly(vector<double> &p);
	double  ThetaScore(Pose &p,vector<double> &q);
	double  DetaTheta(Pose &From,vector<double> &To);
	double  DistScore(Pose &p,vector<double> &q,int flag);
	bool	loadMap(const char *strMapName,vector<Pose> &vtWallPos);
	void	createMap(double metersPerPixel);
	void	initLoc(Pose &pos,Pose &range);
	void	saveMap(const char *strMapName);
	void	saveModifyMap();
	void	drawMap();
	bool	setGoal(Pose &goal);
	void	deleteGoal();
	void	clearNavigationStateAndStop();
	void 	RevisePose(Pose &posRevise,int flag);
	void    RevisePoseAndRange(Pose &posRevise,double xSearchRange,double ySearchRange, double thetaSearchRange,unsigned char flag);

	void 	CleanPlan(unsigned int cleangridwidth);

	void    particleFilterLoc(Pose pos,Pose range, int particlenum);

	void	RegisterGetPosCallback(PGetPosCallBackFunc pPosFunc);
	void	RegisterGetPathCallback(PGetPathCallBackFunc pPathFunc);
	void	RegisterNoPathCallback(PHaveNoPathCallBackFunc pFunc);
	void	RegisterCmdCallback(PCmdCallBackFunc pFunc);	
	void	RegisterLogInfoCallback(PLogInfoCallBackFunc pFunc);
	
	void    RecoverError();

	void    SendPath(vector<vector<double> > &path,int type);

	void	createProbMap(const char *strMapName);
	void	updateProbMap(const char* fileName);
	
	void    Setupdatemode(bool mode);
	bool	Getupdatemode();
	void    Setexpandmode(bool mode);
	bool	Getexpandmode();
	void	SetRefineMode(int mode);
	void	SetVW(vector<double> &vw);
	void	init_pf_locate(Pose &p);
	void	exit();
	double	max(double a,double b);
	double	min(double a,double b);
	void	setvisionrecenter();
	void	clearupdateg();
	void	recentervisionmap(Pose &p1,Pose &p2,GridMap &map);
	void	setdrawvisionmode(bool mode);
	void    ClearVisionData(void);
	void    ClearCollisionData(void);
	void    Setmanualupdate(bool mode);
	bool	Getmanualmode();
	void	SetBackforward(bool mode);
	void	SetFind(void);
	void 	SetOdmSwitch(bool mode);
	void 	Setmodifymap(bool mode);
	void 	SetSpeedLevle(unsigned char speedlever);
	bool    saveMapCallBack(void);
	void 	SetSaveMapDone(int status);
    void    Setautocharge(bool mode);

	// AnXin：2025-2-11
	void	setPlanFullPath(int algNum);
	void	detectCoveragePlan(void);
	// AnXin：2025-4-17
	void	cancelPlanFullPath(void);
	// AnXin：2025-4-24
	void	setRoomVertex(int order, double x, double y);
	// AnXin：2025-5-10
	void	setSearchType(int searchType);
	// AnXin：2025-6-26
	void	subGetMapFromMain(int* ip);

	void	CreateFullPath(int x1, int y1, int x2, int y2, int* rob, int safesize, int minsize);

	void	getsize_forFullRoad(int len, int safe,int mins, int* list);

	void	NavigatePathByGridPoints(const vector<Pose>& gridPath);

	vector<IPoint> ReadFullPathFromFile(const string& filepath);
};

void	NAVI_SetLocationType(LocationType type);
void	NAVI_SetConfig(RobotConfig config);
void	NAVI_PutLaserData(int nranges, float  *ranges, int nintensities, float  *intensities);
/******************************************************************************************************/
void	NAVI_GetVisionData(float *x,float *y, int length);
void  	NAVI_GetCollisionData(double **wallxy, int length);
/******************************************************************************************************/
void	NAVI_PutEncoderData(Pose *pPos);
bool	NAVI_LoadMapAndLoc(const char* strMapName,Pose initPos, Pose initRange,vector<Pose> &vtWallPos);
void	NAVI_CreateMap(double metersPerPixel);
void	NAVI_SaveMap(const char *strMapName);
void	NAVI_SaveModifyMap(void);
void	NAVI_SetGoalPoint(Pose goal);
void	NAVI_DeleteGoal();
void	NAVI_DeleteAllGoal();
void	NAVI_RegisterGetPosCallback(PGetPosCallBackFunc pPosFunc);
void	NAVI_ReginsterGetPathCallback(PGetPathCallBackFunc pPathFunc);
void	NAVI_RegisterNoPathCallback(PHaveNoPathCallBackFunc pFunc);
void	NAVI_RegisterCmdCallback(PCmdCallBackFunc pFunc);

void	NAVI_RegisterLogInfoCallback(PLogInfoCallBackFunc pFunc);

void	NAVI_RevisePose(Pose &posRevise,int flag);
void	NAVI_RevisePoseAndRange(Pose &posRevise,double xSearchRange,double ySearchRange, double thetaSearchRange,unsigned char flag);
void    NAVI_CleanPlan(unsigned int cleangridwidth);
void	NAVI_ParticleFilter(Pose pos,Pose range,int particlenum);

void	NAVI_SetRefineMode(int mode);
void	NAVI_Setupdatemode(bool mode);
bool	NAVI_Getupdatemode();
void	NAVI_Setexpandmode(bool mode);
bool	NAVI_Getexpandmode();
void	NAVI_SetVW(vector<double> &vw);
void	NAVI_UpdateProbMap(const char* fileName);
void    NAVI_OptimizeMap(const char *strMapName);
void	NAVI_VisionRecenter();
void	NAVI_clearupdateg();
void    NAVI_Setlimitdis(int dis);

void	NAVI_Exit();
void	NAVI_Setdrawvisionmode(bool mode);
void	NAVI_ClearVisionData(void);
void	NAVI_ClearCollisionData(void);
void	NAVI_Setmanualupdate(bool mode);
bool	NAVI_Getmanualmode();
void	NAVI_SetBackforward(bool mode);
void	NAVI_SetFind(void);
void 	NAVI_SetOdmSwitch(bool mode);
void 	NAVI_Setmodifymap(bool mode);
void 	NAVI_SetSpeedLevle(unsigned char ucSpeedLever);
void 	NAVI_CloseVisionDate();
void 	NAVI_OpenVisionDate();
void    NAVI_Setautocharge(bool mode);
void    NAVI_Savevisiondate(int mode);

// AnXin：2025-2-11
void    NAVI_SetPlanFullPath(int algNum);
// AnXin：2025-4-17
void NAVI_CancelPlanFullPath(void);
// AnXin：2025-4-24
void NAVI_SetRoomVertex(int order, double x, double y);
// AnXin：2025-5-10
void NAVI_SetSearchType(int searchType);
// AnXin：2025-6-10
void NAVI_SubGetMapFromMain(int* ip);

void NAVI_CreateFullPath(int x1, int y1, int x2, int y2, int* rob, int safesize, int minsize);

void NAVI_getsize_forFullRoad(int len, int safe,int mins, int* list);

void NAVI_NavigatePathByGridPoints(const vector<Pose>& gridPath);

void NAVI_SetgridPath(const vector<Pose>& gridPath);

void NAVI_SetrobotId(int robotId);

IPoint NAVI_GlobalToGrid(double x, double y);
