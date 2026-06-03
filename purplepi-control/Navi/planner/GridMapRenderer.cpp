//#include "StdAfx.h"
#include "GridMapRenderer.h"


 double GridMapRenderer::MAX_VIS = 0.0; // How far can we see in 90deg cone in front of robot?
GridMapRenderer::GridMapRenderer(void)
{
}


GridMapRenderer::~GridMapRenderer(void)
{
}



bool GridMapRenderer::makeVisibilityMap(GridMap &visibilityMap, double xyt[], double maxObservation)
    {

        // Expand the obstacles by .1m  to prevent erroneous visible area near walls, or missing data
        // binaryMap = binaryMap.dilate((byte)255, dilate);
		GridMap  occuMap;
    	 //occuMap.copy(visibilityMap);
		visibilityMap.copy(occuMap);

        // compute step size so that our error is 1 pixel over the whole map.
        double min_dtheta = atan2(1.0, max(visibilityMap.width, visibilityMap.height) );

		
        int sz = 256;
        while (1.0 / sz > min_dtheta)
            sz *= 2;

        // the maximum distance we can see at every theta position
		int length = sz;
        double *minsqrange = NULL;
		minsqrange= new double[length]; // size must be power of two
        double dtheta = PI / length;

        // Initialize the range to infty
        for (int i = 0; i < length; i++)
            minsqrange[i] = maxObservation*maxObservation;

        // handle robot's FOV by setting some minsqranges to zero where we can't see.
        if (true) { /// Unnecessary b/c infty will get replaced with zero in this region anyway
            double t0 = xyt[2] + MathUtil::toRadians(135);
            double t1 = xyt[2] + MathUtil::toRadians(225);
            int i0 = (int) ((t0*length) / (2*PI) + .5);
            int i1 = (int) ((t1*length) / (2*PI) + .5);
            for (int i = i0; i <= i1; i++) {
                int j = i & (length - 1);
                minsqrange[j] = 0;
            }
        }
        
        int maxY = (int)((xyt[1]+maxObservation-visibilityMap.y0)/visibilityMap.metersPerPixel);
        maxY = maxY<visibilityMap.height?maxY:visibilityMap.height;
        int minY = (int)((xyt[1]-maxObservation-visibilityMap.y0)/visibilityMap.metersPerPixel);
        minY = minY>0?minY:0;
        int maxX = (int)((xyt[0]+maxObservation-visibilityMap.x0)/visibilityMap.metersPerPixel);
        maxX = maxX<visibilityMap.width?maxX:visibilityMap.width;
        int minX = (int)((xyt[0]-maxObservation-visibilityMap.x0)/visibilityMap.metersPerPixel);
        minX = minX>0?minX:0;

        // phase 1: collect visibility data for every angle around the robot
        for (int y = minY; y < maxY; y++) {
            for (int x = minX; x < maxX; x++) {
                int idx = y*visibilityMap.width + x;

                // skip over empty space
                if (occuMap.data[idx] == (BYTE)255)
                {
                	 // location of pixel relative to robot
                    double dx = visibilityMap.x0 + x * visibilityMap.metersPerPixel - xyt[0];
                    double dy = visibilityMap.y0 + y * visibilityMap.metersPerPixel - xyt[1];

                    // we have an occupied pixel
                    // direction to the center of the pixel
                    double theta0 = atan2(dy, dx);
                    double sqrange = dx*dx + dy*dy;
                    double thetaWidth = atan2(2*visibilityMap.metersPerPixel, sqrange);

                    // fill in an arc around this pixel, marking farther distances as unobservable.
                    int i0 = (int) ((theta0 - thetaWidth / 2)*length / (2*PI) + .5);
                    int i1 = (int) ((theta0 + thetaWidth / 2)*length / (2*PI) + .5);

                    for (int i = i0; i <= i1; i++) {
                        int j = i & (length - 1);
                        minsqrange[j] = min(minsqrange[j], sqrange);
                    }
                }
            }
        }

        // phase 1.5 All infty ranges need to be changed. If within 90deg infront of robot, infty -> 10**2, else infty ->0
        if (true) {
            // all infty within the back 270 degrees need to be set to zero

            int i0 = (int)(((xyt[2]*length) / (2*PI) + .5)) & (length - 1);
            int half_front = (int) ((MathUtil::toRadians(45) * length) / (2*PI) + .5);

             double maxVisibilitySqRange = maxObservation * maxObservation; // 10 meters
            for (int i =0; i < length; i++) {
                if (minsqrange[i] == Double_MAX_VALUE) 
				{
                    int e = ((i-i0)&(length-1));
                    if (e < half_front || e+half_front > length) {
                       // minsqrange[i] = maxVisibilitySqRange;
                    } else {
                        minsqrange[i] = 0;
                    }
                }
            }
        }
        
        // phase 2: test every pixel for visibility
        if(false){
        int rad = (int)(MathUtil::mod2pi(xyt[0]+MathUtil::toRadians(-135)+PI)/dtheta);
        int size = (int)(MathUtil::toRadians(270)/dtheta);
        for(int i=0;i<size;i++) {
        	int index = (rad+i)%length;
        	double dis = sqrt(minsqrange[index]);
        	double angle = (index)*dtheta;
        	visibilityMap.drawLine(xyt[0], xyt[1], xyt[0]+dis*cos(angle), xyt[1]+dis*sin(angle), (BYTE)254);
        }}
        
        if(true)
        for (int y = minY; y < maxY; y++) {
            for (int x = minX; x < maxX; x++) {

                int idx = y*visibilityMap.width + x;
                if(visibilityMap.data[idx]==(BYTE)254 || visibilityMap.data[idx] == (BYTE)255)continue;
                double dx = visibilityMap.x0 + x * visibilityMap.metersPerPixel - xyt[0];
                double dy = visibilityMap.y0 + y * visibilityMap.metersPerPixel - xyt[1];

                // we have an occupied pixel
                // direction to the center of the pixel
                double theta0 = atan2(dy, dx);
                double sqrange = dx*dx + dy*dy;

                double thetaWidth = atan2(2*visibilityMap.metersPerPixel, sqrange); // * 2;Fudge

                if (true) {
                    //int i = (int) (theta0*minsqrange.length/(2*Math.PI)+.5) & (minsqrange.length - 1);
                    int i0 = (int) ((theta0 - thetaWidth / 2)*length / (2*PI) + .5)& (length - 1);
                    int i1 = (int) ((theta0 + thetaWidth / 2)*length / (2*PI) + .5)& (length - 1);
                    bool notAdd = false;
                    for(int i=i0;i<=i1;i++)
                    	if(minsqrange[i] < sqrange)
                    	{
                    		notAdd = true;
                    		break;
                    	}
                    if (!notAdd)
                        visibilityMap.data[idx] = (BYTE)254;
                    

                }

                if (false) {
                    // make sure we are under the range for all the thetas in this range
                    int i0 = (int) ((theta0 - thetaWidth / 2)*length / (2*PI) + .5);
                    int i1 = (int) ((theta0 + thetaWidth / 2)*length / (2*PI) + .5);

                    // Assume to be clear
                    visibilityMap.data[idx] = (BYTE) 255;

                    for (int i = i0; i <= i1; i++) {
                        int j = i & (length - 1);
                        if (minsqrange[j] < sqrange) // if this pixel is beyond the range, we can't mark as visible
                            visibilityMap.data[idx] = 0;
                    }
                }
            }
        }

        if (false) { // draw oversized explored area around robot, before min convolution
            visibilityMap.drawCircle(xyt[0], xyt[1], .45, (BYTE)255);
        }

        // if (true) { // draw explored area around robot, after min convolution
        //     visibilityMap.drawCircle(xyt[0], xyt[1], .4, (byte)255);
        // }
        //visibilityMap = GridMapRenderer.removeIsolatedIslands(visibilityMap, new double[3], 16, 0.0);
    	
		if(minsqrange!=NULL)
		delete [] minsqrange;

        return true;
    }