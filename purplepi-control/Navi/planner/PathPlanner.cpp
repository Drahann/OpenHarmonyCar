//#include "StdAfx.h"
#include "PathPlanner.h"

#include "../type.h"

#include <fstream>





  void convolveCenteredDisc2DMaxCont(BYTE a[], int width, int height, double radius, double mpp, BYTE r[])
    {
        int pxRadius = (int) ceil(radius/mpp);
        for (int l=-pxRadius; l <= pxRadius; l++) {
            for (int k=-pxRadius; k <= pxRadius; k++) {
                // is it within the radius?
                double k_m = k*mpp;
                double l_m = l*mpp;
                double c00_m = sqrt((k_m - mpp/2)*(k_m - mpp/2) + (l_m - mpp/2)*(l_m - mpp/2));
                double c01_m = sqrt((k_m - mpp/2)*(k_m - mpp/2) + (l_m + mpp/2)*(l_m + mpp/2));
                double c10_m = sqrt((k_m + mpp/2)*(k_m + mpp/2) + (l_m - mpp/2)*(l_m - mpp/2));
                double c11_m = sqrt((k_m + mpp/2)*(k_m + mpp/2) + (l_m + mpp/2)*(l_m + mpp/2));
                double rad = min(min(c00_m, c01_m), min(c10_m, c11_m));

                if (rad >= radius)
                    continue;

                int ymin = max(0, 0-k);
                int ymax = min(height, height - k);
                int xmin = max(0, 0-l);
                int xmax = min(width, width - l);

               /* for (int y = ymin; y < ymax; y++) 
				{
                    int n = (y+k)*width + (xmin+l);
                    int o = y*width + xmin;

                    for (int x = xmin; x < xmax; x++) {
                        r[o] = (int) max(r[o], a[n++]);
                        o++;
                    }
                }*/
				for (int y = ymin; y < ymax; y++) 
				{
        
                    for (int x = xmin; x < xmax; x++) 
					{
                    
						int o = y*width + x;

						if(a[o] == 0xFF)
						{
							r[(y+k)*width + (x+l)] = 0xFF;

						}
                    }
                }

            }
        }
    }






PathPlanner::PathPlanner(void)
{
	robot_geometry_radius = 0.5;
        // width cfg space radius for straight-line maneuvars
	robot_geometry_width = 0.5 ;


	pathPlanning_troughWidth = 1;
	 pathPlanning_upsample =1;
	//???????????????????//
	pathPlanning_maxCost  =  254;

	pathPlanning_minCellCost = 40;
}


PathPlanner::~PathPlanner(void)
{
}

void PathPlanner::setConfig(plannerconfig config,double robot_width, double robot_radius)
{
	robot_geometry_radius = robot_radius ; 
	robot_geometry_width = robot_width;


	pathPlanning_troughWidth = config.troughWidth;
	pathPlanning_upsample = config.upsample;


	pathPlanning_maxCost = config.maxCost;
	pathPlanning_minCellCost = config.minCellCost;

}

void PathPlanner::getVisBoundingBox(GridMap &map,MyRect &bounding){
    	
	bounding.x0 = Integer_MAX_VALUE;
	bounding.y0 = Integer_MAX_VALUE;
	bounding.x1 = Integer_MIN_VALUE;
	bounding.y1 = Integer_MIN_VALUE;


    	for (int y = 0; y < map.height; y++) {
            for (int x = 0; x < map.width; x++) {
            	if (map.data[y*map.width+x] == (unsigned char)254 ){
            		if (x<bounding.x0)bounding.x0 = x;//x0
            		if (x>bounding.x1)bounding.x1 = x;//x1
            		if (y<bounding.y0)bounding.y0 = y;//y0
            		if (y>bounding.y1)bounding.y1 = y;//y1
            	}
            }
    	}
    	return ;
    }
 void PathPlanner::drawTrough(GridMap& gm,
            int map[],
            int width,
            int height,
            double mpp,
            Pose &centerxy,
             Pose &goal,
            double troughWidth)
    {
		//long t0, t1;
		//t0 = TimeUtil.utime();
		
		GridMap trough;
		
		trough.makePixels(gm.x0, gm.y0, gm.width, gm.height, gm.metersPerPixel, 0, false);
		LUT lut;
		trough.makeConstantLUT(255, gm.metersPerPixel,lut);
		
		// make no modifications if only a destination is given
		return;
	}
 bool PathPlanner::renderConfigurationSpace(GridMap &om,
                                                           Pose &centerxy,
                                                           Pose &goal,
                                                           double troughWidth,
                                                           double cfgRadius,ConfigurationSpaceData &res)
    {
        // terrain map params
        double cs    = om.metersPerPixel;
        double x0    = om.x0;
        double y0    = om.y0;
        double sizex = om.width  * om.metersPerPixel;
        double sizey = om.height * om.metersPerPixel;

        // config space params
        int upsample = pathPlanning_upsample;
        int maxCost =  pathPlanning_maxCost;
        int width    = om.width  * upsample;
        int height   = om.height * upsample;
        double mpp   = cs / upsample;
        int cw = width + 2;
        int ch = height + 2;

        GridMap cfggm;
		cfggm.makeMeters(x0, y0, sizex, sizey, mpp, 0xff);
    
		BYTE *cfggmgrid = new BYTE[cfggm.width*cfggm.height];
		memset(cfggmgrid, 0, cfggm.width*cfggm.height*sizeof(BYTE));

        BYTE *terrainMap = new BYTE[cw*ch];
		memset(terrainMap, 0, cw*ch*sizeof(BYTE));

        BYTE *cspace = new BYTE[cw*ch];
		memset(cspace, 0, cw*ch*sizeof(BYTE));

        // populate terrain map
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int omx = (int) x / upsample;
                int omy = (int) y / upsample;
                int cost = om.data[omy*om.width + omx]&0xFF;

                // only obstacle costs are supported
                if (cost == 0xFF)
                    terrainMap[(y+1)*cw + (x+1)] = 0xFF;
            }
        }

        // add infinite borders for configuration space
        for (int y = 0; y < ch; y++)
            terrainMap[(y)*cw + (0)] = 0xFF;

        for (int y = 0; y < ch; y++)
            terrainMap[(y)*cw + (cw-1)] = 0xFF;

        for (int x = 0; x < cw; x++)
            terrainMap[(0)*cw + (x)] = 0xFF;

        for (int x = 0; x < cw; x++)
            terrainMap[(ch-1)*cw + (x)] = 0xFF;

		GridMap terrain;
		terrain.makePixels(x0, y0, cw, ch, mpp, 0, false);

		BYTE *pterrain = new BYTE[cw*ch];
		 for (int y=0; y < ch; y++)
            for (int x=0; x < cw; x++)
                pterrain[y*cw + x]  = (BYTE) terrainMap[(y)*cw + (x)];

		 terrain.setData(pterrain,cw*ch);

		 
		 delete pterrain;
	
		//no use
      //  drawTrough(cfggm, terrainMap, width, height, mpp, centerxy, goal, troughWidth);

        // Max disk convolution to generate cost map
        convolveCenteredDisc2DMaxCont(terrainMap, cw, ch, cfgRadius, om.metersPerPixel/upsample, cspace);


		/*GridMap test;
		test.makePixels(x0, y0, cw, ch, mpp, 0, false);

		BYTE *ptest = new BYTE[cw*ch];
		 for (int y=0; y < ch; y++)
            for (int x=0; x < cw; x++)
                ptest[y*cw + x]  = (BYTE) cspace[(y)*cw + (x)];

		 test.setData(ptest,cw*ch);

		 
		 delete ptest;
		 */

        // build gridmap for configuration space (distance-first planning)
       for (int y=0; y < height; y++)
            for (int x=0; x < width; x++)
                cfggmgrid[y*cfggm.width + x]  = (BYTE) cspace[(y+1)*cw + (x+1)];

        // fill extra pixels with infinite cost
        for (int y=height; y < cfggm.height; y++)
            for (int x=0; x < cfggm.width; x++)
                cfggmgrid[y*cfggm.width + x] = (BYTE) 0xFF;
        for (int y=0; y < cfggm.height; y++)
            for (int x=width; x < cfggm.width; x++)
                cfggmgrid[y*cfggm.width + x] = (BYTE) 0xFF;

        cfggm.setData(cfggmgrid,cfggm.width*cfggm.height);

		

        // Copy to data structure
       
        res.maxCost = maxCost;
        res.gm = cfggm;
        res.cfgSpace = cspace;
        res.width = cw;
        res.height = ch;

		/*if(cfggm.data !=NULL)
		{
			delete cfggm.data;
		}*/


		 //

		delete [] cfggmgrid;

        delete [] terrainMap ;
        delete [] cspace ;

        return true;
    }
bool PathPlanner::computeGoalTarget(GridMap &gm, Pose centerxy,
                                     BYTE reachable[], Pose goal,double res[3])
   {
       int width  = gm.width;
       int height = gm.height;

       // Get coordinate of pixel (0,0) in meters (center of pixels)
       double XY0[2];
	   gm.getXY0(XY0[0],XY0[1]);
       XY0[0] += gm.metersPerPixel/2.0;
       XY0[1] += gm.metersPerPixel/2.0;

       // Find closest reachable cell to the next endpoint on the path

       double closest[2]  ; 
       double bestSqDist   = Double_MAX_VALUE;

       for (int y=0; y < height; y++)
       {
           for (int x=0; x < width; x++)
           {
               if (reachable[y*width + x] == 0)
                   continue;

               double dx = goal.x - (x*gm.metersPerPixel + XY0[0]);
               double dy = goal.y - (y*gm.metersPerPixel + XY0[1]);
               double sqdist = dx*dx + dy*dy;
               if (sqdist < bestSqDist)
               {
                   bestSqDist = sqdist;
                   closest[0] = x*gm.metersPerPixel + XY0[0];
                   closest[1] = y*gm.metersPerPixel + XY0[1];
               }
           }
       }

       // did we actually find a target?
       if (bestSqDist == Double_MAX_VALUE) {
           return false;
       }

       // Return {x, y, distance-to-goal}
       res[0] = closest[0];
	   res[1] = closest[1];
	   res[2] = sqrt(bestSqDist);
   }

bool PathPlanner::getWavefront(GridMap &gm, double target[], Pose centerxy,double **wavefront)
    {
       // long t0, t1;
       // t0 = TimeUtil.utime();

        // compute index of starting cell and goal cell
        double XY0[2]; 
	    gm.getXY0(XY0[0],XY0[1]);
        int tx = (int) ((target[0]   - XY0[0]) / gm.metersPerPixel);
        int ty = (int) ((target[1]   - XY0[1]) / gm.metersPerPixel);
        int px = (int) ((centerxy.x - XY0[0]) / gm.metersPerPixel);
        int py = (int) ((centerxy.y - XY0[1]) / gm.metersPerPixel);
        int width = gm.width;
        int height = gm.height;

        int maxCost = pathPlanning_maxCost;
        double minCellCost = pathPlanning_minCellCost;

	/*	double *targetRes;
		int length;
		LinAlg::resize(target,2, 2,targetRes,&length);*/
		Pose targetPos;
		targetPos.x = target[0];
		targetPos.y = target[1];

        Wavefront::getWavefront(gm,  centerxy,targetPos,minCellCost,maxCost,wavefront);
       // t1 = TimeUtil.utime();

        return true;
    }



bool PathPlanner::descendWavefront(int pos[], int goal[],
            int width, int height,
            double wave[],vector<vector<int> > &bestPath)
   {
    	return descendWavefront(pos, goal, width, height, wave, Wavefront::NEIGHBORS,bestPath);
    }

 /** Descend wavefront from 'pos' to 'goal' given allowed neighbor offsets.
     **/
bool PathPlanner::descendWavefront(int pos[], int goal[] ,
                                                   int width, int height,
                                                   double wave[],
                                                   int neighbors[8][2],vector<vector<int> > &bestPath)
{
       int tx = goal[0];
       int ty = goal[1];
       int px = pos[0];
       int py = pos[1];

      // ArrayList<int[]> bestPath = new ArrayList<int[]>();
       int x = px;
       int y = py;
       double min;
     

       // include initial position for segment-based control
	   vector<int> temp;
	   temp.push_back(px);
	   temp.push_back(py);
	   bestPath.push_back(temp);
    
       // Continue as long as we aren't at the goal
       int i = 0;
       while (x != tx || y != ty)
       {
           // reset min parameters
           min = Double_MAX_VALUE;
			int nmin[2];
			nmin[0] =0;
			nmin[1] = 0;
            for (int i=0;i<8;i++)
           {
			   	int n[2];
				n[0] = *(*(neighbors+i)+0);
			    n[1] = *(*(neighbors+i)+1);
               int nx = x + n[0];
               int ny = y + n[1];
               if (nx < 0 || nx >= width || ny < 0 || ny >= height)
                   continue;

               if (wave[ny*width+nx] < 0)
                   continue;

               if (wave[ny*width+nx] < min)
               {
                   min = wave[ny*width+nx];
                   nmin[0] = n[0];
				   nmin[1] = n[1];
               }
           }

           // were any of the actions valid?
           if (nmin[0] == 0 && nmin[1] == 0)
           {
               printf("NFO: No wavefront gradient descent step was acceptable. Breaking loop.");
               break;
           }

           x = x + nmin[0];
           y = y + nmin[1];

		   vector<int> vt;
		   vt.push_back(x);
		   vt.push_back(y);

		   bestPath.push_back(vt);
			
          
       }

       return true;
   }


   /** Use wavefront to compute the best path from the current robot
     * pose to the target using the gridmap.
     *
     * @param gm       - GridMap built to include configuration space
     *                   and cost map (decays)
     * @param target   - XYT target within driveable part of GridMap
     * @param centerxy - Center of robot (not pose)
     **/
bool PathPlanner::computeWavefrontPath(ConfigurationSpaceData& cspace,
                                      double target[],
                                      double centerxy[],
                                      double wavefront[],vector<vector<double> > &path)
   {
      // long t0, t1;
       //t0 = TimeUtil.utime();
	   //?????
    //   GridMap gm = cspace.gm;

      
       // compute index of starting cell and goal cell

       double XY0[2];
	   cspace.gm.getXY0(XY0[0], XY0[1]);
       int tx = (int) ((target[0]   - XY0[0]) / cspace.gm.metersPerPixel);
       int ty = (int) ((target[1]   - XY0[1]) / cspace.gm.metersPerPixel);
       int px = (int) ((centerxy[0] - XY0[0]) / cspace.gm.metersPerPixel);
       int py = (int) ((centerxy[1] - XY0[1]) / cspace.gm.metersPerPixel);
       int width = cspace.gm.width;
       int height = cspace.gm.height;

       // descend wavefront to generate path
	   int pxy[2];
	   int txy[2];
	   pxy[0] =  px;
	   pxy[1] =  py;
	   txy[0] = tx;
	   txy[1] = ty;

	   vector<vector<int> > bestPath;
       descendWavefront(pxy,txy,width, height,wavefront,bestPath);

      
       short length = (short) bestPath.size();
    
       for (int i=0; i < length; i++)
       {
           int x = bestPath.at(i).at(0);
		   int y = bestPath.at(i).at(1);
           double xpath = x * cspace.gm.metersPerPixel + XY0[0] + cspace.gm.metersPerPixel/2.0;
           double ypath = y * cspace.gm.metersPerPixel + XY0[1] + cspace.gm.metersPerPixel/2.0;
           // we don't use psuedo-distance here because the smoother will do that, later
           
		   vector<double> vt;
		   vt.push_back(xpath);
		   vt.push_back(ypath);
		   path.push_back(vt);
       }

      

       return true;
   }
bool PathPlanner::calculateBasePath(GridMap& terrain_map,
                                                   Pose goal,
                                                   double troughWidth,
                                                   double radius,
                                                   Pose& centerxy,WavefrontResults &res)
    {
        //long t0, t1;
       // t0 = TimeUtil.utime();

        // start results structure
        timeval begintime;
		timeval endtime;

        res.radius = radius;

        // Build gridmap representation of world
          gettimeofday(&begintime,NULL);
		renderConfigurationSpace(terrain_map, centerxy, goal, troughWidth, res.radius,res.cspace);

	

        // maximum acceptable cost
		int maxCost = pathPlanning_maxCost;

        // get reachable terrain
        int nreachableLengh=0; 

		BYTE *pReachable = NULL;
		res.reachable = NULL;	
		res.cspace.gm.getConnectedWithin(centerxy, maxCost,&res.reachable,nreachableLengh);
		
        if (res.reachable == NULL)
		{
            //updateProcFlags("ALL_TERRAIN_UNREACHABLE");
            return false;
        }

		
        // Find best goal in goal list
         double targetXYD[3];
		computeGoalTarget(res.cspace.gm, centerxy, res.reachable, goal,targetXYD);

        res.target[0] = targetXYD[0];
		res.target[1] = targetXYD[1];
        res.targetDist = targetXYD[2];

		if(res.reachable != NULL)
		{
			delete [] res.reachable;
		}

        // Compute wavefront graph
       getWavefront(res.cspace.gm, res.target, centerxy, &(res.wavefront) );

        // Get path to traverse
       /* res.path = new path_t();
        if (res.wavefront == null)
            res.path.length = 0;
        else
            res.path = computeWavefrontPath(res.cspace, res.target, centerxy, res.wavefront);

		
        return res;*/

		if ((*res.wavefront) == NULL)
			return false;  //no path;
		else
		{
			double xy[2];
			xy[0] = centerxy.x;
			xy[1] = centerxy.y;
			computeWavefrontPath(res.cspace, res.target, xy, (res.wavefront),res.path);

		}
gettimeofday(&endtime,NULL);
//printf("path time = %f\n", (endtime.tv_sec - begintime.tv_sec)*1000 + (double)(endtime.tv_usec -begintime.tv_usec)/1000);
	   delete [] res.wavefront;
		return true;
    }
 bool PathPlanner::plan2(Pose thegoal, GridMap &visMap, Pose curpose, vector<vector<double> > &path)
{

	
        // Get center of robot

        Pose centerxy = curpose;
        // regular configuration space radius (tight, but guarantees turning in place)
        double fullRadius = robot_geometry_radius;
        // width cfg space radius for straight-line maneuvars
        double width = robot_geometry_width;
        double radius = fullRadius;
        if(1)      // if (config.requireBoolean("pathPlanning.enableWidthCfgSpace"))
            radius = width;

        // compute all paths
        double troughWidth = pathPlanning_troughWidth;

		bool bGoalNull = false;

      MyRect bounding ;
		getVisBoundingBox(visMap,bounding);
		GridMap terrain;
		GridMap terrain2;
		visMap.cropPixels(bounding.x0, bounding.y0, bounding.x1-bounding.x0, bounding.y1-bounding.y0, false,terrain);

	//   ??????

	if(terrain.metersPerPixel<0.08)
	{
		terrain.decimateMax(2,terrain2);//.resizePixels(x0_m, y0_m, width_m, height_m, false);//.dilate((byte)255, (int)Math.ceil(robotRadius/gm.metersPerPixel));
    	}
	else
	{
		terrain2 = terrain;
	}
		
		
		Pose goal = thegoal;
    	BYTE *connected = NULL;
		int length = 0;
    //	terrain2.getConnectedWithin(curpose, 254,&connected,length);
	  terrain2.getConnectedWithin(centerxy, 254,&connected,length);

        if (connected == NULL)
            connected = new BYTE[terrain2.width*terrain2.height];
        int gix = (int)((thegoal.x - terrain2.x0) / terrain2.metersPerPixel);
        int giy = (int)((thegoal.y - terrain2.y0) / terrain2.metersPerPixel);
        if (gix < 0 || gix >= terrain2.width ||
            giy < 0 || giy >= terrain2.height ||
            connected[giy*terrain2.width + gix] == (BYTE)0)
        {
            Pose pxy ;
            double bestGoal[2];
            double closest = Double_MAX_VALUE;
            for (int y = 0; y < terrain2.height; y++) 
			{
                for (int x = 0; x < terrain2.width; x++) 
				{
                    if (connected[y*terrain2.width + x] != (BYTE)1)
                        continue;
                    pxy.x = terrain2.x0 + (x + 0.5)*terrain2.metersPerPixel;
                    pxy.y = terrain2.y0 + (y + 0.5)*terrain2.metersPerPixel;
                    if (LinAlg::DistancePose(pxy, thegoal) < closest) {
                        bestGoal[0] = pxy.x;
                        bestGoal[1] = pxy.y;
                        closest = LinAlg::DistancePose(pxy, thegoal);
                    }
                }
            }
            if (closest != Double_MAX_VALUE) {
                goal.x = bestGoal[0];
				goal.y = bestGoal[1];
                gix = (int)((goal.x - terrain2.x0) / terrain2.metersPerPixel);
                giy = (int)((goal.y - terrain2.y0) / terrain2.metersPerPixel);
            }
            else
                bGoalNull = true;

        }
        if(bGoalNull)
		{
        	printf("goal is null\n");
        	return false;
        }

        WavefrontResults result;
		result.reachable = NULL;

		
		
		calculateBasePath(terrain2, goal, troughWidth, radius, centerxy,result);

		path = result.path;
        
       /* // set display copies
        display_cspace      = result.cspace.gm;
        display_reachable   = result.reachable;

        // if we couldn't find a goal
        if (result.target == null) {
            display_costmap     = null;
            TIPhysteresis = false;
            return null;
        }*/

	
	if(terrain.data == NULL)
			delete [] terrain.data;
		if(terrain2.data == NULL)
			delete [] terrain2.data;
		if(connected!=NULL)
		{
			delete [] connected;
		}

		return true;
    }
    
