#pragma once

#include "../map/Gridmap.h"
#include "KLD.h"

#include <time.h>
#include "../math/MathUtil.h"
#include "../math/mymath.h"
#include "../math/LinAlg.h"
#include <iterator>
#include "../lcmtype_dir/laser_t.h"


using namespace std;


class Particle 

{
public:
    	double x,y,theta,weight,currentWeight;
    	inline Pose getPose()
		{
			Pose tmp;
			tmp.x = x;
			tmp.y = y;
			tmp.theta = theta;
			return tmp;
    		
    	}
    	
    	inline Particle clone(Particle &dest)
		{ 	
    		dest.x = x;
    		dest.y = y;
    		dest.theta = theta;
    		dest.weight = weight;
    		dest.currentWeight = currentWeight;
    
    	}
 };



class ParticleFilter
{
public:
	ParticleFilter(void);
	~ParticleFilter(void);
	 ParticleFilter(GridMap &gridmap, GridMap &tmpgaussianMap, double region[2][2],bool buseKLD,int particlenum)  ;

	 void initParticles();
	 bool inBoundingBox(int x, int y);
	 bool isValidPosition(int x, int y);
	 int GetnumParticles();
	 bool getParticle(int i,Particle &particle) ;
	 int GetRandomNum(int num);
	 double GetGuassian();
	 void resampleKLD();
	 Pose getMean();
	 Pose getVar(Pose mean);
	 double getEff();
	  double normalizeWeights() ;
	void moveStochastic(double move[]) ;
	void moveNonStochastic(Pose &move);
	void update(Pose &movement, vector<Pose> &laserpoints, double maxObservation, bool alwayUpdate);
	void updateWeights(vector<Pose> &laserpoints, double maxObservation) ;
	double evalWeightScanMatch (int index, vector<Pose> &laserpoints, double maxObservation);
	void resample() ;
private:

    double stdVRange;
    double stdVR;
    double stdVTheta;
    double CovRange;
	double metersPerPixel;

    // Instance variables
    int numParticles;
    GridMap map;
    //private Random random = new Random(); // to generate a particle.
    bool debug ;

    vector<Particle> particles;
    int minDisToObstacle;
    int maxNumber ;
    int NEFFFACTOR ;

	Pose accuMovement;
    
    //double accuMovement[3] ;
    
    int x0;
	int y0;
	int x1;
	int y1;
    
    //private ExecutorService executor;
    double lastEff;
    
    double bsz ;
	int min_samples ;
	double quantile ;
	double error ;
	KLD *pkld;
	
	bool useKLD;
	GridMap gaussianMap;
	
	double omitRange; // if laser scans is two close to the robot, omit them
    double maxRange;

    double mask_out_rad[];
};

