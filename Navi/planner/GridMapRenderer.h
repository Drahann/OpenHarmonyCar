#pragma once

#include "../map/Gridmap.h"
#include <math.h>
#include "../math/MathUtil.h"

class GridMapRenderer
{
	static double MAX_VIS ;
public:
	GridMapRenderer(void);
	~GridMapRenderer(void);


	static bool makeVisibilityMap(GridMap &visibilityMap, double xyt[], double maxObservation);
};

