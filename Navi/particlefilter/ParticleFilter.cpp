
#include "ParticleFilter.h"

#include <time.h>
#include <sys/time.h>


ParticleFilter::ParticleFilter(void)
{
}
 ParticleFilter::ParticleFilter(GridMap &gridmap, GridMap &tmpgaussianMap, double region[2][2],bool buseKLD,int particlenum) 
 {
	 time_t t;
	 srand((unsigned int)time(&t));

	printf("ParticleFilter ParticleFilter************1111****************\n");
    stdVRange = 1;
    stdVR = 0.1;
    stdVTheta = 0.1;
    CovRange = pow(stdVRange,2);

    //private Random random = new Random(); // to generate a particle.
    //debug = false;
    maxNumber = 30;
    NEFFFACTOR = 5.0;
	lastEff = 0.0;

	if(particlenum<150)
    	particlenum = 150;
    
  //  x0=Integer.MAX_VALUE;
	//y0=Integer.MAX_VALUE;
    
  
    bsz = 0.5;
	min_samples = maxNumber;
	quantile = 0.99;
	error = 0.5;
	omitRange = 0.2; // if laser scans is two close to the robot, omit them



	 map = gridmap;
     gaussianMap = tmpgaussianMap;

	 metersPerPixel = map.metersPerPixel;
     useKLD = buseKLD;
	
     if(useKLD)
        	pkld = new  KLD(quantile, error, bsz, min_samples, " ");
	
      //  executor = Executors.newFixedThreadPool(8);
        minDisToObstacle = (int)(0.1/map.metersPerPixel);
        //getObstacleBoundingBox();

		
        double xmin = min(region[0][0], region[1][0]);
        double xmax = max(region[0][0], region[1][0]);
        double ymin = min(region[0][1], region[1][1]);
        double ymax = max(region[0][1], region[1][1]);
        x0 = (int)((xmin-map.x0)/metersPerPixel);
        x1 = (int)((xmax-map.x0)/metersPerPixel);
        y0 = (int)((ymin-map.y0)/metersPerPixel);
        y1 = (int)((ymax-map.y0)/metersPerPixel);

		
        
        numParticles = (int)min((double)10000, (particlenum*(ymax-ymin)*(xmax-xmin)));
       // System.out.println("generated particles "+numParticles+" for "+(ymax-ymin)+" vs "+(xmax-xmin));
      //  System.out.println((y1-y0)*(x1-x0)+" "+this.numParticles);
        initParticles();

		printf("x0 = %d, x1=%d,y0=%d,y1=%d ,numParticles=%d \n", x0,x1,y0,y1, numParticles);
      //  maxRange= config.getRoot().getDouble(Main.lidarName+".max_range", Double.MAX_VALUE);
       // mask_out_rad = config.getRoot().getDoubles(Main.lidarName+".mask_out_deg", null);

	
 }

ParticleFilter::~ParticleFilter(void)
{
}
int ParticleFilter::GetRandomNum(int num)
{

	double d = num ;
	//struct timeval tpstart;

	//gettimeofday(&tpstart,NULL);
	//srand(tpstart.tv_usec);

	return (int)(d*rand()/(RAND_MAX));



}
double ParticleFilter::GetGuassian()
{
	static double V1, V2,S;
	static int phase = 0;
	double X;

	struct timeval tpstart;

	gettimeofday(&tpstart,NULL);
	srand(tpstart.tv_usec);

	if (phase == 0)
	{
		do{
			double U1 = (double)rand()/RAND_MAX;
			double U2 = (double)rand()/RAND_MAX;

			V1 = 2*U1 -1;
			V2 = 2*U2 -1;
			S = V1*V1 + V2*V2;
		}while(S>=1 || S==0);

		X = V1*sqrt(-2*log(S)/S);
	}
	else
	{
		X = V2*sqrt(-2*log(S)/S);
	}

	phase = 1 - phase;

	return X;
}



//随机撒粒子，概率均匀分布
void ParticleFilter::initParticles() 
{


		int index = 0;
    	double initWeight = 1.0/(double)numParticles;//平均分布
    	//double[]angles = new double[this.numParticles];
    	while (index<numParticles) //循环生成粒子
		{
    		
			Particle p;
    		int x,y;
    		do{
    			x = GetRandomNum(map.width-1);
    			y = GetRandomNum(map.height-1);
    		}while (!isValidPosition(x,y)||(!inBoundingBox(x,y)));
    	
    		p.x = x*metersPerPixel + map.x0;
			p.y = y*metersPerPixel + map.y0;
			p.theta = degrees_to_radians(GetRandomNum(360));
			p.weight = initWeight;




			particles.push_back(p);
    			//angles[index] = particles[index].theta;
    		index++;
    	}



    	/*
    	FileWriter fw;
		try {
			fw = new FileWriter("angles.txt",false);
	    	for(double angle:angles)
	    	{
	    		fw.write(String.valueOf(angle));
	    		fw.write(" ");
	    	}
	    	fw.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		//*/
    }


bool   ParticleFilter::isValidPosition(int x, int y)
{
    	int xx,yy;
    	for (int ix = -minDisToObstacle; ix<minDisToObstacle; ix++) 
		{
    		for (int iy = -minDisToObstacle; iy<minDisToObstacle; iy++) {
    			yy = y+iy;
    			xx = x+ix;
    			if(yy>=0&&yy<map.height&&xx>=0&&xx<=map.width)
    			{
    				if (map.data[yy*map.width+xx] == (BYTE)255 )
    				{
    					return false;
    				}
    			}
    			else
    			{
    				return false;
    			}
    		}
    	}
    	return true;
    }
    
bool ParticleFilter::inBoundingBox(int x, int y)
{
    	return x>=x0&&x<=x1&&y>=y0&&y<=y1;
   }

int ParticleFilter::GetnumParticles() 
{
        return numParticles;
}

bool ParticleFilter::getParticle(int i,Particle &particle) 
{

	int size = particles.size();
	if (i>=size)
		return false;
	particle.x = particles.at(i).x;
	particle.y = particles.at(i).y;
	particle.theta = particles.at(i).theta;
	particle.weight = particles.at(i).weight;

	return true;

     
}

void ParticleFilter::resample() 
{
    	/*if(lastEff>numParticles/NEFFFACTOR) {
    	//	if(debug)System.out.println("eff "+lastEff);
    		return;
    	}
    	
        int[] keepIdx= new int[numParticles];
        double[]select = stratifiedRandom(numParticles); 
        double[]w= calcRunSum(particles); 

        int ctr=0; 
        for (int i = 0; i<numParticles; i++)
           while (ctr<numParticles && select[ctr]<=w[i])
           {
        	   //System.out.println(ctr+" "+i+" "+select[ctr]+" accu "+w[ctr]+" "+particles[ctr].weight);
        	   keepIdx[ctr]= i;
               ctr++; 
           }
        updateParticle1(keepIdx);*/
        
    }
 void ParticleFilter::resampleKLD() 
 {
    	if(lastEff> (double)numParticles/NEFFFACTOR) {   //NEFFFACTOR = 5.0
    		//if(debug)System.out.println("eff "+lastEff);
    		return;
    	}
    	//if(debug)System.out.println("resample for eff "+lastEff);
    	pkld->reset();
    	
        int index = GetRandomNum(numParticles);
        double beta = 0;
        double maxWeight = 0;
        for (int i = 0; i < particles.size(); i++) //更新最大权重
        	if(particles.at(i).weight>maxWeight)        	
			{
        		maxWeight = particles.at(i).weight;
        	}
        //System.out.println("max weight "+maxWeight);

		int NO = 0;
        vector<Particle> newParticles;
        int minSample = min_samples;
        while(newParticles.size()<minSample) {
        	beta += (double)GetRandomNum(numParticles)/numParticles*2*maxWeight;
        	while (beta>particles.at(index).weight){
        		beta -= particles.at(index).weight;
        		index = (index+1)%numParticles;
        	}
        	Particle p ;
        	double tempDis = GetGuassian()*stdVR;  //stdVR = 0.1
        	double tempTurn = GetGuassian()*stdVTheta;

			Pose deta;
			deta.x = tempDis*cos(tempTurn);
			deta.y = tempDis*sin(tempTurn);
			deta.theta = tempTurn;
        	Pose newpose = LinAlg::xytMultiply(particles.at(index).getPose(),deta);
        	p.x = newpose.x;
        	p.y = newpose.y;
        	p.theta = newpose.theta;
        	p.weight = particles[index].weight;

			vector<double> sample;
			sample.push_back(p.x);
			sample.push_back(p.y);
			sample.push_back(p.theta);
        	minSample=max(pkld->update(sample), minSample);
        	//System.out.println(index+" "+particles[index].weight +" min sample "+minSample);
			newParticles.push_back(p);

			NO++;
			printf("NO = %d\n",NO);
			if(NO>1000)
				break;
        }
        
         numParticles = newParticles.size();//+extra;
       // if(debug)System.out.println("population changed to "+this.numParticles);

		 particles.clear();
    	
			printf("&&&&&&&&new size = %d\n",numParticles);

    	for (int i=0;i<numParticles;i++)
    	{
			particles.push_back(newParticles.at(i));
    		//System.out.println(i+" has weight "+particles[i].weight);
    	}
    	//double initWeight = 1.0/numParticles;
    	//for(;i<numParticles;i++)    		newP[i] = initParticle(initWeight);
        
    }


Pose ParticleFilter::getMean()
{
		Pose mean;

		mean.x = 0.0;
		mean.y = 0.0;
		mean.theta = 0.0;

		for(int i=0;i<particles.size();i++)
		{
			mean.x += particles.at(i).x;
			mean.y += particles.at(i).y;
			mean.theta += particles.at(i).theta;

		}

		mean.x /= particles.size();
		mean.y /= particles.size();
		mean.theta /= particles.size();
		
		return mean;
}
    
double ParticleFilter::getEff()
{
   	return lastEff;
}
Pose ParticleFilter::getVar(Pose mean)
{
		Pose cov ;
		cov.x = 0.0;
		cov.y = 0.0;
		cov.theta = 0.0;
		
		vector<Particle>::iterator iter;

		for (iter=particles.begin();iter<particles.end();iter++)
		{
			cov.x += pow(iter->x - mean.x, 2);
			cov.y += pow(iter->y - mean.y, 2);
			cov.theta += pow(iter->theta - mean.theta, 2);
		}

		int size = particles.size();
		cov.x = cov.x/size;
		cov.y = cov.y/size;
		cov.theta = cov.theta/size;

		return cov;


	}

int inxx = 0;  //干什么用？
 double ParticleFilter::normalizeWeights() 
 {
	  inxx++;
	 int length = particles.size();    
	 double wSum = 0;

        for (int i = 0; i < length; i++)
        {
        	 wSum += particles.at(i).currentWeight;
        }

        for (int i = 0; i < length; i++)
        	particles.at(i).weight *= particles.at(i).currentWeight / wSum;//为什么是*=，一定弄懂原理


		//归一化
        wSum = 0;
        for (int i = 0; i < length; i++)
        {
        	 wSum += particles.at(i).weight;
        }
        
        for (int i = 0; i < length; i++)
		{
        	particles[i].weight = particles.at(i).weight / wSum;
			
			/*if(fabs(particles[i].x)<0.3 && fabs(particles[i].y)<0.3 && (fabs(particles[i].theta)<PI/4||fabs(particles[i].theta)<(PI/4-2*PI)))
			{
					//printf("init p.x = %f,p.y=%f,p.theta=%f\n",p.x,p.y,p.theta);
printf(" &&&&& no=%d,i =%d, weight = %f, x =%f,y=%f,z=%f\n",inxx,i,particles[i].weight,particles[i].x,particles[i].y,particles[i].theta);
			}
		*/

		}
        double eff = 0;
        for (int i = 0; i < length; i++)
        eff += pow(particles.at(i).weight, 2); //平方相加，再取倒数，有什么数学意义？
        return 1/eff;
 }


void ParticleFilter::moveStochastic(double move[]) 
{
    	double r = sqrt(move[0]*move[0]+move[1]*move[1]);
    	
        for (int index = 0; index < numParticles; index++) 
		{
        	double localR = r*(1+GetGuassian());
			Pose deta;
			deta.x = localR*cos(move[2]);
			deta.y = localR*sin(move[2]);
			deta.theta = move[2];
        	Pose newpose = LinAlg::xytMultiply(particles.at(index).getPose(),deta);
        	particles.at(index).x = newpose.x;
        	particles.at(index).y = newpose.y;
        	particles.at(index).theta = newpose.theta;
        }
        //if (debug) System.out.println("particles applyMove Exit");
    }
void ParticleFilter::moveNonStochastic(Pose &move)
{

        for (int index = 0; index < numParticles; index++) 
		{
        	Pose newpose = LinAlg::xytMultiply(particles.at(index).getPose(),move);
        	particles.at(index).x = newpose.x;
        	particles.at(index).y = newpose.y;
        	particles.at(index).theta = newpose.theta;
        }
       // if (debug) System.out.println("particles applyMove Exit");
}


	double ParticleFilter::evalWeightScanMatch (int index, vector<Pose> &laserpoints, double maxObservation)
	{
		 IntArray2D* scores = NULL;
		 Pose tmp;

		tmp = particles[index].getPose();

		scores = gaussianMap.scores2D(laserpoints,
                    particles.at(index).x - metersPerPixel, 1,
                    particles.at(index).y - metersPerPixel, 1,
                    particles.at(index).theta
                    , tmp, NULL);

					//输入的矩阵大小是1*1，所以vs只有一个值，而不是数组，表示匹配上的点的值的和
		particles.at(index).currentWeight = scores->vs[0];
	        //System.out.println(i.vs.length+" "+i.vs[0]);
	        //System.out.println(index+" "+particles[index].weight);
		//_RPT2(_CRT_WARN,"IDX = %d,curweight=%f\n",index,particles.at(index).currentWeight);
		if (scores!= NULL)
			delete scores;
	     
		return particles.at(index).weight;
		
	}



void ParticleFilter::updateWeights(vector<Pose> &laserpoints, double maxObservation) 
{

        //int zeros = 0;
        //double maxWeight = 1;
        int x,y;
       // Collection<Callable<Double>> tasks = new ArrayList<Callable<Double>>();
       // ArrayList<double[]>points = Main.laser2Points(rr, omitRange, maxRange, mask_out_rad);
        ///*
        for (int i = 0; i < numParticles; i++) {
        	if(particles[i].weight!=0)
        	{
        		x = (int)((particles.at(i).x-map.x0)/metersPerPixel);
            	y = (int)((particles.at(i).y-map.y0)/metersPerPixel);
				//判断点周围是否存在障碍物
            	if (isValidPosition(x,y)) 
				{
				    //算出currentWeight，此时还没有归一化
            		evalWeightScanMatch(i, laserpoints, maxObservation);
					//printf(" i= %d, currentWeight=%f\n", i,particles[i].currentWeight);

            	} else particles.at(i).weight = 0;
            		
        	}//else particles[i].weight=0;
            //if (particles[i].weight > maxWeight) maxWeight = particles[i].weight;
            //if (particles[i].weight == 0) zeros++;
        }
      //  try {
		//	executor.invokeAll(tasks);
		//} catch (InterruptedException e) {
			//e.printStackTrace();
		//}
        //*/

		//重新算weight
        lastEff = normalizeWeights();	//lastEff = weight平方相加的倒数
        //if (debug) System.out.println("Calc Weights Max wt " + maxWeight + " Zeros " + zeros);
        //if (maxWeight < .01) return false;
    }
  void ParticleFilter::update(Pose &movement, vector<Pose> &laserpoints, double maxObservation, bool alwayUpdate)
  {
    	//if(movement == NULL || observations == NULL)return;

		//所有的粒子加上位姿增量
    	moveNonStochastic(movement);

		//根据位姿和增量，求出新的位姿
		//这里其实就是利用位姿增量，但是没有起判断作用
    	accuMovement = LinAlg::xytMultiply(accuMovement, movement);
		accuMovement.theta = MathUtil::mod2pi(accuMovement.theta);
		
		if(alwayUpdate||fabs(accuMovement.theta)>0.1 || accuMovement.x*accuMovement.x+accuMovement.y*accuMovement.y>0.01)
    	{
    		//LinAlg.print(accuMovement);
			accuMovement.x = 0.0;
			accuMovement.y = 0.0;
			accuMovement.theta = 0.0;

			//更新粒子的weight
    		updateWeights(laserpoints, maxObservation);

			//重采样
    		if(true){
    			if(useKLD)resampleKLD();
        		else resample();
    		}else 
			{
    	    /*	FileWriter fw;
    			try {
    				fw = new FileWriter("weights.txt",false);
    		    	for(Particle p:particles)
    		    	{
    		    		fw.write(String.valueOf(p.weight));
    		    		fw.write("\n");
    		    	}
    		    	fw.close();
    			} catch (IOException e) {
    				// TODO Auto-generated catch block
    				e.printStackTrace();
    			}*/
    		}
    		
    	}
    }
