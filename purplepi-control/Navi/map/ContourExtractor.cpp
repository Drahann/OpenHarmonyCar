#include "ContourExtractor.h"

#define min1(a,b)            (((a) < (b)) ? (a) : (b))

bool compare(const Join &a, const Join &b)
{
	return a.distance < b.distance;

}

ContourExtractor::ContourExtractor(void)
{
	maxSkipPoints = 3;
    adjacentAcceptDistance = 0.1;
	maxDistance = 5;
    startContourMaxDistance = 0.5;
	minPointsPerContour = 0;
	maxDistanceRatio = 2.2; // just big enough to allow for a missed return.
    alwaysAcceptDistance = 0.15;
/*

	 maxSkipPoints = config.getInt("max_skip_points", maxSkipPoints);
        adjacentAcceptDistance = config.getDouble("adjacent_accept_distance", adjacentAcceptDistance);
        maxDistance = config.getDouble("max_distance", maxDistance);
        startContourMaxDistance = config.getDouble("start_contour_max_distance", startContourMaxDistance);
        minPointsPerContour = config.getInt("min_points_per_contour", minPointsPerContour);
        maxDistanceRatio = config.getDouble("max_distance_ratio", maxDistanceRatio);
        alwaysAcceptDistance = config.getDouble("always_accept_distance", alwaysAcceptDistance);*/
}


ContourExtractor::~ContourExtractor(void)
{
}

            //points是激光点在世界坐标系中的位置
void ContourExtractor::getContours(vector<Pose> &points, vector<vector<vector<double> > > &contours)
{

        
        // Compute all candidate joins.
        
        //找到并存储所有满足距离要求的点对
        vector<Join> joins;   //存放join class join:int a,b; 
                              //                    double distance
        for (int i = 0; i < points.size(); i++) //将激光点都轮一圈
		{   
		    //每个激光点与它后面的4个点
            for (int j = i + 1; j < min1(points.size(), i + maxSkipPoints + 2); j++) //在maxSkipPoints=3范围内与第i个点比较，属于临近点
			{
				Pose p1 = points.at(i);
				//p1.x = points.at(i).at(0);
				//p1.y = points.at(i).at(1);

				Pose p2 = points.at(j);
				//p2.x = points.at(j).at(0);
				//p2.y = points.at(j).at(1);

                double distance = LinAlg::DistancePose(p1, p2);//两点距离

                // If the points are adjacent(临近) and their distance is
                // less than a threshold（临界值）, always accept them. This is
                // to help us keep together points that might
                // otherwise be separated due to the noise of the
                // sensor（传感器）.
                if (i+1 == j && distance < adjacentAcceptDistance)//如果是相邻点，并且满足距离要求，0.1
                    distance = -1;
				
                //认为是连续的点
                if (distance < maxDistance) //如果距离满足要求，5
				{
                    Join join ;
                    join.a = i;     //a,b存储点的序数和这两点间的距离
                    join.b = j;
                    join.distance = distance;
                    joins.push_back(join);
                }
            }
        }

        /////////////////////////////////////////////////////////////////////////
        // Sort joins in order of least cost to maximum cost.
        //join是在一定范围内的两个点之间的距离和序号
		sort(joins.begin(),joins.end(),compare );  //将join按照存储的两点间距离升序排列

        /////////////////////////////////////////////////////////////////////////
        // Perform joins

        // Who is the left/right neighbor of each point?  If no neighbor, -1.

		//找到每个点左右满足距离要求的最近的点（如果有的话，没有就为-1），将左右点序号信息存储到两个数组中（没有就置为-1）
        int *left = new int[points.size()];   
        int *right = new int[points.size()];

        for (int i = 0; i < points.size(); i++) {  //将两个数组初始化为-1，数组长度的和激光点个数相同
            left[i] = -1;
            right[i] = -1;
        }

        for (int joinidx = 0; joinidx < joins.size(); joinidx++)   //将升序排列好的join数组，从头到尾循环一遍，每一个join都存储了两个激光点的序数a，b和间距
		{
			//?????
            Join join = joins.at(joinidx);  //挨个取出来

            // Can't join two points that already have different neighbors.
            if (right[join.a] >=0 || left[join.b] >= 0)  //第一次进来肯定不执行，都初始化为-1了
                continue;

            // If the left or right point is already part of a
            // contour, what is the distance between the most recently
            // added point?  If the distance jumps suddenly, we may
            // not want to connect to this contour.
            double lastDistance = Double_MAX_VALUE;  //999999999999.9999
            if (left[join.a] >= 0)
			{
				Pose p1 =  points.at(join.a);
				//p1.x = points.at(join.a).at(0);
				//p1.y = points.at(join.a).at(1);

				Pose p2= points.at(left[join.a]);
				//p2.x = points.at(left[join.a]).at(0);
				//p2.y = points.at(left[join.a]).at(1);

                lastDistance = min1(lastDistance, LinAlg::DistancePose(p1,p2));

			}

            if (right[join.b] >= 0)
			{
				Pose p1 =points.at(join.b);
				//p1.x = points.at(join.b).at(0);
				//p1.y = points.at(join.b).at(1);

				Pose p2= points.at(right[join.b]);
				//p2.x = points.at(right[join.b]).at(0);
				//p2.y = points.at(right[join.b]).at(1);

                lastDistance = min1(lastDistance, LinAlg::DistancePose(p1,p2));

			}

            if (lastDistance == Double_MAX_VALUE)
                lastDistance = startContourMaxDistance;//0.5

            double distanceRatio = join.distance / lastDistance;//距离比例

			//如果两个点的距离过大，就认为不是一个轮廓上的点
            if (distanceRatio > maxDistanceRatio && join.distance > alwaysAcceptDistance)//maxDistanceRatio = 2.2;alwaysAcceptDistance = 0.15;
                continue;

            // join the points.
            right[join.a] = join.b;
            left[join.b] = join.a;
        }

        /////////////////////////////////////////////////////////////////////////
        // Construct the joins.

       
        for (int root = 0; root < points.size(); root++) {
            // is this isn't left-most point in a chain, we've already extracted this contour.
            if (left[root] >=0)
                continue;     

            vector<vector<double> > contour ;

			//如果是一个连续的轮廓，会存储很长，断开的位置是right为-1，断开的判断标准是距离比例过大并且距离超过15cm
            for (int child = root; child >= 0; child = right[child])  //如果是离散的单个的点呢，为什么要存储它？有什么用？
			{
				vector<double> temp;
				temp.push_back(points.at(child).x);
				temp.push_back(points.at(child).y);
				temp.push_back(points.at(child).theta);
                contour.push_back(temp);


			}

            if (contour.size() >= minPointsPerContour)  //0，为什么是0呢，具体连续多少个？
                contours.push_back(contour);
        }

		delete [] left;
		delete [] right;

       
    }

	void ContourExtractor::getContours_Pose(vector<Pose> &points, vector<vector<Pose> > &contours)
	{
	        //vector<vector<vector<double> > > tmp_contours;
			
			// Compute all candidate joins.
			
			//找到并存储所有满足距离要求的点对
			vector<Join> joins;   //存放join class join:int a,b; 
								  //					double distance
			for (int i = 0; i < points.size(); i++) //将激光点都轮一圈
			{	
				//每个激光点与它后面的4个点
				for (int j = i + 1; j < min1(points.size(), i + maxSkipPoints + 2); j++) //在maxSkipPoints=3范围内与第i个点比较，属于临近点
				{
					Pose p1 = points.at(i);
					//p1.x = points.at(i).at(0);
					//p1.y = points.at(i).at(1);
	
					Pose p2 = points.at(j);
					//p2.x = points.at(j).at(0);
					//p2.y = points.at(j).at(1);
	
					double distance = LinAlg::DistancePose(p1, p2);//两点距离
	
					// If the points are adjacent(临近) and their distance is
					// less than a threshold（临界值）, always accept them. This is
					// to help us keep together points that might
					// otherwise be separated due to the noise of the
					// sensor（传感器）.
					if (i+1 == j && distance < adjacentAcceptDistance)//如果是相邻点，并且满足距离要求，0.1
						distance = -1;
					
					//认为是连续的点
					//if (distance < maxDistance) //如果距离满足要求，5
					if (distance < 3.0)
					{
						Join join ;
						join.a = i; 	//a,b存储点的序数和这两点间的距离
						join.b = j;
						join.distance = distance;
						joins.push_back(join);
					}
				}
			}
	
			/////////////////////////////////////////////////////////////////////////
			// Sort joins in order of least cost to maximum cost.
			//join是在一定范围内的两个点之间的距离和序号
			sort(joins.begin(),joins.end(),compare );  //将join按照存储的两点间距离升序排列
	
			/////////////////////////////////////////////////////////////////////////
			// Perform joins
	
			// Who is the left/right neighbor of each point?  If no neighbor, -1.
	
			//找到每个点左右满足距离要求的最近的点（如果有的话，没有就为-1），将左右点序号信息存储到两个数组中（没有就置为-1）
			int *left = new int[points.size()];   
			int *right = new int[points.size()];
			
	        /*将两个数组初始化为-1，数组长度的和激光点个数相同*/
			for (int i = 0; i < points.size(); i++) 
			{  
				left[i] = -1;
				right[i] = -1;
			}
	
			for (int joinidx = 0; joinidx < joins.size(); joinidx++)   //将升序排列好的join数组，从头到尾循环一遍，每一个join都存储了两个激光点的序数a，b和间距
			{
				//?????
				Join join = joins.at(joinidx);	//挨个取出来
	
				// Can't join two points that already have different neighbors.
				if (right[join.a] >=0 || left[join.b] >= 0)  //第一次进来肯定不执行，都初始化为-1了
					continue;
	
				// If the left or right point is already part of a
				// contour, what is the distance between the most recently
				// added point?  If the distance jumps suddenly, we may
				// not want to connect to this contour.
				double lastDistance = Double_MAX_VALUE;  //999999999999.9999
				if (left[join.a] >= 0)
				{
					Pose p1 =  points.at(join.a);
					//p1.x = points.at(join.a).at(0);
					//p1.y = points.at(join.a).at(1);
	
					Pose p2= points.at(left[join.a]);
					//p2.x = points.at(left[join.a]).at(0);
					//p2.y = points.at(left[join.a]).at(1);
	
					lastDistance = min1(lastDistance, LinAlg::DistancePose(p1,p2));
	
				}
	
				if (right[join.b] >= 0)
				{
					Pose p1 =points.at(join.b);
					//p1.x = points.at(join.b).at(0);
					//p1.y = points.at(join.b).at(1);
	
					Pose p2= points.at(right[join.b]);
					//p2.x = points.at(right[join.b]).at(0);
					//p2.y = points.at(right[join.b]).at(1);
	
					lastDistance = min1(lastDistance, LinAlg::DistancePose(p1,p2));
	
				}
	
				if (lastDistance == Double_MAX_VALUE)
					lastDistance = startContourMaxDistance;//0.5
	
				double distanceRatio = join.distance / lastDistance;//距离比例
	
				//如果两个点的距离过大，就认为不是一个轮廓上的点
				if (distanceRatio > maxDistanceRatio && join.distance > alwaysAcceptDistance)//maxDistanceRatio = 2.2;alwaysAcceptDistance = 0.15;
					continue;
	
				// join the points.
				right[join.a] = join.b;
				left[join.b] = join.a;
			}
	
			/////////////////////////////////////////////////////////////////////////
			// Construct the joins.
	
		   
			for (int root = 0; root < points.size(); root++) 
			{
				if (left[root] >=0)
					continue;	  
	
				vector<Pose > contour ;
			
	
				//如果是一个连续的轮廓，会存储很长，断开的位置是right为-1，断开的判断标准是距离比例过大并且距离超过15cm
				for (int child = root; child >= 0; child = right[child])  //如果是离散的单个的点呢，为什么要存储它？有什么用？
				{
					Pose temp;
					temp.x = points.at(child).x;
					temp.y = points.at(child).y;
					temp.theta = points.at(child).theta;
					contour.push_back(temp);
	
	
				}
	
				if (contour.size() >= minPointsPerContour)	//0，为什么是0呢，具体连续多少个？
					contours.push_back(contour);
			}
	
			delete [] left;
			delete [] right;
	
		   
		}



