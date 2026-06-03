#pragma once
#include "../map/Gridmap.h"
#include <assert.h>
#include "../type.h"
#include <math.h>
#include <vector>
#include "IntMaxHeap.h"
#include "../math/LinAlg.h"

using namespace std;

class Wavefront
{
	Wavefront(void);
	~Wavefront(void);

	
    // Neighboring cells to consider
public:

	static int NEIGHBORS[8][2];

    /** Calculate wavefront map from cost map.
      * Does not terminate until reaching pose (if possible)
      * Uses 8-connected neighbors
      *
      * @param gm          - GridMap on which to plan
      * @param pose        - Robot pose.  Required to be within gridmap
      * @param target      - Goal for wavefront.  Also must be in gridmap range
      * @param minCellCost - Minimum cost for a cell (enforced: must exceed zero)
      * @param maxCost     - maximum cost used to compute driveable terrain.
      **/
    static bool getWavefront(GridMap &gm, Pose &pos, Pose &target,
                                        double minCellCost, int maxCost,double **wavemap);

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
   static bool getWavefront(GridMap &gm, Pose &pos, Pose &target,
                                        double minCellCost, int maxCost, int steps,double **wavemap);

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
	static bool   getWavefront(GridMap &gm, Pose &pos, Pose &target,
                                        double minCellCost, int maxCost,
                                        int steps, int neighbors[8][2],double **wavemap);
	};