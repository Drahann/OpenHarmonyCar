#pragma once

#include "../type.h"

#include <algorithm>
#include <vector>
#include <math.h>
#include "../math/LinAlg.h"

using namespace std;

class Join //implements Comparable<Join>
{
public:
        int    a, b; // which points might we join?
        double distance;

    /*    inline int compareTo(Join j)
        {
			if(distance > j.distance)
				return 1;
			else if (distance == j.distance)
			   return 0;
			else
			   return -1;
          
        }*/
};
bool compare(const Join &a, const Join &b);

class ContourExtractor
{
public:
	ContourExtractor(void);
	~ContourExtractor(void);

	int maxSkipPoints ;

    /** If two points are adjacent (relative to scan order), and they
        are really close together, join them even if another point is
        closer.
    **/
    double adjacentAcceptDistance ;

    /** When adding a point to a contour, it's never okay to add a
     * point farther than this away.
     **/
    double maxDistance ;

    /** When starting a new contour, pretend that the last two points
     * were this far apart. This affects maxDistanceRatio, and
     * effectively limits our willingness to create contours that
     * contain only sparsely-connected points. **/
    double startContourMaxDistance ;

   int minPointsPerContour ;

   double maxDistanceRatio ; // just big enough to allow for a missed return.

    double alwaysAcceptDistance ;

	void getContours(vector<Pose> &points, vector<vector<vector<double> > > &contours);
	void getContours_Pose(vector<Pose> &points, vector<vector<Pose> > &contours);
};

