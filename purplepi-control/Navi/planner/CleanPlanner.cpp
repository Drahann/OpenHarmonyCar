#include "CleanPlanner.h"
#include <fstream>
#include "ant.h"



CleanPlanner::CleanPlanner(void)
{
	LINEDIS = 6;
}

void CleanPlanner::setConfig(plannerconfig config,double robot_width, double robot_radius, unsigned int cleangridwidth)
{
	
	PathPlanner::setConfig(config,robot_width, robot_radius);
	LINEDIS = cleangridwidth;
}

CleanPlanner::~CleanPlanner(void)
{
}
	void CleanPlanner::getBoundingBox(GridMap &map,MyRect &bounding){
    	
	bounding.x0 = Integer_MAX_VALUE;
	bounding.y0 = Integer_MAX_VALUE;
	bounding.x1 = Integer_MIN_VALUE;
	bounding.y1 = Integer_MIN_VALUE;
    	for (int y = 0; y < map.height; y++) {
            for (int x = 0; x < map.width; x++) {
            	if (map.data[y*map.width+x] == (unsigned char)(-1) ){
            		if (x<bounding.x0)bounding.x0 = x;//x0
            		if (x>bounding.x1)bounding.x1 = x;//x1
            		if (y<bounding.y0)bounding.y0 = y;//y0
            		if (y>bounding.y1)bounding.y1 = y;//y1
            	}
            }
    	}
    	return ;
    }

	/*<FUNC+>**********************************************************************/
/* 函数名称: IfvaildPoint                                                         */
/* 功能描述: 判断地图中某点周围有障碍物存在，若不存在则虚拟线没有闭合，认为为非法区域线 */
/*<FH->************************************************************************/
int CleanPlanner::IfvaildPoint(GridMap &tmpmap,int x0,int y0,int data,int size)
{
	int tmpdata;
	int width = tmpmap.width;
	for(int i = -size;i<=size;i++)
	{
		for(int j = -size;j <= size;j++)
		{
			tmpdata = tmpmap.data[(y0 + i) * width + (x0+j )];
			if(tmpdata == -1 || tmpdata < data)
			{
				return 0;
			}
		}
	}
	return -1;
}
/*<FUNC+>********************************************************************************/
/* 函数名称: findnextpoint                                                              */
/* 功能描述: 递归寻找所有某个特殊值的点，直到找到最后一个，并将这些点连成一条直线       */
/*<FH->**********************************************************************************/
int CleanPlanner::findnextpoint(GridMap &tmpmap,int x0,int y0,int x,int y,int data,int &nextpoint)
{
	int tmpdata;
	int width = tmpmap.width;
	int ret = 0;
	vector<int> oncepoint;
	nextpoint = y * width + x;

	for(int i = -1;i<=1;i++)
	{
		for(int j = -1;j <= 1;j++)
		{
			tmpdata = tmpmap.data[(y + i) * width + (x+j )];
			if(tmpdata == data && (!(i == 0 && j == 0)))
			{
				ret = 1;
				nextpoint = (y + i) * width + (x + j);
				tmpmap.data[(y + i) * width + (x + j )] = 0;
				if(-1 != findnextpoint(tmpmap,x0,y0,x+j,y+i,data,nextpoint))
				{
					oncepoint.push_back(nextpoint);				
				}
						//return 0;
			}
		}
	}
	double dis;
	double maxdis = 0.0;
	int tmpx,tmpy;
	for(int k = 0; k < oncepoint.size();k++)
	{
		tmpx = oncepoint[k] % width - x0;
		tmpy = oncepoint[k] / width - y0;
		dis = sqrt((double)(tmpx * tmpx + tmpy * tmpy));
		if(dis > maxdis)
		{
			maxdis = dis;
			nextpoint = oncepoint[k];
		}
	}
	if(ret == 0 || -1 == IfvaildPoint(tmpmap,x,y,data,1))
	{	
			return -1;
	}
	return 0;
}
/*<FUNC+>********************************************************************************/
/* 函数名称: getline                                                                    */
/* 功能描述: 根据该点获取一条直线，并获得该直线起点和终点的坐标值                       */
/*<FH->**********************************************************************************/

int  CleanPlanner::getline(GridMap &tmpmap,int x0,int y0 ,int data)
{
	int lineflag = 0;
	int ret;
	int width = tmpmap.width;
	int nextpoint = y0 * width + x0;
	int endx,endy;
	double dis;

	if(IfvaildPoint(tmpmap,x0,y0,data,2) == -1)
	{
		lineflag = -1;
	}
	else{
		 findnextpoint(tmpmap,x0,y0,x0,y0,data,nextpoint);
		
			endx = nextpoint % width;
			endy = nextpoint / width;
			dis = sqrt((double)((endx - x0) * (endx - x0) + (endy - y0) * (endy - y0)));
			if(x0 == endx && y0 == endy)
			{
				lineflag = -1;
			}

			else if(IfvaildPoint(tmpmap,endx,endy,data,2) == -1)
			{
				lineflag = -1;
			}
			else if(dis <= 6)  // less than  expanddis;
			{
				lineflag = -1;
			}
		
	}
	if(lineflag == 0)
	{
		Pose tmppos;
		tmppos.x = (double)x0 * tmpmap.metersPerPixel +  tmpmap.x0;
		tmppos.y = (double)y0 * tmpmap.metersPerPixel +  tmpmap.y0;
		linepos.push_back(tmppos);
		
		tmppos.x = (double)endx * tmpmap.metersPerPixel +  tmpmap.x0;
		tmppos.y = (double)endy * tmpmap.metersPerPixel +  tmpmap.y0;
		linepos.push_back(tmppos);
	}
	return 0;
}
/*<FUNC+>********************************************************************************/
/* 函数名称: getmapline                                                                  */
/* 功能描述: 初始化并在地图中找到划分区域线                                             */
/*<FH->**********************************************************************************/
bool  CleanPlanner::GetMapLine(GridMap &origmap)
{
	MyRect bounding ;
	int startindex = 7;
	int endindex = 5;
	int wigth = 0;
	GridMap tmpmap;
	origmap.copy(tmpmap);
	getBoundingBox(tmpmap,bounding);
	wigth = tmpmap.width;
	//wigth = origmap.width;
	if(linepos.size() > 0)    //INIT
	{
		linepos.clear();
		pathpos.clear();
		posvaild.clear();
	}
	for(int index = startindex;index >= endindex;index--)
	{
		for(int i = bounding.y0 + 1;i < (bounding.y1 - 1);i++)
		{
			for(int j = (bounding.x0 + 1);j < (bounding.x1-1);j++)
			{
				if(tmpmap.data[i * wigth + j] == index)
				{
					//printf("index = %d\n",index);
					getline(tmpmap,j,i,index);
				}
			}
		}
	}
/*
	if(tmpmap.data != NULL)
	{
		delete [] tmpmap.data;
		tmpmap.data = NULL;
	}
*/
	return true;
}
/*<FUNC+>********************************************************************************/
/* 函数名称: getmappath                                                                  */
/* 功能描述:  根据一条直线，提取该直线两侧各一个点                                        */
/*<FH->**********************************************************************************/
int CleanPlanner::getmappath(GridMap &origmap,Pose &p1,Pose &p2)
{
		
	double xa,xb,ya,yb,dist;
	double r = 0.5;
	int i;
	int nsteps;
	Pose tmppos;
	double line_k1 = 0.0;
	int vaildflag = 0;
	int radius = 2;
	int times;
	xa = p1.x;
	xb = p2.x;
	ya = p1.y;
	yb = p2.y;
	dist = sqrt((xa - xb) * (xa - xb) + (ya -yb) * (ya- yb));
	nsteps = (int) (dist / origmap.metersPerPixel + 1);

	for (i = 0; i < nsteps; i++)
	{
            double alpha = ((double) i)/nsteps;
            double x = xa*alpha + xb*(1-alpha);
            double y = ya*alpha + yb*(1-alpha);

            int ix = (int) ((x - origmap.x0) / origmap.metersPerPixel);
			int iy = (int) ((y - origmap.y0) / origmap.metersPerPixel);
            for(int j = iy - radius;j <= (iy + radius);j++)
			{
				for(int k = ix - radius;k <= (ix + radius);k++)
				{
				    origmap.data[j*origmap.width + k] = -1;
     
				}
			}
	}

	line_k1 = (p1.y - p2.y) / (p1.x - p2.x);

	for(times = 0; times < 2;times++)
	{
		if(fabs(line_k1) > 1)
		{
			xa = p1.x + r;
			ya = p1.y;
			
			xb = p2.x + r;
			yb = p2.y;
		}
		else{
			ya = p1.y + r;
			xa =p1.x;
			yb = p2.y + r;
			xb =p2.x;
		}

		dist = sqrt((xa - xb) * (xa - xb) + (ya -yb) * (ya- yb));
		nsteps = (int) (dist / origmap.metersPerPixel + 1);
		for (i = nsteps / 6; i < (nsteps * 5 / 6); i++) 
		{
			double alpha = ((double) i)/nsteps;
			double x = xa*alpha + xb*(1-alpha);
			double y = ya*alpha +yb*(1-alpha);

			int ix = (int) ((x - origmap.x0) / origmap.metersPerPixel);
			int iy = (int) ((y - origmap.y0) / origmap.metersPerPixel);
				
			if(origmap.data[iy*origmap.width + ix] == 0)
			{
				tmppos.x = x;
				tmppos.y = y;
				pathpos.push_back(tmppos);
				posvaild.push_back(0);
				r = -r;
				vaildflag ++;
				break;
			}
		}
		if( vaildflag == 0)
		{
			break;
		}
	}
	if(vaildflag < 2)
	{
		if(pathpos.size() % 2 == 1)
		{
			pathpos.erase(--pathpos.end());
			posvaild.erase(--posvaild.end());
		}
		return -1;
	}
	
	return 0;
}
/*<FUNC+>********************************************************************************/
/* 函数名称: dividecleanmap                                                                  */
/* 功能描述:  地图划分区域入口函数，可能需要更新当前位姿值，方便判断联通关系            */
/*<FH->**********************************************************************************/
void  CleanPlanner::dividecleanmap(GridMap &origmap,Pose &cur)
{
	int tmpx,tmpy;
	int width = origmap.width;
	int ret;
	for(int i = 0;i < linepos.size();i = i+2)
	{
		//origmap.drawLine(linepos[i].x,linepos[i].y,linepos[i+1].x,linepos[i+1].y, -1);
		ret = getmappath(origmap,linepos[i],linepos[i+1]);
	}
	
	for(int i = 0 ;i < pathpos.size(); i ++)
	{
		for(int j = 0;j < pathpos.size();j ++)
		{
			if(i == j)
			{
				posdis.push_back(0);
			}
			else{
				posdis.push_back(999);
			}
		}
	}
	for(int i = 0 ;i < pathpos.size(); i= i + 2)
	{
		int j = i + 1;
		posdis[j * pathpos.size() + i] = 1;
		posdis[i * pathpos.size() + j] = 1;
		
	}
	

	tmpx =  (int)((cur.x - origmap.x0) / origmap.metersPerPixel);
	tmpy =  (int)((cur.y - origmap.y0) / origmap.metersPerPixel);

	if(origmap.data[tmpy * width + tmpx] == 0)
	{
		return;
	}
	//printf("curpose is error\n");

	int size = 5;
	for(int i = -size;i<= size ;i ++)
	{
		for(int j = -size; j <= size; j++)
		{
			if(origmap.data[(tmpy + i) * width + (tmpx + j)] == 0)
			{
				cur.x = (double)(tmpx + j) * origmap.metersPerPixel + origmap.x0;
				cur.y = (double)(tmpy + i) * origmap.metersPerPixel + origmap.y0;	
				return;
			}
		}
	}
	return;
}
/*<FUNC+>********************************************************************************/
/* 函数名称: SetPosePath                                                                  */
/* 功能描述:  根据联通关系及地图，判断生成的区域联通点中，哪些点在当前封闭的区域中       */
/*<FH->**********************************************************************************/
void CleanPlanner::SetPosePath(BYTE *connect,GridMap &origmap)
{
	int tmpx,tmpy;
	int width = origmap.width;
	vector<int> tmppath;
//	printf("IN [%s] LINE :%d\n",__func__,__LINE__);
	for(int i = 0;i < pathpos.size();i++)
	{
		tmpx =  (int)((pathpos[i].x - origmap.x0) / origmap.metersPerPixel);
		tmpy =  (int)((pathpos[i].y - origmap.y0) / origmap.metersPerPixel);

		if(*(connect + tmpx + tmpy * width) == 1)
		{
			posvaild[i] = 1;
			tmppath.push_back(i);
		}
		
	}
	for(int i = 0; i < tmppath.size();i++)
	{
		for(int j = i + 1; j < tmppath.size();j++)
		{		
				int m = tmppath[i];
				int n = tmppath[j];
				posdis[posvaild.size() * m + n ] = 1;
				posdis[posvaild.size() * n + m ] = 1;
		}
	}

	return;
}
/*<FUNC+>********************************************************************************/
/* 函数名称: GetPath                                                                */
/* 功能描述:  递归寻找路径                                                */
/*<FH->**********************************************************************************/
int CleanPlanner::Ppath(vector<int> &pMid,int x,int y,int q)
{
    int k=0;
	int len = posvaild.size();
    k = pMid[x * len + y];
    if(k==-1)
	return q;
    else
    {
	q=Ppath(pMid,x,k,q);
	connectpos.push_back(k);
	q++;
	q=Ppath(pMid,k,y,q); 
	return q;
    }
}

void CleanPlanner::GetPath(int start,int end)
{
    int length = posdis.size();
	int len = posvaild.size();
    int i,j,k;
	vector<int> pA;
	vector<int> pMid;
    for(i=0;i<length;i++)
    {
         pA.push_back(posdis[i]);
		 pMid.push_back(-1);
    }
    for(k=0;k<len;k++)
    { 
		for(i=0;i<len;i++)
			for(j=0;j<len;j++)
				if(pA[i * len +j] > pA[i * len +k] + pA[k * len +j] )
				{
					pA[i * len +j] = pA[i * len +k] + pA[k * len +j];
					pMid[i * len +j] =k;
				} 
    }
	connectpos.push_back(start);
	Ppath(pMid,start,end,0);
	connectpos.push_back(end);
	pMid.clear();
	pA.clear();
}
/*<FUNC+>********************************************************************************/
/* 函数名称: SetPosePath                                                                */
/* 功能描述:  当一次路径规划结束后，寻找下一个路径点                                    */
/*<FH->**********************************************************************************/
int CleanPlanner::GetNextPos(vector<Pose> &cleanpath)
{
	int i = 0;
	vector<int> tmppath;
	int tmpindex = -1;
	int ret;
	int start = 0;
	if(posvaild.size() == 0)
	{
		return -1;
	}
	for(i = 0; i < posvaild.size();i++)
	{
		if(posvaild[i] == 0)
		{
			break;
		}
	}
	if(i == posvaild.size())
	{
		linepos.clear();
		pathpos.clear();
		posvaild.clear();
		return 1;
	}
	for(int i = 0; i < posvaild.size();i++)
	{
		if(posvaild[i] == 1)
		{
			start = i;
			if(i % 2 == 0)
			{
				if(posvaild[i + 1] == 0)
				{
					tmppath.push_back(i);
					tmppath.push_back(i + 1);
					break;
				}
			}
			if(i % 2 == 1)
			{
				if(posvaild[i - 1] == 0)
				{
					tmppath.push_back(i);
					tmppath.push_back(i - 1);
					break;
				}
			}
		}
	}
	if(tmppath.size() == 0)
	{
		for(i = 0; i < posvaild.size();i++)
		{
			if(posvaild[i] == 0)
			{
				connectpos.clear();
				GetPath(start,i);
				if(connectpos.size() > 1)
				{
					tmppath = connectpos;
					break;
				}

			}
		}
	}
	if(	tmppath.size() > 0)
	{
		for(i = 0; i < tmppath.size();i++)
		{
			cleanpath.push_back(pathpos[tmppath[i]]);
		}
	}
	for(int i = 0 ;i < posvaild.size();i++)
	{
		if(posvaild[i] == 1)
		{
			posvaild[i] = -1;
		}
	}
	return 0;
}



int CleanPlanner::creatcleanamap(GridMap &map, Pose curpose,GridMap &resMap)
{

     
        Pose centerxy = curpose;
        
        double width = robot_geometry_width;
        double radius = width;

		int initalwidth = map.width;
		int initalheight = map.height;

		GridMap terrain;
		GridMap terrain2;
		GridMap terrain3;
		int res = 0;
   
        // compute all paths
        double troughWidth = pathPlanning_troughWidth;

		bool bGoalNull = false;

		Pose goal;
		//WavefrontResults result;
		
		if(connectpath.size() == 0)
		{

			if(result.cspace.gm.data != NULL)
			{

				delete [] result.cspace.gm.data;
				result.cspace.gm.data = NULL;
			}
			//saveMap("smztest4.txt",map);
			GetMapLine(map); // add by smz
			//saveMap("smztest3.txt",map);
	    	if(map.metersPerPixel<0.08)
		    {
			   map.decimateMax(2,terrain2);
		     }
		else
		{
			terrain2 = map;
		
			}
			//WavefrontResults result;
			result.reachable = NULL;
			result.radius = radius;
			renderConfigurationSpace(terrain2, centerxy, goal, troughWidth, result.radius,result.cspace);
			dividecleanmap(result.cspace.gm,curpose);  // smz
			//saveMap("smztest.txt",result.cspace.gm);
		}
		
    	BYTE *connected = NULL;
		int length = 0;

    	result.cspace.gm.getConnectedWithin(curpose, 254,&connected,length);

		if(connected != NULL)
		{		
			SetPosePath(connected,result.cspace.gm);  // add by smz
		}

		GridMap connectMap;		
/*
		connectMap.makePixels(result.cspace.gm.x0, result.cspace.gm.y0, result.cspace.gm.width, result.cspace.gm.height, result.cspace.gm.metersPerPixel, 0, false);
	
		initalwidth =result.cspace.gm.width;
		initalheight = result.cspace.gm.height;
*/	
		if (connected == NULL)
        { 
			printf("connected = NULL \n");
			res = -1;
			

		}
		else
		{	
			connectMap.makePixels(result.cspace.gm.x0, result.cspace.gm.y0, result.cspace.gm.width, result.cspace.gm.height, 									result.cspace.gm.metersPerPixel, 0, false);
	
			initalwidth =result.cspace.gm.width;
			initalheight = result.cspace.gm.height;

			for (int y = 0; y < connectMap.height; y++) {
				for (int x = 0; x < connectMap.width; x++) {
					connectMap.data[y*connectMap.width+x] = (-1)*connected[y*connectMap.width+x];
            	}
            }

		
			MyRect bound2;
			getBoundingBox(connectMap,bound2);

			//if(bound2.x0 == 0 && bound2.x1==initalwidth && bound2.y0 == 0 && bound2.y1==initalheight)
			//	res = -2;
			
			if((initalwidth-(bound2.x1-bound2.x0))<40 && (initalheight-(bound2.y1-bound2.y0))<40)
				res = -2;
			

			connectMap.cropPixels(bound2.x0-2, bound2.y0-2, bound2.x1-bound2.x0+4, bound2.y1-bound2.y0+4, false,terrain3);

			for (int y = 0; y < terrain3.height; y++) {
				for (int x = 0; x < terrain3.width; x++) {
					if(terrain3.data[y*terrain3.width+x] == 0xFF)

						terrain3.data[y*terrain3.width+x] =0;
					else
					{
							terrain3.data[y*terrain3.width+x] =-1;
					}

            	}
            }
			resMap = terrain3;
			
    	}
		//printf("[%s] PASS LINE %d\n",__func__,__LINE__);
		//saveMap("resultmap.txt",terrain3);
		

/***********************add by smz****************************/
		//printf("[%s] PASS LINE %d\n",__func__,__LINE__);
		connectpath.clear();
		GetNextPos(connectpath);
		//printf("[%s] PASS LINE %d\n",__func__,__LINE__);		
/***********************add end****************************/
		
		if(terrain.data != NULL)
		{
			delete [] terrain.data;
			terrain.data = NULL;
		}
		if(terrain2.data != NULL)
		{
			delete [] terrain2.data;
			terrain2.data = NULL;
		}
		if(terrain3.data != NULL)
		{
			delete [] terrain3.data;
			terrain3.data = NULL;
		}
		if(connectMap.data !=NULL)
		{
			delete [] connectMap.data;
			connectMap.data = NULL;
		}
/*
		if(result.cspace.gm.data != NULL)
		{

			delete [] result.cspace.gm.data;
			result.cspace.gm.data = NULL;
		}
*/
		if(connected!=NULL)
		{
			delete [] connected;
			connected = NULL;
		}

		return res;
    }
    
	bool CleanPlanner::cleanmapplan(GridMap &cleanmap, Pose curpose,Pose goal,vector<vector<double> > &path, int curx,int cury)
	{

        // maximum acceptable cost
		int maxCost = pathPlanning_maxCost;
		Pose centerxy = curpose;
        // get reachable terrain,
        int nreachableLengh=0; 

		BYTE *pReachable = NULL;
		WavefrontResults res;
		res.reachable = NULL;	
		res.cspace.gm = cleanmap;
		//res.cspace.gm.getConnectedWithin(centerxy, maxCost,&res.reachable,nreachableLengh);
		
		res.cspace.gm.getConnectedWithin(curx,cury, maxCost,&res.reachable,nreachableLengh);

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


		if ((*res.wavefront) == NULL)
			return false;  //no path;
		else
		{
			double xy[2];
			xy[0] = centerxy.x;
			xy[1] = centerxy.y;
			computeWavefrontPath(res.cspace, res.target, xy, (res.wavefront),res.path);

		}
	   delete [] res.wavefront;

	   	path = res.path;
      

		return true;


	}

	int CleanPlanner:: FindLeftNeighbour(GridMap &map,POINT &ptCur, POINT &ptLeft,bool bj1,POINT ptPre, POINT ptNext,int LineGap )
	{
	//TRACE("FIND BEGIN \n");
	//左下角为（0，0）
	int i = ptCur.x;
	int j = ptCur.y;
	unsigned char *m_pbFind = NULL;

	unsigned int m_nWidthNum;
	unsigned int m_nHeightNum;

	GridMap bordmap;
	map.BorderMap(bordmap);



	m_nWidthNum = map.width ;
	m_nHeightNum = map.height;

	if (m_nHeightNum>0 && m_nWidthNum>0)
	{
		m_pbFind = new unsigned char[m_nWidthNum*m_nHeightNum];
		memset(m_pbFind, 0, m_nWidthNum*m_nHeightNum);
	}	
	else
	{
		return -1;

	}
	if(i<0 || i>(m_nWidthNum-1) || j<0 || j>(m_nHeightNum-1))
	{
		if(m_pbFind!=NULL)
		{
			delete [] m_pbFind;
			m_pbFind = NULL;	
		}
		return -1;
	}

	m_pbFind[(j)*m_nWidthNum+i] = 1;

	if(ptCur.x-LINEDIS<0)
	{
		if(m_pbFind!=NULL)
		{
			delete [] m_pbFind;
			m_pbFind = NULL;	
		}
		return -1;
	}

	int k = 0;
	while(1)
	{
		if(k==0)
		{
			//第一个点只能在左侧找
			if(i-1==0)
			{
				//在左侧无障碍物了
				if(m_pbFind!=NULL)
				{
					delete [] m_pbFind;
					m_pbFind = NULL;	
				return -1;
				}
			}
			else 
			{
				// 
				if(bordmap.data[j*m_nWidthNum+i-1] == 0xFF)
				{
					i = i-1;
					j = j;
					m_pbFind[j*m_nWidthNum+i] = 1;
					
				}
				else{
					if(bordmap.data[(j-1)*m_nWidthNum+i-1]== 0xFF && bordmap.data[(j+1)*m_nWidthNum+i-1]== 0xFF)
					{
						if(bj1==true)
						{
							i = i-1;
							j = j+1;
							m_pbFind[j*m_nWidthNum+i] = 1;

						}
						else{
							i = i-1;
							j = j-1;
							m_pbFind[j*m_nWidthNum+i] = 1;
						}

					}
					else if(bordmap.data[(j-1)*m_nWidthNum+i-1]== 0xFF)
					{
							i = i-1;
							j = j-1;
							m_pbFind[j*m_nWidthNum+i] = 1;;

					}
					else if(bordmap.data[(j+1)*m_nWidthNum+i-1]== 0xFF)
					{
							i = i-1;
							j = j+1;
							m_pbFind[j*m_nWidthNum+i] = 1;
					}
					else
					{
						if(j-1>=0)
						{
							if(bordmap.data[(j-1)*m_nWidthNum+i]== 0xFF )//在左侧下面无障碍物了
							{
									i = i;
									j = j-1;
									m_pbFind[j*m_nWidthNum+i] = 1;

							}
							else
							{
								if(bordmap.data[(j+1)*m_nWidthNum+i]== 0xFF )//在左侧上无障碍物了
								{   //判断i+1是否超出边界？？？？
									i = i;
									j = j+1;
									m_pbFind[j*m_nWidthNum+i] = 1;

								}
		
								else if(m_pbFind!=NULL)
								{
									delete [] m_pbFind;
									m_pbFind = NULL;		
									return -1;
								}

							}

						}
						else{
								if(m_pbFind!=NULL)
								{
									delete [] m_pbFind;
									m_pbFind = NULL;		
									return -1;
								}
						}

					}

				}

			}
		
		}
		else
		{
			int mindis = 10;
			int x = -1;
			int y = -1;

			/*TRACE("BORDER  ****" );
			for(int n = j-1;n<=j+1;n++)
			{			
				for(int m = i-1;m<= i+1;m++)	
				{
					TRACE("%d,",m_pBorder[n*m_nWidthNum+m] );

				}
				TRACE("\n" );
			}
			TRACE("BFINDER  ****" );

			for(int n = j-1;n<=j+1;n++)
			{			
				for(int m = i-1;m<= i+1;m++)	
				{
					TRACE("%d,",m_pbFind[n*m_nWidthNum+m] );

				}
				TRACE("\n" );
			}*/




			int bx0 = max(i-1,0);
			int by0 = max(j-1,0);

			int bx1 = min(i+1,(int)(m_nWidthNum-1));
			int by1 = min(j+1,(int)(m_nHeightNum-1));
			

			for(int n = by0;n<=by1;n++)
			{			
				for(int m = bx0;m<= bx1;m++)	
				{

				//	TRACE("BORDER= %d,  i = %d, j=%d\n",m_pBorder[n*m_nWidthNum+m],m,n);

					if(m==i&& n==j)
						continue;
					if(m<0 || m>=m_nWidthNum || n<0 || n>=m_nHeightNum)
						continue;
					if(m_pbFind[n*m_nWidthNum+m] == 1)
						continue;
					if(bordmap.data[n*m_nWidthNum+m] == 0xFF)
					{
						int dis = ((m-i)*(m-i)+(n-j)*(n-j));

						if(dis< mindis)
						{
							mindis = dis;
							x = m;
							y = n;
						}
						else if(dis == mindis)
						{
							if(m<x)
							{
								x = m;
								y = n;

							}

						}
					}

				}
			}
			if(x == -1 || y==-1)
			{
				if(m_pbFind!=NULL)
				{
					delete [] m_pbFind;
					m_pbFind = NULL;
				}			
				

				return -1;
			}
			else
			{
				i = x;
				j = y;
				m_pbFind[j*m_nWidthNum+i] = 1;
			}		

		}
		if(ptPre.x!=-1)
		{
			if(i == ptPre.x && j==ptPre.y)
			{
			//左侧又绕回，全是障碍物了
				if(m_pbFind!=NULL)
				{
					delete [] m_pbFind;
					m_pbFind = NULL;
				}
				
				return -1;
			}
		}
		if(ptNext.x!=-1)
		{
			if(i == ptNext.x && j==ptNext.y)
			{
			//左侧又绕回，全是障碍物了
				if(m_pbFind!=NULL)
				{
					delete [] m_pbFind;
					m_pbFind = NULL;
				}
				
				return -1;
			}
		}
		if(i == ptCur.x - LineGap)
		{
			if(j+1<m_nHeightNum)
			{
				if(bj1== true && map.data[(j+1)*m_nWidthNum+i]==0)
				{

						ptLeft.x =i;
						ptLeft.y = j;
						if(m_pbFind!=NULL)
						{
						delete [] m_pbFind;
						m_pbFind = NULL;
						}
						
				
						return ptLeft.y;

				}
			}
			if(bj1== false && map.data[(j-1)*m_nWidthNum+i]==0)
			{

					ptLeft.x =i;
					ptLeft.y = j;
					if(m_pbFind!=NULL)
					{
					delete [] m_pbFind;
					m_pbFind = NULL;
					}
					//TRACE("FIND END \n");
				
					return ptLeft.y;

			}
			/*bool bfind = false;
			for(int r = m_nHeightNum-1;r>=0;r-- )
			{
				if(m_pbFind[r*m_nWidthNum+ptCur.x - LINEDIS]==1)
				{
					ptLeft.x = ptCur.x - LINEDIS;
					ptLeft.y = r;
					if(m_pbFind!=NULL)
					{
					delete m_pbFind;
					m_pbFind = NULL;
					}
					TRACE("FIND END \n");
					bfind = true;
					return ptLeft.y;
				}

			}
			if(bfind ==false)
				return -1;*/

		}

		k++;
	}

	if(m_pbFind!=NULL)
	{
		delete [] m_pbFind;
		m_pbFind = NULL;
	}



	}



bool CleanPlanner::GetCleanAllLine(GridMap &map, vector< vector<CleanLine> > &vtAllCleanLine)
{

	unsigned int m_nWidthNum;
	unsigned int m_nHeightNum;
	int begin=0;
	int lastline = 0;

	m_nWidthNum = map.width ;
	m_nHeightNum = map.height;

	if( map.width <=0 || map.height<=0)
	{
		return false;
	}


		for(int j = 1;j<m_nHeightNum-2;j++)
		{
			for(int i = 1; i<m_nWidthNum-2;i++)
			{
					
					if(map.data[j*m_nWidthNum+i] ==  0)
					{
						int n = 1;
						while(n<8)
						{

							if(i-n>=1)
								map.data[j*m_nWidthNum+i-n] = 0;
							else
								break;
							n++;
						}
						break;
					}


			}
		}

		for(int j = 2;j<m_nHeightNum-2;j++)
		{
			for(int i = m_nWidthNum-1; i>2;i--)
			{
					
					if(map.data[j*m_nWidthNum+i] ==  0)
					{
						int n = 1;
						while(n<8)
						{
							if((i+n)<(m_nWidthNum-2))
								map.data[j*m_nWidthNum+i+n] = 0;
							n++;
						}
						break;
					}


			}
		}
		



	for (int i = 0; i<m_nWidthNum;i= i+1)
	{
		bool bfind = false;
		for(int j = 0; j<m_nHeightNum;j++)
		{
				if(map.data[j*m_nWidthNum+i] == 0)
				{	
					begin = i;
					bfind = true;
					break;
				}

		}
		if(bfind)
			break;

	}

	for (int i = m_nWidthNum-1; i>0;i--)
	{
		bool bfind = false;
		for(int j = 0; j<m_nHeightNum;j++)
		{
				if(map.data[j*m_nWidthNum+i] == 0)
				{	
					lastline = i;
					bfind = true;
					break;
				}

		}
		if(bfind)
			break;

	}	


	//int n = (lastline - begin)%LINEDIS;

	

	int i = begin;
	while(i<m_nWidthNum)
	{

		vector<CleanLine> vtcleanline;
		vector<int> pt;

		for(int j = 0; j<m_nHeightNum;j++)
		{
				if(map.data[j*m_nWidthNum+i] == 0)
				{
					if(pt.size()<=0 )
					{			
						pt.push_back(j);						
						if(j!= (m_nHeightNum-1))
						{
							if(map.data[(j+1)*m_nWidthNum+i] == 0xFF )
								pt.push_back(j);	
						}
					}
					else if(j == (m_nHeightNum-1) && (map.data[j*m_nWidthNum+i] == 0) )
					{				
						pt.push_back(j);
						if(map.data[(j-1)*m_nWidthNum+i] == 0xFF)										
							pt.push_back(j);  	

					}
					else {
							if(map.data[(j-1)*m_nWidthNum+i] == 0xFF)														
								pt.push_back(j);										
							if(j!= (m_nHeightNum-1))
							{
								if(map.data[(j+1)*m_nWidthNum+i] == 0xFF)												
									pt.push_back(j);	
							}
					}
				}	
		}

		if (pt.size()>0)
		{
			if(pt.size()%2 ==0)
			{

				int no = 0;
				for(int m= 0;m<pt.size();m=m+2)
				{
					CleanLine line;
					no++;
					line.i = i;
					line.j1 = pt.at(m);
					line.j2 = pt.at(m+1);

					line.areaInx = -1;
					line.bCleaned = false;
					vtcleanline.push_back(line);

				}
			}
		}

		vtAllCleanLine.push_back(vtcleanline);


		if(i==lastline)
			break;

		i = i+ LINEDIS;
		if(lastline!=0)
		{

			if(i>=m_nWidthNum)
			{	
				if(i-lastline>2)
					break;
				i = lastline;		

			}
		}
	}


	return true;

}



void CleanPlanner::PlanBorderPath(GridMap &map,Pose &beginPos,vector<Pose> &path)
{

	POINT ptcur;

	ptcur.x = (int) ((beginPos.x - map.x0) /map.metersPerPixel);
    ptcur.y = (int) ((beginPos.y - map.y0)/  map.metersPerPixel);


	Pose pathpos;
	
	int i = ptcur.x;
	int j = ptcur.y;
	int no = 0;


	GridMap bordmap;
	map.BorderMap(bordmap);

	pathpos.x = map.x0 + i*map.metersPerPixel;
	pathpos.y = map.y0 + j*map.metersPerPixel;

	path.push_back(pathpos);
	double mindis = 999999999999999999;
					
	for(int n = 0;n<bordmap.height;n++)
	{			
			for(int m = 0;m<bordmap.width;m++)	
			{
				if(bordmap.data[n*bordmap.width+m] == 0xFF)
				{
						
						double dis = (n-ptcur.y)*(n-ptcur.y)+(m-ptcur.x)*(m-ptcur.x);
						if(dis <mindis)
						{
							mindis = dis;
							j= n;
							i= m;
						}
							
				}

			}
			
	}

	pathpos.x = map.x0 + i*map.metersPerPixel;
	pathpos.y = map.y0 + j*map.metersPerPixel;

	path.push_back(pathpos);

	while(1)
	{

			bool haveOc = false;

			int bx0 = max(i-1,0);
			int by0 = max(j-1,0);

			int bx1 = min(i+1,bordmap.width-1);
			int by1 = min(j+1,bordmap.height-1);
			

			for(int n = by0;n<=by1;n++)
			{			
				for(int m = bx0;m<= bx1;m++)	
				{
	

					if(m==i&& n==j)
					{	
						bordmap.data[n*bordmap.width+m] = 8;
					    continue;

					}

					if(m<0 || m>=bordmap.width || n<0 || n>=bordmap.height)
						continue;

					if(bordmap.data[n*bordmap.width+m] == 0xFF)
					{
						haveOc = true;
						j= n;
						i= m;
						bordmap.data[n*bordmap.width+m] = 8;
						if(no%20 == 0 )
						{
							pathpos.x = map.x0 + m*map.metersPerPixel;
							pathpos.y = map.y0 + n*map.metersPerPixel;

							path.push_back(pathpos);

						
						}
						{

						//	pathpos.x = map.x0 + m*map.metersPerPixel;
						//	pathpos.y = map.y0 + n*map.metersPerPixel;

						//	path.push_back(pathpos);
						}

						break;
					}
				}
				if(haveOc)
					break;
			}

			bool  bborderover = true;
			if(!haveOc)
			{
				double mindis = 99999999999999999;
				Pose InitPos;
				Pose goal;
				int initI = i;
				int initJ = j;
				bool bhaveinit = false;
				bool bhavegoal = false;

				/*for(int k = j-1;k<=j+1;k++)
				{
					for(int t = i-1;t<=i+1;t++)
					{
						if(k<0)
							k=0;
						if(k>=map.height)
							k=map.height-1;
						if(t<0)
							t=0;
						if(t>=map.width)
							t=map.width-1;
						
						if(map.data[k*map.width+t] == 0)
						{
								bhaveinit = true;
								InitPos.x = map.x0 + t*map.metersPerPixel;
								InitPos.y = map.y0 + k*map.metersPerPixel;
								initI = t;
								initJ = k;
						}
					}
				}*/

				for(int n = 0;n<bordmap.height;n++)
				{			
					for(int m = 0;m<bordmap.width;m++)	
					{
						if(bordmap.data[n*bordmap.width+m] == 0xFF)
						{
						
							bborderover = false;

							double dis = (n-initJ)*(n-initJ)+(m-initI)*(m-initI);
							if(dis <mindis)
							{
								mindis = dis;
								j= n;
								i= m;
							}
							
						}

					}
			
				}

				if(bborderover == false)
				{
					/*for(int k = j-1;k<=j+1;k++)
					{
						for(int t = i-1;t<=i+1;t++)
						{
						if(k<0)
							k=0;
						if(k>=map.height)
							k=map.height-1;
						if(t<0)
							t=0;
						if(t>=map.width)
							t=map.width-1;
							if(map.data[k*map.width+t] == 0)
							{
								bhavegoal = true;
								goal.x = map.x0 + t*map.metersPerPixel;
								goal.y = map.y0 + k*map.metersPerPixel;
		
							}
						}
					}*/

	
					/*if(bhavegoal && bhaveinit)
					{
						vector< vector<double>> wavepath;

						cleanmapplan(map,InitPos,goal,wavepath,initI,initJ);

						for(int t = 0;t<wavepath.size();t++)
						{
							vector<double> xy = wavepath.at(t);
							if(xy.size()>=2)
							{
								Pose pos;
								pos.x = xy.at(0);
								pos.y = xy.at(1);

								path.push_back(pos);
							}

						}	
					}
					else*/
					{

						pathpos.x = map.x0 + i*map.metersPerPixel;
						pathpos.y = map.y0 + j*map.metersPerPixel;

						path.push_back(pathpos);
					}
				}
				

				// over 
				if(bborderover)
				{
						pathpos.x = map.x0 + i*map.metersPerPixel;
						pathpos.y = map.y0 + j*map.metersPerPixel;

						path.push_back(pathpos);
				}
					break;
			}

		no++;

	}


}

void CleanPlanner::GetCleanArea(GridMap &map, vector< vector<CleanLine> > &vtAllCleanLine,vector<CleanArea> &vtArea)
{

	int lasti = 0;
	for (int m= 0; m<vtAllCleanLine.size();m++)
	{
		//TRACE("m=%d\n",m);
		//int curi = m*LINEDIS;
		vector<CleanLine>  vtCurLine;

		vtCurLine = vtAllCleanLine.at(m);

		if(vtCurLine.size()>0)
		{

			int gap = LINEDIS;
			if(m ==0)
			{
				gap == LINEDIS;
			}
			if(vtCurLine.size()>0)
			{
				if(m!=0)
					gap = vtCurLine.at(0).i-lasti;
				lasti = vtCurLine.at(0).i;
			}
		if(vtArea.size()<=0)
		{
			for(int n = 0;n<vtCurLine.size();n++)
			{
				CleanArea area;
				vtAllCleanLine.at(m).at(n).areaInx = n+1;			
				area.inx = n+1;
				area.vtArea.push_back(vtCurLine.at(n));		
				vtArea.push_back(area);
				
			}


		}
		else
		{

			for(int n = 0;n<vtCurLine.size();n++)
			{
				CleanLine curLine = vtCurLine.at(n);
				int res = -1;
				int curi = curLine.i;
				//TRACE("n=%d\n",n);

				POINT ptcur;
				ptcur.x = curi;
				ptcur.y = curLine.j1-1;
				POINT ptleft;

				POINT ptPre;
				POINT ptNext;

				if(n==0)
				{
					ptPre.x = -1;
					ptPre.y = -1;
					ptNext.x = curi;
					ptNext.y = curLine.j2+1;
				}
				else
				{
					ptPre.x = vtCurLine.at(n-1).i;
					ptPre.y = vtCurLine.at(n-1).j2+1;
					ptNext.x = curi;
					ptNext.y = curLine.j2+1;
					
				}

				res = FindLeftNeighbour(map,ptcur, ptleft,true,ptPre, ptNext , gap );

				if(res!=-1)
				{
					res = res+1;
				}
				if(res == -1 )
				{
					//加入新的区域
				
					int size = vtArea.size();
					vtAllCleanLine.at(m).at(n).areaInx = size+1;
			
					CleanArea area;
					area.inx = size+1;
					area.vtArea.push_back(curLine);
					vtArea.push_back(area);


				}

				if(res>=0)
				{
				
					vector<CleanLine>  vtLeftLine;
					vtLeftLine =  vtAllCleanLine.at(m-1);
					int leftj = -1;
					int leftLineAreaInx = 0;
					int tt = 0;

					for(int t = 0;t<vtLeftLine.size();t++)
					{
						CleanLine leftline = vtLeftLine.at(t);
						if(leftline.j1 == res)
						{
							
							tt = t;
							leftj = leftline.j2;
							leftLineAreaInx = leftline.areaInx;
							break;
							
						}
					}
					if(leftj == -1)
					{
						//出错，不对	
						printf("ERROR 1\n");
													//加入新的区域				
						int size = vtArea.size();
						vtAllCleanLine.at(m).at(n).areaInx = size+1;
						CleanArea area;
						area.inx = size+1;
						area.vtArea.push_back(curLine);
						vtArea.push_back(area);

					}
					else{

							int resj =-1;
						 	POINT ptcur;
							ptcur.x = curi;
							ptcur.y = curLine.j2+1;
						 	if(n==vtCurLine.size()-1)
							{
								ptPre.x = curi;
								ptPre.y = curLine.j1-1;
								ptNext.x = -1;
								ptNext.y = -1;
							}
							else
							{
								ptPre.x = curi;
								ptPre.y = curLine.j1-1;
								ptNext.x = curi;
								ptNext.y = vtCurLine.at(n+1).j1-1;
					
							}
							resj = FindLeftNeighbour(map,ptcur, ptleft,false,ptPre, ptNext, gap );
							if(resj!=-1)
							{
								resj = resj-1;
							}
				
						 if(resj == -1)
						 {				
							//加入新的区域					
							int size = vtArea.size();
							vtAllCleanLine.at(m).at(n).areaInx = size+1;			
							
							CleanArea area;
							area.inx = size+1;
							area.vtArea.push_back(curLine);
							vtArea.push_back(area);					

						 }
						 else if(leftj == resj)
						 {
							 //属于一个区域
							 	vtAllCleanLine.at(m).at(n).areaInx = leftLineAreaInx;
								curLine.areaInx = leftLineAreaInx;
								//有问题						
								vtArea.at(curLine.areaInx -1).vtArea.push_back(curLine);

						 }
						 else
						 {
							//加入新的区域
							
							int size = vtArea.size();
							vtAllCleanLine.at(m).at(n).areaInx = size+1;
							
							CleanArea area;
							area.inx = size+1;
							area.vtArea.push_back(curLine);
							vtArea.push_back(area);

						 }					 

					}

				}

			}

		}

		}
	}

}

void CleanPlanner::PlanAreaPath(GridMap &map, Pose &curPose,vector<CleanArea> &vtArea, vector<Pose> &path)
{
		printf("clean PLAN \n");
	//0  1
	//2  3

		

		POINT curPt;
	    double pixelsPerMeter = 1.0 / map.metersPerPixel;
		int dis1 = Integer_MAX_VALUE; 

		POINT firstPt;
		Pose firstGoal;
		int goalx;
		int goaly;

		int k=0;
		int cornum = 2;
		int num= 0;


		curPt.x = (int) ((curPose.x - map.x0) * pixelsPerMeter);
        curPt.y = (int) ((curPose.y - map.y0) * pixelsPerMeter);

		if(vtArea.size()<=0)
		{
			return ;
		}



	

	for (int i = 0;i<vtArea.size();i++)
	{
		CleanArea area;
		
		area = vtArea.at(i);
		int size = area.vtArea.size();
		//area.inx = i+1;

		vtArea.at(i).bFind = false;

		vtArea.at(i).cor[0].x = area.vtArea.at(0).i;
		vtArea.at(i).cor[0].y = area.vtArea.at(0).j2;

		vtArea.at(i).cor[1].x = area.vtArea.at(size-1).i;
		vtArea.at(i).cor[1].y = area.vtArea.at(size-1).j2;

		vtArea.at(i).cor[2].x = area.vtArea.at(0).i;
		vtArea.at(i).cor[2].y = area.vtArea.at(0).j1;

		vtArea.at(i).cor[3].x = area.vtArea.at(size-1).i;
		vtArea.at(i).cor[3].y = area.vtArea.at(size-1).j1;


		for(int t =0;t<4;t++)
		{
			int dis = (vtArea.at(i).cor[t].x - curPt.x)*(vtArea.at(i).cor[t].x - curPt.x)
				+(vtArea.at(i).cor[t].y - curPt.y)*(vtArea.at(i).cor[t].y - curPt.y);
			if(dis<dis1)
			{
				dis1 = dis;			
				goalx = vtArea.at(i).cor[t].x;
				goaly = vtArea.at(i).cor[t].y;	
				k = i;
				cornum = t;
			}

		}
	}
	

	CleanArea area;
	area = vtArea.at(0);
	if(area.vtArea.size()>0)
	{
		k = 0;
		cornum = 0;
		goalx = vtArea.at(0).cor[0].x;
		goaly = vtArea.at(0).cor[0].y;
	}
	else
	{
		return;
	}

	
	firstGoal.x = map.x0 + goalx*map.metersPerPixel;
	firstGoal.y = map.y0 + goaly*map.metersPerPixel;
	vector< vector<double> > wavepath;

	cleanmapplan(map,curPose,firstGoal,wavepath,curPt.x,curPt.y);

	//printf("curPose x = %f,curPose y=%f\n", curPose.x,curPose.y);
	////////////////////////////
/*	path.push_back(firstGoal);

		for(int t = 0;t<wavepath.size();t++)
		{
			vector<double> xy = wavepath.at(t);
			if(xy.size()>=1)
			{
				Pose pos;
				pos.x = xy.at(0);
				pos.y = xy.at(1);
				//printf("111 pose x = %f,pose y=%f\n", pos.x,pos.y);
				path.push_back(pos);
			}

		}
	//////////////////////////////
	
	*/

	while(1)
	{
		Pose goal;
		Pose curPos;
		int x;
		int y;

		double mindis = 999999999999999;

		if(cornum==0||cornum==2)
		{

			for(int j=0;j<vtArea.at(k).vtArea.size();j++)
			{
				POINT p1;
				POINT p2;
				Pose pos1;
				Pose pos2;

				if( (cornum == 2 && (j%2==0)) || (cornum == 0 && (j%2==1)))
				{
					p1.x = vtArea.at(k).vtArea.at(j).i;
					p1.y = vtArea.at(k).vtArea.at(j).j1;

					p2.x = vtArea.at(k).vtArea.at(j).i;
					p2.y = vtArea.at(k).vtArea.at(j).j2;
					
					curPt.x = p2.x;
					curPt.y = p2.y;

				}
				if((cornum == 2 && (j%2==1)) || (cornum == 0 && (j%2==0)))
				{
					p1.x = vtArea.at(k).vtArea.at(j).i;
					p1.y = vtArea.at(k).vtArea.at(j).j2;

					p2.x = vtArea.at(k).vtArea.at(j).i;
					p2.y = vtArea.at(k).vtArea.at(j).j1;

					curPt.x = p2.x;
					curPt.y = p2.y;

				}
												
				pos1.x = map.x0 + p1.x*map.metersPerPixel;
				pos1.y = map.y0 + p1.y*map.metersPerPixel;

				pos2.x = map.x0 + p2.x*map.metersPerPixel;
				pos2.y = map.y0 + p2.y*map.metersPerPixel;

				//printf(" pose x = %f,pose y=%f\n", pos1.x,pos1.y);
				//printf(" pose x = %f,pose y=%f\n", pos2.x,pos2.y);
				path.push_back(pos1);
				path.push_back(pos2);

			}
		}
		if(cornum==1||cornum==3)
		{

			for(int j=vtArea.at(k).vtArea.size()-1;j>=0;j--)
			{
				int size  = vtArea.at(k).vtArea.size()-1;
				POINT p1;
				POINT p2;
				Pose pos1;
				Pose pos2;
				if( (cornum == 3 && ((size-j)%2==0)) || (cornum == 1 && ((size-j)%2==1)))
				{
					p1.x = vtArea.at(k).vtArea.at(j).i;
					p1.y = vtArea.at(k).vtArea.at(j).j1;

					p2.x = vtArea.at(k).vtArea.at(j).i;
					p2.y = vtArea.at(k).vtArea.at(j).j2;

					curPt.x = p2.x;
					curPt.y = p2.y;

				}
				if((cornum == 3 && ((size-j)%2==1)) || (cornum == 1 && ((size-j)%2==0)))
				{
					p1.x = vtArea.at(k).vtArea.at(j).i;
					p1.y = vtArea.at(k).vtArea.at(j).j2;

					p2.x = vtArea.at(k).vtArea.at(j).i;
					p2.y = vtArea.at(k).vtArea.at(j).j1;

					curPt.x = p2.x;
					curPt.y = p2.y;

				}
				pos1.x = map.x0 + p1.x*map.metersPerPixel;
				pos1.y = map.y0 + p1.y*map.metersPerPixel;

				pos2.x = map.x0 + p2.x*map.metersPerPixel;
				pos2.y = map.y0 + p2.y*map.metersPerPixel;
				//printf(" pose x = %f,pose y=%f\n", pos1.x,pos1.y);
				//printf(" pose x = %f,pose y=%f\n", pos2.x,pos2.y);

				path.push_back(pos1);
				path.push_back(pos2);

			}
		}
	
		vtArea.at(k).bFind = true;
		
		num++;
	//	TRACE("num =%d, vtArea.size=%d,  cur = %d \n",num,vtArea.size(), k+1);

		if(num==(vtArea.size()))
		{
			break;
		}
		
		int bfindnum =0;

		for(int i = 0;i<vtArea.size();i++)
		{
			if(vtArea.at(i).bFind!=true)
			{
				for(int j=0;j<4;j++)
				{
					//double dis = sqrt((double)((curPt.x-vtArea.at(i).cor[j].x)*(curPt.x-vtArea.at(i).cor[j].x)
						//		  +(curPt.y-vtArea.at(i).cor[j].y)*(curPt.y-vtArea.at(i).cor[j].y)));
					

						double dis =0.0;
						Pose cur;
						Pose goal;

						Pose pos1;
						Pose pos2;

						cur.x = curPt.x*map.metersPerPixel+map.x0;
						cur.y = curPt.y*map.metersPerPixel+map.y0;

						goal.x = vtArea.at(i).cor[j].x*map.metersPerPixel+map.x0;
						goal.y = vtArea.at(i).cor[j].y*map.metersPerPixel+map.y0;
						vector< vector<double> > wavepath;

						cleanmapplan(map,cur,goal,wavepath,curPt.x,curPt.y);

						for(int t = 0;t<wavepath.size();t++)
						{
							vector<double> xy = wavepath.at(t);
							if(xy.size()>=2)
							{
								Pose pos;
								pos.x = xy.at(0);
								pos.y = xy.at(1);

							//	path.push_back(pos);
							}
							if(t==0)
							{
								pos1.x = curPt.x;
								pos1.y = curPt.y;
								pos2.x = xy.at(0);
								pos2.y = xy.at(1);
								dis = dis+ sqrt((double)((pos1.x-pos2.x)*(pos1.x-pos2.x)
								  +(pos1.y-pos2.y)*(pos1.y-pos2.y)));
								pos1.x = xy.at(0);
								pos1.y = xy.at(1);

							}
							else
							{
								pos2.x = xy.at(0);
								pos2.y = xy.at(1);
								dis = dis+ sqrt((double)((pos1.x-pos2.x)*(pos1.x-pos2.x)
								  +(pos1.y-pos2.y)*(pos1.y-pos2.y)));
								pos1.x = xy.at(0);
								pos1.y = xy.at(1);

							}

						}




					if(dis<mindis)
					{
						k = i;
						mindis = dis;
						cornum = j;
						x = vtArea.at(i).cor[j].x;
						y = vtArea.at(i).cor[j].y;
					}
				}

			}


		}

		goal.x = map.x0 + x*map.metersPerPixel;
		goal.y = map.y0 + y*map.metersPerPixel;

		curPos.x = map.x0 + curPt.x*map.metersPerPixel;
		curPos.y = map.y0 + curPt.y*map.metersPerPixel;

		
		/*vector< vector<double> > wavepath;

		cleanmapplan(map,curPos,goal,wavepath,curPt.x,curPt.y);

		for(int t = 0;t<wavepath.size();t++)
		{
			vector<double> xy = wavepath.at(t);
			if(xy.size()>=2)
			{
				Pose pos;
				pos.x = xy.at(0);
				pos.y = xy.at(1);
	//printf("333 pose x = %f,pose y=%f\n", pos.x,pos.y);
				path.push_back(pos);
			}

		}	*/

	}
	printf("clean PLAN over\n");
	}

void CleanPlanner::saveMap(const char* fileName, GridMap &map)
{
 	int range = 15;
	double resolution = 0.05;
 	ofstream outFile;
 	printf("save = %s\n",fileName);
	outFile.open(fileName,ios::out);


	if (!outFile)
	{	
		return;
	}


	outFile<<range;
	outFile<<' ';
	outFile<<resolution;
	outFile<<' ';
	outFile<<map.height;
	outFile<<' ';
	outFile<<map.width;
	outFile<<' ';
	outFile<<map.metersPerPixel;
	outFile<<' ';
	outFile<<map.x0;
	outFile<<' ';
	outFile<<map.y0;
	outFile<<endl;

	for (int j=0;j<map.height;j++)	
	{
		for (int i=0;i<map.width;i++)
		{
			int status =0;
			status = map.data[j*map.width+i];
			if (status == 255)
			{
				outFile<<-1;
			}
			else{
				outFile<<0;
			}
			
			outFile<<' ';
		}
		outFile<<endl;
	}

	outFile.close();





}
	int  CleanPlanner::PlanCleanPath(GridMap &rawmap,Pose& curPose, vector<vector<double> > &path)
	{
		 printf("ENTER PlanCleanPath \n");
		 
		 vector< vector<CleanLine> > vtAllCleanLine;
		 vector<CleanArea> vtArea;
		 vector<Pose>  vtPath;

		 if(rawmap.width<=0 || rawmap.height<=0)
		 {
			return -3;
		 }

		 GridMap cleanmap;
		 GridMap verticalmap;
		 GridMap horizontalmap;

		 int res = creatcleanamap(rawmap, curPose,verticalmap) ;
		
		//printf("cleanmap.wid =%d,cleanmap.height=%d\n",cleanmap.width,cleanmap.height);
		 if(res!=0)
		 {
			 printf(" return :create map false  \n");
				// -1 connected null ; -2 map in not closed; 
			 return res;

		 }

		 if(verticalmap.height<verticalmap.width)
		 {
			
			horizontalmap.width = verticalmap.height;
			horizontalmap.height = verticalmap.width;
			horizontalmap.metersPerPixel = verticalmap.metersPerPixel;

			horizontalmap.x0 = -verticalmap.y0 - verticalmap.metersPerPixel*verticalmap.height;
			horizontalmap.y0 = verticalmap.x0;
			horizontalmap.data  = NULL;
	
			if(horizontalmap.width>0 && horizontalmap.height>0)
			{
				horizontalmap.data = new BYTE[horizontalmap.width*horizontalmap.height ];
				memset(horizontalmap.data, 0, horizontalmap.width*horizontalmap.height );
			}

			for(int j=0;j<horizontalmap.height;j++)
				for(int i = 0;i<horizontalmap.width;i++)
				{
					horizontalmap.data[j*horizontalmap.width+i] = verticalmap.data[ verticalmap.width*(horizontalmap.width-i-1)+j];
				}

				cleanmap = horizontalmap;
	
		 }
		 else
		 {
			 cleanmap =verticalmap;
		 }


		 saveMap("/mnt/cf/cleanmap.txt",cleanmap);

		 GridMap bakmap;

		 bakmap = cleanmap;
		 GetCleanAllLine(bakmap,vtAllCleanLine);

		printf(" line size=%d\n",	vtAllCleanLine.size());

		 GetCleanArea(bakmap,vtAllCleanLine,vtArea);
		printf(" vtArea size=%d\n",	vtArea.size());


		 PlanAreaPath(bakmap,curPose,vtArea, vtPath);

		 Pose lastpos;
		 vector<Pose> vtBorderPath;

		printf(" vtPath size=%d\n",	vtPath.size());
		
		if(vtPath.size()>0)
		{
			lastpos = vtPath.at(vtPath.size()-1);
			PlanBorderPath(cleanmap,lastpos,vtBorderPath);

			printf(" vtBorderPath size=%d\n",	vtBorderPath.size());
		}

		


		for(int  i= 0;i<vtPath.size();i++)
		{
			vector<double> dot;

			dot.push_back(vtPath.at(i).x);
			dot.push_back(vtPath.at(i).y);
            dot.push_back(1);
			
			path.push_back(dot);

		}
		for(int  i= 0;i<vtBorderPath.size();i++)
		{
			vector<double> dot;

			dot.push_back(vtBorderPath.at(i).x);
			dot.push_back(vtBorderPath.at(i).y);
			dot.push_back(-1);
			path.push_back(dot);

		}
		

	if(verticalmap.height<verticalmap.width)
	{

			 for(int k=0;k<path.size();k++)
			 {
				 //TRACE("K=%d,  x=%f, y=%f\n",k, path.at(k).x,path.at(k).y);
				 vector<double> dot;
				 dot = path.at(k);
				 if(dot.size()>=2)
				{
				 double x = dot.at(0);
				 double y = dot.at(1);

				 path.at(k).at(0) = y;
				 path.at(k).at(1) = -x;

				}

			 }
	}

	
		
	if(verticalmap.data != NULL)
	{	
		delete [] verticalmap.data;
		verticalmap.data = NULL;
	}
	if(horizontalmap.data != NULL)
	{	
		delete [] horizontalmap.data;
		horizontalmap.data = NULL;
	}
	if(cleanmap.data != NULL)
	{	
		delete [] cleanmap.data;
		cleanmap.data = NULL;
	}		

		//RegionCoverage(cleanmap,curPose,path);


		/*for(int i=0;i<path.size();i++)
		{
			
			printf("hh   path.x =%f,path.y=%f\n",path.at(i).x,path.at(i).y);

		}*/
		
		return 0;
	}

