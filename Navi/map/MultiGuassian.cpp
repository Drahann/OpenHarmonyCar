#include "MultiGuassian.h"


MultiGuassian::MultiGuassian(void)
{
}


MultiGuassian::~MultiGuassian(void)
{
}

    int MultiGuassian::bestIndices(vector<IntArray2D*> &vtScores,int *bestidx)
    {
        double bestscore = -999999999999.0;   //double.max
       

        for (int tidx = 0; tidx < vtScores.size(); tidx++) {
            for (int yidx = 0; yidx < vtScores.at(tidx)->m_dim1; yidx++) {
                for (int xidx = 0; xidx < vtScores.at(tidx)->m_dim2; xidx++) {
                    int score = vtScores.at(tidx)->get(yidx, xidx); 
                    if (score > bestscore) {
                        bestscore = score;
                        bestidx[0] = xidx;
                        bestidx[1] = yidx;
                        bestidx[2] = tidx;
                    }
                }
            }
        }

        return 1;
    }

int MultiGuassian::bestIndices(IntArray2D* pScores,int *bestidx)
    {
        double bestscore = -999999999999.0;   //double.max
       

       
            for (int yidx = 0; yidx < pScores->m_dim1; yidx++) {
                for (int xidx = 0; xidx < pScores->m_dim2; xidx++) {
                    int score = pScores->get(yidx, xidx); 
                    if (score > bestscore) {
                        bestscore = score;
                        bestidx[0] = xidx;
                        bestidx[1] = yidx;
                    
                    }
                }
            }
        

        return 1;
    }


    void MultiGuassian::setModel(GridMap &tmp)
    {
        gm = gm;
        if (decimate <= 1) {
            dgm = gm;
        } else {
            dgm = gm.decimateMax(decimate);
            dgm = dgm.max4(); // necessary to avoid quantization problems.
        }
    }

void MultiGuassian::matchRaw(vector<Pose> &points,  Pose& posepriorxyt, double **pinv,
                           double xrange, double yrange, double thetaRange, double thetaResolution,Pose &resPose)
{

		int	   max_search_iterations = 100;
		int    decimate =1;
		double metersPerPixel = m_GridsizeQH;
        double lowResXYT[3];
		double priorxyt[3];

		priorxyt[0] = posepriorxyt.x;
		priorxyt[1] = posepriorxyt.y;
		priorxyt[2] = posepriorxyt.theta;

		lowResXYT[0] =  priorxyt[0] - xrange;
        lowResXYT[1] =  priorxyt[1] - yrange;
        lowResXYT[2] =  priorxyt[2] - thetaRange ;

		vector<IntArray2D*> vtlowResScores;
        scores3D(points,
                lowResXYT[0], (int) (2*xrange/metersPerPixel + 1),
                lowResXYT[1], (int) (2*yrange/metersPerPixel + 1),
                lowResXYT[2], thetaResolution,
                max(1, (int) (2*thetaRange/thetaResolution)),
                posepriorxyt, pinv,vtlowResScores);


        /////////////////////////////////////////////////////////////////
        // Step 1. Compute the covariance by fitting a Gauassian to the
        // low-resolution samples.
       // MultiGaussianEstimator mge = new MultiGaussianEstimator(3);

        /////////////////////////////////////////////////////////////////
        // If no decimation has been requested, return a result now.
        if (decimate <= 1) {
            int bestidx0[3];
			bestIndices(vtlowResScores,bestidx0);
            double xyt0score = vtlowResScores.at(bestidx0[2])->get(bestidx0[1],bestidx0[0]);

            double u[3] ;
			u[0] = lowResXYT[0] + bestidx0[0]*metersPerPixel;
			u[1] = lowResXYT[1] + bestidx0[1]*metersPerPixel;
            u[2] = lowResXYT[2] + bestidx0[2]*thetaResolution ;


			resPose.x = u[0];
			resPose.y = u[1];
			resPose.theta = u[2];
			return ;

            // Heuristic: posterior's mean is the MLE, posterior's
            // covariance is the fit covariance.
           // MultiGaussian mg = mge.getEstimate();
           // return new MultiGaussian(mg.getCovariance(), u);
        }

        /////////////////////////////////////////////////////////////////
        // Step 3. Search promising low-resolution voxels at high resolution.
        // Consider the peaks at low resolution in decreasing order. For each, compute
        // the corresponding cell at high resolution.
        int bestLowResIdx[3] ;
        int bestHighResScore = -1;
        double bestHighResXYT[3] ;

        for (int iters = 0; true; iters++) 
        {

           // if (debug) {
           //     if ((iters >= 100 && (iters & (iters - 1))==0) || iters == max_search_iterations)
           //         _RPT1(_CRT_WARN,"WRN: MultiResolutionScanMatcher: many iterations (%d)\n", iters);
           // }

            // Find the next best score that's less than maxscore
            // This implementation just researches the entire low
            // resolution volume. We could do better with a MaxHeap
            // type data structure, but emperically, this does not
            // appear to take a significant amount of time.
            int thisBestLowResScore = -1;

			for (int tidx = 0; tidx < vtlowResScores.size(); tidx++) 
            {
				for (int yidx = 0; yidx < vtlowResScores.at(tidx)->m_dim1; yidx++)
                {
                    for (int xidx = 0; xidx < vtlowResScores.at(tidx)->m_dim2; xidx++) 
                    {
                        int score =  vtlowResScores.at(tidx)->get(yidx, xidx);
                        if (score > thisBestLowResScore) {
                            thisBestLowResScore = score;
                            bestLowResIdx[0] = xidx;
                            bestLowResIdx[1] = yidx;
                            bestLowResIdx[2] = tidx;
                        }
                    }
                }
            }

            if (iters > max_search_iterations || bestHighResScore >= thisBestLowResScore) {
                // we're done: we have not found another low
                // resolution voxel that needs to be searched. Thus,
                // we return a result now.
 //               MultiGaussian mg = mge.getEstimate();
 //               return new MultiGaussian(mg.getCovariance(), bestHighResXYT);

				resPose.x = bestHighResXYT[0];
				resPose.y = bestHighResXYT[1];
				resPose.theta = bestHighResXYT[2];
				return ;


            }

			vtlowResScores.at(bestLowResIdx[2])->set(bestLowResIdx[1], bestLowResIdx[0], - thisBestLowResScore);
           // lowResScores[bestLowResIdx[2]].set(bestLowResIdx[1], bestLowResIdx[0], - thisBestLowResScore);

            // evaluate this grid at high resolution.
            // xyt1 is the lower-left corner of the cell that contained the maximum.
			double xyt1[3];
            xyt1[0] = lowResXYT[0] + bestLowResIdx[0]*metersPerPixel;
            xyt1[1] = lowResXYT[1] + bestLowResIdx[1]*metersPerPixel;
            xyt1[2] = lowResXYT[2] + bestLowResIdx[2]*thetaResolution ;

			IntArray2D* highResscores = NULL;
            highResscores = scores2D(points,  xyt1[0], decimate,
                                              xyt1[1], decimate,
                                              xyt1[2],
                                              posepriorxyt, pinv);

            int thisBestHighResIdx[2] ;
			bestIndices(highResscores,thisBestHighResIdx);
            int thisBestHighResScore = highResscores->get(thisBestHighResIdx[1], thisBestHighResIdx[0]);

            if (thisBestHighResScore > bestHighResScore) {
                bestHighResScore = thisBestHighResScore;
                bestHighResXYT[0] = xyt1[0] + thisBestHighResIdx[0]*metersPerPixel;
                bestHighResXYT[1] = xyt1[1] + thisBestHighResIdx[1]*metersPerPixel;
                bestHighResXYT[2] = xyt1[2];
            }  
			if(highResscores!=NULL)
			{
				delete [] highResscores;
			}

          /*  if (thisBestHighResScore > thisBestLowResScore && debug) {
                // TODO: Investigate cases where this
                // happens. Numerical precision problems?
                System.out.printf("DEBUG: MultiResolutionScanMatcher %10d %10d %10d [%5d %5d %5d] [%5d %5d]\n",
                                  thisBestLowResScore, thisBestHighResScore,
                                  thisBestHighResScore - thisBestLowResScore,
                                  bestLowResIdx[0], bestLowResIdx[1], bestLowResIdx[2],
                                  thisBestHighResIdx[0], thisBestHighResIdx[1]);
          }  */
        }

		if(vtlowResScores.size()>0)
		{
			for (int i =0;i<vtlowResScores.size();i++)
			{
				IntArray2D* pArray = vtlowResScores.at(i);
				if (pArray!=NULL)
				{
					delete [] pArray;
				}
			}

		}
    




}