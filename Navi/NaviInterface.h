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
#include <stdint.h>
#include "Astarplanner.h"
#include "FullPathCoverage.h"
#include "Eigen/LU"

#define GET_ROOM_BOUNDARY 1
#define COVERAGEALG 1
#define ROBOTSIZE 7

#define COOP_AVOID_CHANNEL "COOP_AVOID"
#define COOP_AVOID_CMD_POSE_REQUEST  -40
#define COOP_AVOID_CMD_POSE_RESPONSE -39
#define COOP_AVOID_CMD_STOP_REQUEST  -38
#define COOP_AVOID_CMD_STOP_ACK      -37
#define COOP_AVOID_CMD_RESUME_REQUEST -36
#define COOP_AVOID_CMD_RESUME_ACK    -35
#define COOP_AVOID_CMD_ACK           -34
#define COOP_AVOID_STATUS_COMMAND     74

enum CoopAvoidState
{
	COOP_AVOID_NORMAL = 0,
	COOP_AVOID_WAIT_PEER_POSE,
	COOP_AVOID_WAIT_STOP_ACK,
	COOP_AVOID_WAIT_RESUME_ACK,
	COOP_AVOID_PEER_PAUSED_BY_ME,
	COOP_AVOID_STOPPED_FOR_PEER
};

enum CoopAvoidTrigger
{
	COOP_AVOID_TRIGGER_NOPATH = 1,
	COOP_AVOID_TRIGGER_MATCH_JUMP = 2
};

enum CoopAvoidHeartbeatStatus
{
	COOP_HEARTBEAT_NORMAL = 0,
	COOP_HEARTBEAT_DIAGNOSING = 1,
	COOP_HEARTBEAT_STOPPED_FOR_PEER = 2,
	COOP_HEARTBEAT_PEER_PAUSED_BY_ME = 3,
	COOP_HEARTBEAT_LOCAL_DYNAMIC_FALLBACK = 4
};

enum CoopAvoidHeartbeatEvent
{
	COOP_HEARTBEAT_EVENT_NONE = 0,
	COOP_HEARTBEAT_EVENT_NOPATH = 1,
	COOP_HEARTBEAT_EVENT_MATCH_JUMP = 2,
	COOP_HEARTBEAT_EVENT_TIMEOUT = 3,
	COOP_HEARTBEAT_EVENT_RESUME = 4,
	COOP_HEARTBEAT_EVENT_NO_LCM = 5
};

enum MapLifecycleState
{
	MAP_STATE_IDLE = 0,
	MAP_STATE_MAPPING,
	MAP_STATE_SAVING,
	MAP_STATE_MAP_READY,
	MAP_STATE_NAVIGATING
};

enum FullPathErrorCode
{
	FULLPATH_OK = 0,
	FULLPATH_ROOM_INIT_FAILED = 1001,
	FULLPATH_ROAD_FILE_INVALID = 1002,
	TARGET_STATIC_INVALID = 1003,
	TARGET_DYNAMIC_BLOCKED_LASER = 1004,
	TARGET_DYNAMIC_BLOCKED_VISION = 1005,
	ASTAR_NO_PATH = 1006
};

enum TargetCheckResult
{
	TARGET_CHECK_OK = 0,
	TARGET_CHECK_STATIC_INVALID,
	TARGET_CHECK_DYNAMIC_BLOCKED_LASER,
	TARGET_CHECK_DYNAMIC_BLOCKED_VISION
};


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

struct MappingScanFrame
{
    Pose pose;
    std::vector<Pose> bodyPoints;
    std::vector<Pose> forelaserPoints;
    std::vector<Pose> limitlaserPoints;

    MappingScanFrame()
    {
        pose.x = 0;
        pose.y = 0;
        pose.theta = 0;
    }
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
	MapLifecycleState m_mapState;
	int             m_lastFullPathError;
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
	int             m_encoderInitialized;

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
    vector<MappingScanFrame> m_mappingScanFrames;
    bool m_mappingReplayInProgress;

	int robotId;
	pthread_mutex_t m_coop_mutex;
	CoopAvoidState m_coopState;
	int m_coopSeq;
	int m_coopActiveSeq;
	int m_coopActivePeer;
	int m_coopTriggerReason;
	long m_coopStateTimeMs;
	int m_coopHeartbeatStatus;
	int m_coopHeartbeatEvent;
	bool m_coopPendingStopRequest;
	int m_coopPendingStopSource;
	int m_coopPendingStopSeq;
	bool m_coopSavedGoalValid;
	Pose m_coopSavedGoal;
	int m_coopSavedCoverageIndex;
	int m_coverageTurnIndex;
	bool m_coverageActive;
	bool m_coopPeerPoseValid;
	bool m_coopPeerHasGoal;
	Pose m_coopPeerPose;
	Pose m_coopPeerGoal;
	int m_coopLastTxCommand;
	int m_coopLastTxTarget;
	int m_coopLastTxSeq;
	int m_coopLastTxReason;
	long m_coopLastTxTimeMs;
	int m_coopLastTxRetryCount;
	bool m_coopLastTxAcked;
	bool m_localDynamicBlockGoalValid;
	Pose m_localDynamicBlockGoal;
	int m_localDynamicBlockCount;
	long m_localDynamicBlockFirstMs;
	long m_localDynamicLastCoopMs;

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
	TargetCheckResult checkTargetLegal(vector<double> &p, bool includeDynamic);
	bool 	iftargetlegalStatic(vector<double> &p);
	void    setLastFullPathError(int errorCode);
	double  doArrive(vector<double> &p);
	double  doArrivelaseronly(vector<double> &p);
	double  ThetaScore(Pose &p,vector<double> &q);
	double  DetaTheta(Pose &From,vector<double> &To);
	double  DistScore(Pose &p,vector<double> &q,int flag);
	bool	loadMap(const char *strMapName,vector<Pose> &vtWallPos);
	bool	createMap(double metersPerPixel, bool forceNewMap = false);
	void    resetMappingRuntimeState(void);
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
    void    clearMappingLaserFrames(void);
    void    resetScanMatcherRuntime(ScanMatcher *scanMatcher);
    void    cacheMappingScanFrame(vector<Pose> &bodyPoints,
                                  vector<Pose> &forelaserPoints,
                                  vector<Pose> &limitlaserPoints);
    bool    replayMappingScanFramesForSave(void);
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
	void    setMapLifecycleState(MapLifecycleState state);
	MapLifecycleState getMapLifecycleState(void);

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

	bool	CreateFullPath(int x1, int y1, int x2, int y2, int* rob, int safesize, int minsize);

	void	getsize_forFullRoad(int len, int safe,int mins, int* list);

	void	NavigatePathByGridPoints(const vector<Pose>& gridPath);

	vector<IPoint> ReadFullPathFromFile(const string& filepath);
	void	handleCoopAvoidMessage(int commandId, int targetRobotId, int sourceRobotId,
								 int seq, const double *dparams, int ndparams,
								 const int8_t *iparams, int niparams,
								 const uint8_t *bparams, int nbparams);
	void	triggerCoopAvoidance(int reason);
	void	updateCoopAvoidance(void);
	bool	isStoppedForCoopPeer(void);
	bool	isPeerPausedByMe(void);
	void	markCoverageIndex(int index);
	void	resetCoverageState(void);
	void	respondCoopPoseRequest(int sourceRobotId, int seq);
	void	sendCoopPoseRequest(int seq, int reason);
	void	sendCoopStopRequest(int targetRobotId, int seq, int reason);
	void	sendCoopStopAck(int targetRobotId, int seq);
	void	sendCoopResumeRequest(int targetRobotId, int seq);
	void	sendCoopResumeAck(int targetRobotId, int seq);
	void	sendCoopAck(int targetRobotId, int seq, int ackedCommandId);
	void	beginCoopReliableTxLocked(int commandId, int targetRobotId, int seq, int reason);
	void	clearCoopReliableTxLocked(void);
	void	publishCoopHeartbeatStatus(int status, int event);
	void	fallbackToLocalDynamicAvoidance(int failedCommand, int reason,
										   bool acked, int retryCount);
	bool	holdLocalDynamicBlockBeforeCoop(const Pose &goal, int reason,
											const char *context);
	void	resetLocalDynamicBlockState(void);
	bool	getCurrentGoal(Pose &goal);
	bool	peerLikelyBlocksCurrentRoute(int reason);
	bool	pauseForCoopPeer(int sourceRobotId, int seq);
	void	resumeAfterCoopPeer(void);
	void	publishZeroVelocity(void);
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
bool	NAVI_CreateMapWithMode(double metersPerPixel, bool forceNewMap);
void    NAVI_ClearGeneratedMapFiles(void);
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

bool NAVI_CreateFullPath(int x1, int y1, int x2, int y2, int* rob, int safesize, int minsize);

void NAVI_getsize_forFullRoad(int len, int safe,int mins, int* list);

void NAVI_NavigatePathByGridPoints(const vector<Pose>& gridPath);

void NAVI_SetgridPath(const vector<Pose>& gridPath);

void NAVI_SetrobotId(int robotId);

IPoint NAVI_GlobalToGrid(double x, double y);
void NAVI_HandleCoopAvoidMessage(int commandId, int targetRobotId, int sourceRobotId,
								 int seq, const double *dparams, int ndparams,
								 const int8_t *iparams, int niparams,
								 const uint8_t *bparams, int nbparams);
