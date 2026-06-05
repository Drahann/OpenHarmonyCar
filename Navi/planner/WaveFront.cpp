
#include "WaveFront.h"



 int Wavefront::NEIGHBORS[8][2] = { { 0,  1},    // Up
                                  {-1,  1},    // Up+Left
                                  {-1,  0},    // Left
                                  {-1, -1},    // Left+Down
                                  { 0, -1},    // Down
                                  { 1, -1},    // Down+Right
                                  { 1,  0},    // Right
                                  { 1,  1} };  // Right+Up


Wavefront::Wavefront()
{
}


Wavefront::~Wavefront()
{
}	




bool Wavefront::getWavefront(GridMap &gm,  Pose &pos, Pose &target,
                                        double minCellCost, int maxCost,double **wavemap)
    {
        // terminate upon connecting goal to pose
        int steps = 0;
        return getWavefront(gm, pos, target, minCellCost, maxCost, steps, NEIGHBORS, wavemap);
    }

    /** Calculate wavefront map from cost map to a finite number of steps.
      * Uses 8-connected neighbors
      *
      * @param gm          - GridMap on which to plan
      * @param pose        - Robot pose.  Required to be within gridmap and in
      *                    - the *vehicle center* (for configuration space)
      * @param target      - Goal for wavefront.  Also must be in gridmap range
      * @param minCellCost - Minimum cost for a cell (enforced: must exceed zero)
      * @param maxCost     - Maximum cost used to compute driveable terrain.
      * @param steps       - Number of nodes to expand until quitting
      *                      Set steps < 1 to run until reaching "pose"
      **/
bool Wavefront::getWavefront(GridMap &gm, Pose &pos, Pose &target,
                                        double minCellCost, int maxCost, int steps,double **wavemap)
    {
        return getWavefront(gm, pos, target, minCellCost, maxCost, steps, NEIGHBORS,wavemap);
    }

    /** Calculate wavefront map from cost map to a finite number of steps.
      * @param gm          - GridMap on which to plan
      * @param pose        - Robot pose.  Required to be within gridmap and in
      *                    - the *vehicle center* (for configuration space)
      * @param target      - Goal for wavefront.  Also must be in gridmap range
      * @param minCellCost - Minimum cost for a cell (enforced: must exceed zero)
      * @param maxCost     - Maximum cost used to compute driveable terrain.
      * @param steps       - Number of nodes to expand until quitting
      *                      Set steps < 1 to run until reaching "pose"
      * @param neighbors   - List of options for cell neighbors (e.g. [0, +1])
      **/

#if 0
bool Wavefront::getWavefront(GridMap &gm, Pose &pos, Pose &target,
                                        double minCellCost, int maxCost,
                                        int steps, int neighbors[8][2],double **wavemap)
    {
        // min cell costs too low to accumulate can make it impossible to find the wavefront
        //ASSERT(minCellCost > 0);

printf("minCellCost = %f, %d ,step =%d\n", minCellCost,maxCost,steps);

        // for convenience, robot and pose indices in the gridmap
        int tx = (int) floor((target.x - gm.x0) / gm.metersPerPixel);
        int ty = (int) floor((target.y - gm.y0) / gm.metersPerPixel);
        int px = (int) floor((pos.x   - gm.x0) / gm.metersPerPixel);
        int py = (int) floor((pos.y   - gm.y0) / gm.metersPerPixel);
        if (tx < 0 || ty < 0 || px < 0 || py < 0 ||
            tx >= gm.width || ty >= gm.height || px >= gm.width || py >= gm.height) {
            printf("WRN: Wavefront cannot compute when target or pose land outside of map.");
            return false;
        }

        // Wavefront data structures
	double *tmpwavmap = new double[gm.width*gm.height];
		
        IntMaxHeap wavefront ;
printf("width= %d ,gm.height =%d\n", gm.width,gm.height);
        // Initialize wavefront and wavemap costs
        for (int y=0; y < gm.height; y++)
        {
            for (int x=0; x < gm.width; x++)
            {
                double cost = ((int) gm.data[y*gm.width+x]) & 0xFF;
                if (cost > maxCost)
                   tmpwavmap[y*gm.width+x] = Double_MAX_VALUE;
			//	   *((*wavemap)+y*gm.width+x)= Double_MAX_VALUE;
                else
                    tmpwavmap[y*gm.width+x]  = -1;
            }
        }

        // get cost for goal
        // min cost in 1
	
        tmpwavmap[ty*gm.width+tx]  = max((double)(((int) gm.data[ty*gm.width + tx]) & 0xFF), minCellCost);
        int c = c = ((tx & 0xFFFF) << 16) | (ty & 0xFFFF);
        wavefront.add(c, -tmpwavmap[ty*gm.width+tx] );

        // compute wavefront
        int step = 0;

        timeval begintime;
	timeval mid1time;
  gettimeofday(&begintime,NULL);
	int count=0;
        while (wavefront.size() > 0)
        {
count++;
            IntHeapPair ihp;
			wavefront.removeMaxPair(ihp);
            int nx = (ihp.o >> 16) & 0xFFFF;
            int ny =  ihp.o        & 0xFFFF;
            int n = ny*gm.width + nx;

            for (int i=0;i<8;i++)
            {
				int neighbor[2];
				neighbor[0] = *(*(neighbors+i)+0);
			    neighbor[1] = *(*(neighbors+i)+1);
                int npx = nx + neighbor[0];
                int npy = ny + neighbor[1];
                int np = npy*gm.width+npx;

                // skip if out of bounds
                if (npx >= gm.width  || npx < 0 ||
                    npy >= gm.height || npy < 0)
                    continue;

                // was it already set?
                if (tmpwavmap[np]  > 0)
                    continue;

                // Get costs for n and n'
                // note that we already ensured that we are not dealing
                // with infinite costs when we initialized some nodes in
                // the wavemap to Double.MAX_VALUE, above
                double nCost  = max(minCellCost, (double)(((int) gm.data[n])  & 0xFF));
                double npCost = max(minCellCost, (double)(((int) gm.data[np]) & 0xFF));

                // Clearly we have 1) a valid node which 2) has not been
                // assigned a wavefront cost and  does not exceed the max
                // cost threshold for its terrain cost.  We must now compute
                // the wavefront value

                // Transition cost is distance between cells (due to diagonals)
                double transition = LinAlg::magnitude(neighbor,2);
                // newval is the minimum possible cost for this node because
                // we are only expanding from the min heap
                double newval = tmpwavmap[n]  + transition*(npCost + nCost)/2.0;
                tmpwavmap[np]  = newval;
                c = ((npx & 0xFFFF) << 16) | (npy & 0xFFFF);

                // store the negative wavefront cost because we're implementing
                // a min heap with a max heap class
                wavefront.add(c, -newval);

                // Stop upon reaching the robot?
                if (steps < 1 && npx == px && npy == py)
                    break ;
            }

            // Stop if we've exceeded the number of steps requested
            if (steps >= 1 && step++ >= steps)
                break ;
        }

	/*	for(int j=0;j<gm.width*gm.height;j++)
		{
		 _RPT1(_CRT_WARN,"%f\n", *((*wavemap)+j));

		}*/
 gettimeofday(&mid1time,NULL);
printf("path wavefront time = %f ,count=%d\n", (mid1time.tv_sec - begintime.tv_sec)*1000 + (double)(mid1time.tv_usec -begintime.tv_usec)/1000,count);
	wavemap = &tmpwavmap;
        return true;
    }

#endif
bool Wavefront::getWavefront(GridMap &gm, Pose &pos, Pose &target,
                                        double minCellCost, int maxCost,
                                        int steps, int neighbors[8][2],double **wavemap)
    {
        // min cell costs too low to accumulate can make it impossible to find the wavefront
        //ASSERT(minCellCost > 0);


        // for convenience, robot and pose indices in the gridmap
        int tx = (int) floor((target.x - gm.x0) / gm.metersPerPixel);
        int ty = (int) floor((target.y - gm.y0) / gm.metersPerPixel);
        int px = (int) floor((pos.x   - gm.x0) / gm.metersPerPixel);
        int py = (int) floor((pos.y   - gm.y0) / gm.metersPerPixel);
        if (tx < 0 || ty < 0 || px < 0 || py < 0 ||
            tx >= gm.width || ty >= gm.height || px >= gm.width || py >= gm.height) {
            printf("WRN: Wavefront cannot compute when target or pose land outside of map.");
            return false;
        }

        // Wavefront data structures
		*wavemap = new double[gm.width*gm.height];
        IntMaxHeap wavefront ;

        // Initialize wavefront and wavemap costs
        for (int y=0; y < gm.height; y++)
        {
            for (int x=0; x < gm.width; x++)
            {
                double cost = ((int) gm.data[y*gm.width+x]) & 0xFF;
                if (cost > maxCost)
                   // wavemap[y*gm.width+x] = Double_MAX_VALUE;
				   *((*wavemap)+y*gm.width+x)= Double_MAX_VALUE;
                else
                    *((*wavemap)+y*gm.width+x) = -1;
            }
        }

        // get cost for goal
        // min cost in 1
        *((*wavemap)+ty*gm.width + tx) = max((double)(((int) gm.data[ty*gm.width + tx]) & 0xFF), minCellCost);
        int c = c = ((tx & 0xFFFF) << 16) | (ty & 0xFFFF);
        wavefront.add(c, -*((*wavemap)+ty*gm.width+tx));

        // compute wavefront
        int step = 0;

   
	bool bFindPath = false;
        while (wavefront.size() > 0)
        {

            IntHeapPair ihp;
			wavefront.removeMaxPair(ihp);
            int nx = (ihp.o >> 16) & 0xFFFF;
            int ny =  ihp.o        & 0xFFFF;
            int n = ny*gm.width + nx;

            for (int i=0;i<8;i++)
            {
				int neighbor[2];
				neighbor[0] = *(*(neighbors+i)+0);
			    neighbor[1] = *(*(neighbors+i)+1);
                int npx = nx + neighbor[0];
                int npy = ny + neighbor[1];
                int np = npy*gm.width+npx;

                // skip if out of bounds
                if (npx >= gm.width  || npx < 0 ||
                    npy >= gm.height || npy < 0)
                    continue;

                // was it already set?
                if (*((*wavemap)+np) > 0)
                    continue;

                // Get costs for n and n'
                // note that we already ensured that we are not dealing
                // with infinite costs when we initialized some nodes in
                // the wavemap to Double.MAX_VALUE, above
                double nCost  = max(minCellCost, (double)(((int) gm.data[n])  & 0xFF));
                double npCost = max(minCellCost, (double)(((int) gm.data[np]) & 0xFF));

                // Clearly we have 1) a valid node which 2) has not been
                // assigned a wavefront cost and  does not exceed the max
                // cost threshold for its terrain cost.  We must now compute
                // the wavefront value

                // Transition cost is distance between cells (due to diagonals)
                double transition = LinAlg::magnitude(neighbor,2);
                // newval is the minimum possible cost for this node because
                // we are only expanding from the min heap
                double newval = *((*wavemap)+n) + transition*(npCost + nCost)/2.0;
                *((*wavemap)+np) = newval;
                c = ((npx & 0xFFFF) << 16) | (npy & 0xFFFF);

                // store the negative wavefront cost because we're implementing
                // a min heap with a max heap class
                wavefront.add(c, -newval);

                // Stop upon reaching the robot?
                if (steps < 1 && npx == px && npy == py)
		{
		    bFindPath = true;
                    break ;
		}
            }

            // Stop if we've exceeded the number of steps requested
	    if(bFindPath)
		break;
            if (steps >= 1 && step++ >= steps)
                break ;
        }

	/*	for(int j=0;j<gm.width*gm.height;j++)
		{

		 _RPT1(_CRT_WARN,"%f\n", *((*wavemap)+j));

		}*/

        return true;
    }

