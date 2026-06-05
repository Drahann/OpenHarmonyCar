#pragma once
#include "Gridmap.h"
#include "GXYTNode.h"
#include "Graph.h"
#include "ContourExtractor.h"
#include "../math/LinAlg.h"
#include "../math/mymath.h"
#include "MultiResolutionScanMatcher.h"



#include <vector>
#include <iterator>

#include "lcm/lcm.h"
#include "../lcmtype_dir/robot_control_t.h"
#include "../lcmtype_dir/grid_map_t.h"
int gzCompress(const char* src, int srcLen, char *dest, int destLen);
using namespace std;


    // Represents one of our historical scans which we use to build our local model.
    class Scan
    {
	public:
        Pose xyt;  // x, y, theta of robot

        // 2D points in global coordinate frame (i.e., projected by xyt)
      //  vector<vector<double>> gpoints;

		 vector<Pose> gpoints;
        // contours, in global coordinate frame.
        vector<vector<vector<double> > > gcontours;
    };


class ScanMatcher
{
public:
	ScanMatcher(void);
	~ScanMatcher(void);


//	CRITICAL_SECTION m_csPose;
	pthread_mutex_t  m_csPose_mutex;
	
    Graph  g;
    GridMap gm;
    bool gmDirty ;

	ProbMap pm;
	vector<int> glevel;

	ContourExtractor contourExtractor;

   
    MultiResolutionScanMatcher matcher;
	MultiResolutionScanMatcher pmatcher;

    double	metersPerPixel ;
    bool	useOdometry ;
    double	rangeCovariance ;
    int		maxScanHistory ;
    int		decimate ; // throw away all but every Nth scan. (1 = keep all).

    double	search_x_m ;
    double	search_y_m ;
    double	search_theta_rad ;
    double	search_theta_res_rad ;

    double	pose_dist_thresh_m ;
    double	pose_theta_thresh_rad ;

    double	gridmap_size ;

    int		old_scan_decay ; // in units of gray-scale values per age of scan kept.

    int		decimateCounter;

	bool m_IfVaild_Encoder;
    vector<Scan> scans;      

	vector<double> submapscore;
    // where do we think the robot is now (in global coordinates)?
    Pose xyt;

	Pose Encoderpos;

	Pose m_poselastxyt;

	vector<particles> pfswarm;


	

    inline void getPosition(Pose &pos)
    {

		pthread_mutex_lock( &m_csPose_mutex );
		 pos = xyt ;
		pthread_mutex_unlock( &m_csPose_mutex );
      ;
    }

	inline void SetXyt(Pose &pos)
	{
	
		pthread_mutex_lock( &m_csPose_mutex );
		xyt = pos;
		pthread_mutex_unlock( &m_csPose_mutex );
	}
    
	inline void SetEncoderXyt(Pose &pos)
	{
		pthread_mutex_lock( &m_csPose_mutex );
		Encoderpos = pos;
		m_IfVaild_Encoder = true;
		pthread_mutex_unlock( &m_csPose_mutex );
	}
 /*   public void processOdometry(double odomxyt[], double P[][])
    {
        if (!useOdometry)
            return;

        xyt = LinAlg::xytMultiply(xyt, odomxyt);

        // XXX ignore uncertainty for now.
    }
    */
    inline void setTheta(double theta)
	{
		pthread_mutex_lock( &m_csPose_mutex );
		xyt.theta = theta;
		pthread_mutex_unlock( &m_csPose_mutex );
    }

    /** Points should be projected into robot's coordinate frame. **/
    void processScan(vector<Pose> &doublelaserpoints,vector<Pose> &rpoints,vector<double> &flag);
    void drawScan(Scan &s);
	bool addProbMap(ProbMap &probMap, Pose pose, vector<Pose> &points,int level);
	void initpfswarm();
	void initprobmap();
	bool expendmap(Pose &pose);

};

