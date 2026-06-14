
#include "Astarplanner.h"
#include "../NaviInterface.h"

CAstar::CAstar()

{
    m_RobotCur.x = 0;
    m_RobotCur.y = 0;
    m_RobotCur.theta = 0;

    m_gblGrid = NULL;
    m_gblInfo = NULL;
    m_pGridState = NULL;
    m_plaserGridState = NULL;
    m_pvisionGridState = NULL;
    GGridSize = 0.1;
    m_expandGridNum = 0;
}

CAstar::~CAstar() {
    if (m_gblInfo != NULL) {
        free(m_gblInfo);
        m_gblInfo = NULL;
    }
    if (m_gblGrid != NULL) {
        free(m_gblGrid);
        m_gblGrid = NULL;
    }
    if (m_pGridState != NULL) {
        free(m_pGridState);
        m_pGridState = NULL;
    }
    if (m_plaserGridState != NULL) {
        free(m_plaserGridState);
        m_plaserGridState = NULL;
    }
    if (m_pvisionGridState != NULL) {
        free(m_pvisionGridState);
        m_pvisionGridState = NULL;
    }
}

int CAstar::mallocspace(GridMap &map) {
    m_plaserGridState =
        (GridState *)malloc(sizeof(GridState) * map.width * map.height);
    return 0;
}

int CAstar::mallocvisionspace(GridMap &map) {
    m_pvisionGridState =
        (GridState *)malloc(sizeof(GridState) * map.width * map.height);
    return 0;
}

int CAstar::initLaserMap(GridMap &map) {
    // printf("initlasermap \n");

    for (int i = 0; i < map.width * map.height; i++) {
        m_plaserGridState[i].CurrentState = FreeSpace; // FreeSpace = 0
        m_plaserGridState[i].LastState = FreeSpace;
    }
    // int i,j;

    int obstaclearea = m_expandGridNum * m_expandGridNum + 4;
    for (int i = 0; i < map.width * map.height; i++) {
        switch (map.data[i]) {
        case 0xFF: {
            m_plaserGridState[i].CurrentState = Occupied; //=2
            m_plaserGridState[i].LastState = Occupied;
            int tmpx = (int)(i % map.width);
            int tmpy = (int)(i / map.width);

            // int obstaclearea = m_expandGridNum*m_expandGridNum + 4;
            // obstaclearea = 2*(m_expandGridNum/1.41 + 1)*(m_expandGridNum/1.41
            // + 1)+2;
            for (int k = tmpx - (m_expandGridNum);
                 k <= tmpx + (m_expandGridNum);
                 k++) // fix me:extend area correct?
            {
                for (int l = tmpy - (m_expandGridNum);
                     l <= tmpy + (m_expandGridNum); l++) {
                    if (k >= 0 && k < map.width && l >= 0 && l < map.height) {

                        if (k != tmpx || l != tmpy) {
                            int iix = 0;
                            int iiy = 0;
                            iix = k - tmpx;
                            iiy = l - tmpy;
                            int idis = 0;
                            idis = iix * iix + iiy * iiy;
                            if (obstaclearea >= idis) {

                                if (m_plaserGridState[l * map.width + k]
                                        .CurrentState != Occupied) {
                                    m_plaserGridState[l * map.width + k].CurrentState = Near_Obstacle;
                                }
                            }
                        }
                    }
                }
            }

        } break;
        case 0:
        default:

            break;
        }
    }

    int dangerarea = 0;
    dangerarea = (int)(0.16 / map.metersPerPixel);
    for (int ix = 0; ix < map.width; ix++) {
        for (int iy = 0; iy < map.height; iy++) {
            if (Near_Obstacle ==
                m_plaserGridState[iy * map.width + ix].CurrentState) {
                for (int k = ix - dangerarea; k <= ix + dangerarea;
                     k++) // fix me:extend area correct?
                {
                    for (int l = iy - dangerarea; l <= iy + dangerarea; l++) {
                        if (k >= 0 && k < map.width && l >= 0 &&
                            l < map.height) {
                            // TODO
                            //  printf("set danger1");
                            //////////////
                            // m_pGridState[k][l]
                            if (m_plaserGridState[l * map.width + k]
                                    .CurrentState == FreeSpace) {
                                // m_plaserGridState[l*map.width+k].CurrentState = danger;
                            }
                        }
                    }
                }
            }
        }
    }
#if 0
	int neardangerarea = 0;
	neardangerarea = (int)(0.06/map.metersPerPixel);
	for(int ix = 0;ix < map.width; ix++)
	{
		for(int iy = 0;iy < map.height; iy++)
		{	
			if( danger == m_plaserGridState[iy*map.width+ix].CurrentState )
			{
				for (int k = ix-neardangerarea;k<=ix+neardangerarea;k++)//fix me:extend area correct?
				{
					for (int l = iy-neardangerarea;l<=iy+neardangerarea;l++)
					{
						if (k>=0 && k<map.width && l>=0 && l<map.height)  
						{
							
										//m_pGridState[k][l]
							if (m_plaserGridState[l*map.width+k].CurrentState == FreeSpace)
							{
								m_plaserGridState[l*map.width+k].CurrentState = neardanger; 
							}
							
						}
					}
				}
			}
		}
		
	}
#endif
    int bufferarea = 0;
    bufferarea = (int)(0.11 / map.metersPerPixel);
    for (int ix = 0; ix < map.width; ix++) {
        for (int iy = 0; iy < map.height; iy++) {
            if (danger == m_plaserGridState[iy * map.width + ix].CurrentState) {
                for (int k = ix - bufferarea; k <= ix + bufferarea;
                     k++) // fix me:extend area correct?
                {
                    for (int l = iy - bufferarea; l <= iy + bufferarea; l++) {
                        if (k >= 0 && k < map.width && l >= 0 &&
                            l < map.height) {

                            // m_pGridState[k][l]
                            if (m_plaserGridState[l * map.width + k]
                                    .CurrentState == FreeSpace) {
                                m_plaserGridState[l * map.width + k]
                                    .CurrentState = heighcost;
                            }
                        }
                    }
                }
            }
        }
    }

    // printf("width = %d , height = %d\n",map.width,map.height);

    return 0;
}

int CAstar::initVisionMap(GridMap &map) {
    for (int i = 0; i < map.width * map.height; i++) {
        m_pvisionGridState[i].CurrentState = FreeSpace; // FreeSpace = 0
        m_pvisionGridState[i].LastState = FreeSpace;
    }
    // int i,j;

    // int obstaclearea = m_expandGridNum*m_expandGridNum + 4;
    int obstaclearea = 8;
    for (int i = 0; i < map.width * map.height; i++) {
        switch (map.data[i]) {
        case 0xFF: {
            m_pvisionGridState[i].CurrentState = Occupied; //=2
            m_pvisionGridState[i].LastState = Occupied;
            int tmpx = (int)(i % map.width);
            int tmpy = (int)(i / map.width);

            // int obstaclearea = m_expandGridNum*m_expandGridNum + 4;
            // obstaclearea = 2*(m_expandGridNum/1.41 + 1)*(m_expandGridNum/1.41
            // + 1)+2;
            for (int k = tmpx - (m_expandGridNum);
                 k <= tmpx + (m_expandGridNum);
                 k++) // fix me:extend area correct?
            {
                for (int l = tmpy - (m_expandGridNum);
                     l <= tmpy + (m_expandGridNum); l++) {
                    if (k >= 0 && k < map.width && l >= 0 && l < map.height) {

                        if (k != tmpx || l != tmpy) {
                            int iix = 0;
                            int iiy = 0;
                            iix = k - tmpx;
                            iiy = l - tmpy;
                            int idis = 0;
                            idis = iix * iix + iiy * iiy;
                            if (obstaclearea >= idis) {

                                if (m_pvisionGridState[l * map.width + k]
                                        .CurrentState != Occupied) {
                                    m_pvisionGridState[l * map.width + k].CurrentState = Near_Obstacle;
                                }
                            }
                        }
                    }
                }
            }

        } break;
        case 0:
        default:
            // m_plaserGridState[i].CurrentState = FreeSpace ;  //FreeSpace = 0
            // m_plaserGridState[i].LastState = FreeSpace ;
            break;
        }
    }
#if 0
	int dangerarea = 0;
	dangerarea = (int)(0.06/map.metersPerPixel);
	for(int ix = 0;ix < map.width; ix++)
	{
		for(int iy = 0;iy < map.height; iy++)
		{	
			if( Near_Obstacle == m_pvisionGridState[iy*map.width+ix].CurrentState )
			{
				for (int k = ix-dangerarea;k<=ix+dangerarea;k++)//fix me:extend area correct?
				{
					for (int l = iy-dangerarea;l<=iy+dangerarea;l++)
					{
						if (k>=0 && k<map.width && l>=0 && l<map.height)  //�ڵ�ͼ��Χ��
						{
							
										//m_pGridState[k][l]
							if (m_pvisionGridState[l*map.width+k].CurrentState == FreeSpace)//�ڷ�Χ�ڣ�����û����λ
							{
								m_pvisionGridState[l*map.width+k].CurrentState = danger; //���ϰ��︽�������������˰뾶�ľ��� =15
							}
							
						}
					}
				}
			}
		}
		
	}	
	
	int neardangerarea = 0;
	neardangerarea = (int)(0.06/map.metersPerPixel);
	for(int ix = 0;ix < map.width; ix++)
	{
		for(int iy = 0;iy < map.height; iy++)
		{	
			if( danger == m_pvisionGridState[iy*map.width+ix].CurrentState )
			{
				for (int k = ix-neardangerarea;k<=ix+neardangerarea;k++)//fix me:extend area correct?
				{
					for (int l = iy-neardangerarea;l<=iy+neardangerarea;l++)
					{
						if (k>=0 && k<map.width && l>=0 && l<map.height)  //�ڵ�ͼ��Χ��
						{
							
										//m_pGridState[k][l]
							if (m_pvisionGridState[l*map.width+k].CurrentState == FreeSpace)//�ڷ�Χ�ڣ�����û����λ
							{
								m_pvisionGridState[l*map.width+k].CurrentState = neardanger; //���ϰ��︽�������������˰뾶�ľ��� =10
							}
							
						}
					}
				}
			}
		}
		
	}	

	int bufferarea = 0;
	bufferarea = (int)(0.10/map.metersPerPixel);
	for(int ix = 0;ix < map.width; ix++)
	{
		for(int iy = 0;iy < map.height; iy++)
		{	
			if( neardanger == m_pvisionGridState[iy*map.width+ix].CurrentState )
			{
				for (int k = ix-bufferarea;k<=ix+bufferarea;k++)//fix me:extend area correct?
				{
					for (int l = iy-bufferarea;l<=iy+bufferarea;l++)
					{
						if (k>=0 && k<map.width && l>=0 && l<map.height)  //�ڵ�ͼ��Χ��
						{
							
										//m_pGridState[k][l]
							if (m_pvisionGridState[l*map.width+k].CurrentState == FreeSpace)//�ڷ�Χ�ڣ�����û����λ
							{
								m_pvisionGridState[l*map.width+k].CurrentState = heighcost; //���ϰ��︽�������������˰뾶�ľ��� =5
							}
							
						}
					}
				}
			}
		}
		
	}
#endif

#if 1
    int bufferarea = 0;
    bufferarea = (int)(0.16 / map.metersPerPixel);
    for (int ix = 0; ix < map.width; ix++) {
        for (int iy = 0; iy < map.height; iy++) {
            if (Near_Obstacle ==
                m_pvisionGridState[iy * map.width + ix].CurrentState) {
                for (int k = ix - bufferarea; k <= ix + bufferarea;
                     k++) // fix me:extend area correct?
                {
                    for (int l = iy - bufferarea; l <= iy + bufferarea; l++) {
                        if (k >= 0 && k < map.width && l >= 0 &&
                            l < map.height) {

                            // m_pGridState[k][l]
                            if (m_pvisionGridState[l * map.width + k]
                                    .CurrentState == FreeSpace) {
                                m_pvisionGridState[l * map.width + k]
                                    .CurrentState = heighcost;
                            }
                        }
                    }
                }
            }
        }
    }
#endif
    // printf("width = %d , height = %d\n",map.width,map.height);

    return 0;
}

void CAstar::set_GGridSize(double m) { GGridSize = m; }

void CAstar::SetStartPose(Pose tmpPose) {
    m_RobotCur.x = tmpPose.x;
    m_RobotCur.y = tmpPose.y;
    m_RobotCur.theta = tmpPose.theta;
}

void CAstar::SetEndPose(Pose tmpPose) {
    m_RobotEnd.x = tmpPose.x;
    m_RobotEnd.y = tmpPose.y;

    m_subgoal.x = (int)(m_RobotEnd.x);
    m_subgoal.y = (int)(m_RobotEnd.y);
}
void CAstar::SetConfig(double radiusRobot) {
    m_expandGridNum = radiusRobot / GGridSize + 1;
}

// from real world to matrix grid
IPoint CAstar::GlobalToGrid(double x, double y) // IPoint : int x, int y ;
{
    IPoint Grid;
    Grid.x = (int)((x - x0) / GGridSize);   // x0：地图左上角的 x 坐标
    Grid.y = (int)((y - y0) / GGridSize);   // y0：地图左上角的 y 坐标
    return Grid;
}

// from real world to matrix grid
Pose CAstar::GridToGlobal(int i , int j)
{
        Pose p;
        p.x = i*GGridSize + x0;
        p.y = j*GGridSize + y0;
        return p ;
}

int CAstar::GridInMapOrNot(int x, int y) //�Ƿ�Խ��
{
    int ret;
    ret = (x >= 0) && (y >= 0) && (x < GMapWidth) && (y < GMapLength);
    if (0 == ret) {
        return 0;
    }
    return 1;
}

int CAstar::PositionInMapOrNot(double x, double y) //�Ƿ�Խ��
{
    int ret;
    ret = (x >= x0 && x < x0 + GMapWidth * GGridSize && y >= y0 &&
           y < y0 + GMapLength * GGridSize);
    if (0 == ret) {
        return 0;
    }
    return 1;
}

void CAstar::FreePlanner() {
    if (m_gblInfo != NULL) {
        free(m_gblInfo);
        m_gblInfo = NULL;
    }
    if (m_gblGrid != NULL) {
        free(m_gblGrid);
        m_gblGrid = NULL;
    }
    if (m_pGridState != NULL) {
        free(m_pGridState);
        m_pGridState = NULL;
    }
    if (m_plaserGridState != NULL) {
        free(m_plaserGridState);
        m_plaserGridState = NULL;
    }
    if (m_pvisionGridState != NULL) {
        free(m_pvisionGridState);
        m_pvisionGridState = NULL;
    }
}

void CAstar::PlannerInitial(ProbMap &map) {

    m_ReachTraverseGoal = 0;

    GMapWidth = map.width;
    GMapLength = map.height;

    GGridSize = map.metersPerPixel;

    x0 = map.x0;
    y0 = map.y0;

    m_gblGrid = (Node *)malloc(sizeof(Node) * map.width * map.height);
    m_gblInfo = (NodeInfo *)malloc(sizeof(NodeInfo) * map.width *
                                   map.height); // NodeInfo : int x, int y;
    m_pGridState = (GridState *)malloc(
        sizeof(GridState) * map.width *
        map.height); // GridState : int LastState, int CurrentState;

    InitializeCellTotal(map);
}

void CAstar::InitializeCellTotal(ProbMap &map) {
    int i, j;
    // initialize each grid cell
    for (i = 0; i < GMapLength * GMapWidth; i++) {
        m_gblGrid[i].nodeInfo = &(m_gblInfo[i]); // m_gblGrid[i].nodeInfo
        m_gblGrid[i].next = NULL;
        m_gblGrid[i].prev = NULL;
        m_gblGrid[i].parent = NULL;
        m_gblGrid[i].state = NEW;
        m_gblGrid[i].id = i;
        m_gblGrid[i].h = 1e+7;
        m_gblGrid[i].f = 1e+7;
        m_gblGrid[i].k = 1e+7;
        m_gblInfo[i].x = i % GMapWidth;
        m_gblInfo[i].y = i / GMapWidth;
        // m_pGridState[m_gblInfo[i].x][m_gblInfo[i].y].CurrentState = FreeSpace
        // ; m_pGridState[m_gblInfo[i].x][m_gblInfo[i].y].LastState = FreeSpace
        // ;

        m_pGridState[i].CurrentState = FreeSpace; // FreeSpace = 0
        m_pGridState[i].LastState = FreeSpace;
    }

    int obstaclearea = m_expandGridNum * m_expandGridNum + 4;
    for (i = 0; i < GMapWidth; i++) {
        for (j = 0; j < GMapLength; j++) {

            // 			m_pGridState[i][j].CurrentState = FreeSpace ;
            // 			m_pGridState[i][j].LastState = FreeSpace ;
            if (map.data[j * GMapWidth + i] <= 0.5) {
                // 				if
                // (m_pGridState[i][j].CurrentState!=Occupied
                // ||m_pGridState[i][j].CurrentState!=Near_Obstacle)
                // 				{
                // 					m_pGridState[i][j].CurrentState
                // =
                // FreeSpace ;
                // m_pGridState[i][j].LastState = FreeSpace ;
                // 				}
            }

            // if (map.data[j*GMapWidth+i] == 0xFF || map.data[j*GMapWidth+i] ==
            // 0x02)//-1,2
            if (map.data[j * GMapWidth + i] > 0.5) {

                m_pGridState[j * GMapWidth + i].CurrentState =
                    Occupied; //ռ�� =2
                m_pGridState[j * GMapWidth + i].LastState = Occupied;

                // robot  width

                // int obstaclearea = m_expandGridNum*m_expandGridNum + 4;
                // obstaclearea = 2*(m_expandGridNum/1.41 +
                // 1)*(m_expandGridNum/1.41 + 1)+2;
                for (int k = i - m_expandGridNum; k <= i + m_expandGridNum;
                     k++) // fix me:extend area correct?
                {
                    for (int l = j - m_expandGridNum; l <= j + m_expandGridNum;
                         l++) {
                        if (k >= 0 && k < GMapWidth && l >= 0 &&
                            l < GMapLength) {
                            if (k == i && l == j) {
                            } else {
                                int iix = 0;
                                int iiy = 0;
                                iix = k - i;
                                iiy = l - j;
                                int idis = 0;
                                idis = iix * iix + iiy * iiy;
                                if (obstaclearea >= idis) {

                                    // m_pGridState[k][l]
                                    if (m_pGridState[l * GMapWidth + k]
                                            .CurrentState != Occupied) {
                                        m_pGridState[l * GMapWidth + k].CurrentState = Near_Obstacle;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    int dangerarea = 0;
    dangerarea = (int)(0.16 / map.metersPerPixel);
    for (int ix = 0; ix < GMapWidth; ix++) {
        for (int iy = 0; iy < GMapLength; iy++) {
            if (Near_Obstacle ==
                m_pGridState[iy * GMapWidth + ix].CurrentState) {
                for (int k = ix - dangerarea; k <= ix + dangerarea;
                     k++) // fix me:extend area correct?
                {
                    for (int l = iy - dangerarea; l <= iy + dangerarea; l++) {
                        if (k >= 0 && k < GMapWidth && l >= 0 &&
                            l < GMapLength) {

                            // TODO setdanger2 这里先把danger去掉，后续在考虑
                            //  printf("set danger2");
                            //////////////
                            // m_pGridState[k][l]
                            if (m_pGridState[l * GMapWidth + k].CurrentState ==
                                FreeSpace) {
                                // m_pGridState[l*GMapWidth+k].CurrentState = danger;
                            }
                        }
                    }
                }
            }
        }
    }
#if 0
    int neardangerarea = 0;
	neardangerarea = (int)(0.06/map.metersPerPixel);
	for(int ix = 0;ix < GMapWidth; ix++)
	{
		for(int iy = 0;iy < GMapLength; iy++)
		{	
			if( danger == m_pGridState[iy*GMapWidth+ix].CurrentState )
			{
				for (int k = ix-neardangerarea;k<=ix+neardangerarea;k++)//fix me:extend area correct?
				{
					for (int l = iy-neardangerarea;l<=iy+neardangerarea;l++)
					{
						if (k>=0 && k<GMapWidth && l>=0 && l<GMapLength)  
						{
							
				
								//m_pGridState[k][l]
							if (m_pGridState[l*GMapWidth+k].CurrentState == FreeSpace)
							{
								m_pGridState[l*GMapWidth+k].CurrentState = neardanger; 
							}
							
						}
					}
				}
			}
		}
		
	}
#endif
    int bufferarea = 0;
    bufferarea = (int)(0.11 / map.metersPerPixel);
    for (int ix = 0; ix < GMapWidth; ix++) {
        for (int iy = 0; iy < GMapLength; iy++) {
            if (danger == m_pGridState[iy * GMapWidth + ix].CurrentState) {
                for (int k = ix - bufferarea; k <= ix + bufferarea;
                     k++) // fix me:extend area correct?
                {
                    for (int l = iy - bufferarea; l <= iy + bufferarea; l++) {
                        if (k >= 0 && k < GMapWidth && l >= 0 &&
                            l < GMapLength) {

                            // m_pGridState[k][l]
                            if (m_pGridState[l * GMapWidth + k].CurrentState ==
                                FreeSpace) {
                                m_pGridState[l * GMapWidth + k].CurrentState =
                                    heighcost;
                            }
                        }
                    }
                }
            }
        }
    }
#if 0	
	ofstream outFile;
 
	outFile.open("/data/test/testmap_planmap.txt",ios::out);

	if (!outFile)
	{	
		printf("epenerror\n");
		//return false;
	}

	outFile<<20;
	outFile<<' ';
	outFile<<0.05;
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

	for (int j=0;j<GMapLength;j++)	
	{
		for (int i=0;i<GMapWidth;i++)
		{
			int status =0;
			status = m_pGridState[j*GMapWidth+i].CurrentState;
			if (status != 0)
			{
				outFile<<-1;
			}
			else
			{
				outFile<<0;
			}
			
			outFile<<' ';
		}
		outFile<<endl;
	}

	outFile.close();
#endif
#if 1
    if (NAVI_ShouldDumpPlanDebugMaps()) {
        ofstream outFile;

        outFile.open("/data/test/pathplanmap.txt", ios::out);

        if (!outFile) {
            printf("epenerror\n");
            // return false;
        }

        outFile << 20;
        outFile << ' ';
        outFile << 0.05;
        outFile << ' ';
        outFile << map.height;
        outFile << ' ';
        outFile << map.width;
        outFile << ' ';
        outFile << map.metersPerPixel;
        outFile << ' ';
        outFile << map.x0;
        outFile << ' ';
        outFile << map.y0;
        outFile << endl;

        for (int j = 0; j < map.height; j++) {
            for (int i = 0; i < map.width; i++) {
                double status = 0;
                status = map.data[j * map.width + i];
                if (status > 0.5) {
                    outFile << -1;
                } else {
                    outFile << 0;
                }

                outFile << ' ';
            }
            outFile << endl;
        }

        outFile.close();
    }
#endif
}

/* GlobalPlanner                                                    */

int CAstar::GlobalPlanner(vector<NodeInfo> &vtPath, GridMap &map,
                          GridMap &visionmap, int type) // NodeInfo : int x, int y;
{

    NodeInfo *ni = NULL;
    Node *path;
    int step = 0;
    Node *p = NULL;
    Node *currentnode = NULL;
    Node *endnode = NULL;
    // currentnode = GetNode((int)m_RobotCur.x,(int)m_RobotCur.y);
    // //GetNode:Global to Grid _RPT2(_CRT_WARN,"m_RobotEnd.x= %f,m_RobotEnd.y=
    //%f\n",m_RobotEnd.x,m_RobotEnd.y); endnode =
    // GetNode((int)m_RobotEnd.x,(int)m_RobotEnd.y);

    double dis = (m_RobotEnd.x - m_RobotCur.x) * (m_RobotEnd.x - m_RobotCur.x) +
                 (m_RobotEnd.y - m_RobotCur.y) * (m_RobotEnd.y - m_RobotCur.y);

    dis = sqrt(dis);

    if (dis < 0.250) {
        return 1;
    }

    {

        InitializeCellRelation(); // state = new
        InitializeStartPoint(m_RobotCur.x, m_RobotCur.y, m_RobotCur.theta);
        InitializeGoalPoint(m_RobotEnd.x, m_RobotEnd.y, 0.0);

        switch (type)
        {
        case 1:
            cout << "Begin DStarSearch!" << endl;
            path = DStarSearch(m_initial, m_numInitial, m_costR, map, visionmap);
            break;
        case 2:
            cout << "Begin AStarSearch!" << endl;
            path = AStarSearch(m_initial, m_numInitial, m_costR, map, visionmap);
            break;
        case 3:
            cout << "Begin BFSSearch!" << endl;
            path = BFSSearch(m_initial, m_numInitial, m_costR, map, visionmap);
            break;
        }

        if (path != NULL) {

            p = path;
            while (p != NULL) {
                step++;
                ni = (NodeInfo *)p->nodeInfo;
                NodeInfo pathInfo;
                pathInfo.x = ni->x;
                pathInfo.y = ni->y;
                vtPath.push_back(pathInfo);

                // printf("step %d is x = %d, y=%d\n",step,ni->x,ni->y);

                p = (Node *)p->parent;
                // 				if (step==1)//
                // 				{
                // 					m_subgoal.x=ni->x*200+100;
                // 					m_subgoal.y=ni->y*200+100;
                // 				}

                //_RPT2(_CRT_WARN,"m_subgoal.x= %d,
                // m_subgoal.y=%d\n",m_subgoal.x,m_subgoal.y);
            }
        } else {

            return -1;
        }
    }

    return 0;
}

Node *CAstar::GetNode(int x, int y) {
    IPoint robot;
    Node *p = NULL;
    robot = GlobalToGrid(x, y);
    p = (Node *)&(m_gblGrid[robot.y * GMapWidth + robot.x]);
    return p;
}

void CAstar::InitializeCellRelation() {
    int i;
    for (i = 0; i < GMapLength * GMapWidth; i++) {
        m_gblGrid[i].next = NULL;
        m_gblGrid[i].prev = NULL;
        m_gblGrid[i].parent = NULL;
        m_gblGrid[i].state = NEW;
        m_gblGrid[i].h = 1e+7;
        m_gblGrid[i].f = 1e+7;
        m_gblGrid[i].k = 1e+7;
        m_gblGrid[i].id = i;
    }
}

int CAstar::InitializeStartPoint(double x, double y, double theta) {
    int posx, posy;
    Node *CurRobot;
    NodeInfo *ni;
    if (PositionInMapOrNot(x, y)) {
        posx = (int)((x - x0) / GGridSize);
        posy = (int)((y - y0) / GGridSize);

        // printf("posx = %d,posy= %d\n",	posx,posy);

        m_gblRobot[0] = posx;
        m_gblRobot[1] = posy;

        // m_gblGrid  m_gblInfo
        CurRobot = &(m_gblGrid[m_gblRobot[1] * GMapWidth +
                               m_gblRobot[0]]); // CurRobot m_gblGrid
        CurRobot->nodeInfo =
            &(m_gblInfo[m_gblRobot[1] * GMapWidth + m_gblRobot[0]]); //ָm_gblInfo
        CurRobot->next = NULL;
        CurRobot->prev = NULL;
        CurRobot->parent = NULL;
        CurRobot->state = NEW;
        CurRobot->id = 0;
        CurRobot->h = 1e+7;
        CurRobot->f = 1e+7;

        ni = (NodeInfo *)CurRobot->nodeInfo;

        ni->x = m_gblRobot[0];
        ni->y = m_gblRobot[1];
        //_RPT2(_CRT_WARN,"ni->x = %d, ni->y=%d\n",ni->x,ni->y);
    } else {
        //		_RPT0(_CRT_WARN,"Warning!");
        return 0;
    }
    return 1;
}

int CAstar::InitializeGoalPoint(double x, double y, double theta) {
    int posx, posy;
    Node *root;
    if (PositionInMapOrNot(x, y)) {
        posx = (int)((x - x0) / GGridSize);
        posy = (int)((y - y0) / GGridSize);

        m_gblGoal[0] = posx;
        m_gblGoal[1] = posy;

        root = &(m_gblGrid[m_gblGoal[1] * GMapWidth + m_gblGoal[0]]);
        root->g = 0;
        root->h = hfunction(root);
        root->f = root->g + root->h;
        // put it in the initial array
        m_initial[0] = root;
        m_numInitial = 1;
        m_costR[0] = m_costR[1] = 1e+7; // this is of vital importance
        return 1;
    }

    else {
        m_ReachTraverseGoal = 1;
    }
    return 0;
}

double CAstar::hfunction(Node *p) {
    NodeInfo *ni;
    double h, dx, dy;

    if (p == NULL)
        return (1e+7);

    ni = (NodeInfo *)p->nodeInfo;

    // Uncomment this to use Euclidean distance
    dx = m_gblRobot[0] - ni->x;
    dy = m_gblRobot[1] - ni->y;
    // h = sqrt(dx * dx + dy * dy);
    h = sqrt(dx * dx) + sqrt(dy * dy);

    return (h);
}

Node *CAstar::insertOPEN(Node *openList, Node *newnode) {
    Node *p;
    Node *q;

    if (newnode->state == NEW) // NEW=0
    {
        newnode->state = OPEN;
    } else { // node is already in the openlist

        // node is on the open list, so delete it and re-insert it below
        p = (Node *)newnode->prev;
        q = (Node *)newnode->next;
        if (p != NULL)
            p->next = newnode->next;
        if (q != NULL)
            q->prev = newnode->prev;

        // check for the case of the node being at the head of the list
        if (newnode == openList)
            openList = (Node *)newnode->next;

        newnode->next = NULL;
        newnode->prev = NULL;
    }

    // now insert the state into the openList

    // Test the case where openList is NULL
    if (openList == NULL) {
        newnode->next = openList;
        newnode->prev = NULL;
        return (newnode);
    }

    // Test the case where the new node is at the head of the list
    if ((newnode->f < openList->f) || (newnode->f == openList->f)) {
        newnode->next = openList;
        if (openList != NULL) {
            openList->prev = newnode;
        }
        newnode->prev = NULL;
        return (newnode);
    }

    // start the loop through the OPEN list
    p = openList; // p : just the starting position of the openlist
    q = (Node *)p->next;
    while (p != NULL) {

        if (q == NULL) // end of the list, insert after p
        {

            p->next = newnode;
            newnode->next = NULL;
            newnode->prev = p;
            return (openList);
        }

        if (newnode->f < q->f || newnode->f == q->f) {
            // insert the node before p and after q
            newnode->next = q;
            if (q != NULL) {
                q->prev = newnode;
            }
            p->next = newnode;
            newnode->prev = p;

            return (openList);
        }

        p = (Node *)p->next;
        q = (Node *)p->next;
    }

    return (openList);
}

int CAstar::robot(Node *p) {
    NodeInfo *ni;

    ni = (NodeInfo *)p->nodeInfo;

    //_RPT4(_CRT_WARN,"ni->x = %d,m_gblRobot[0]=%d, ni->y=%d,
    // m_gblRobot[1]=%d\n",ni->x,m_gblRobot[0],ni->y,m_gblRobot[1]); if(ni->x ==
    // m_gblRobot[0] && ni->y == m_gblRobot[1])
    // if(m_gblGrid[m_gblRobot[1]*GMapWidth+m_gblRobot[0] ].state == OPEN)
    if (ni->x == m_gblRobot[0] && ni->y == m_gblRobot[1]) {
        return (1);
    } else {
        return (0);
    }
}

int CAstar::getNeighbors(Node *parent, Node **neighbor, GridMap &map,
                         GridMap &visionmap) {
    NodeInfo *ni;
    int i, posx, posy;
    int deltax[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    int deltay[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    int numNeighbors;

    ni = (NodeInfo *)parent->nodeInfo;

    // build all of the legal children
    numNeighbors = 0;
    for (i = 0; i < 8; i++) {
        // calculate the potential position of the next child
        posx = ni->x + deltax[i];
        posy = ni->y + deltay[i];
        // bounds check
        if (posx >= 0 && posx < GMapWidth && posy >= 0 && posy < GMapLength) {
            if (m_pGridState[posy * GMapWidth + posx].CurrentState ==
                    Occupied ||
                m_pGridState[posy * GMapWidth + posx].CurrentState ==
                    Near_Obstacle ||
                m_pGridState[posy * GMapWidth + posx].CurrentState == danger ||
                m_pGridState[posy * GMapWidth + posx].CurrentState ==
                    neardanger ||
                m_gblGrid[posy * GMapWidth + posx].state == CLOSED) {
                continue;
            }
            int lx = (int)((map.x0 - x0) / 0.05);
            int ly = (int)((map.y0 - y0) / 0.05);

            lx = posx - lx;
            ly = posy - ly;

            int vx = (int)((visionmap.x0 - x0) / 0.05);
            int vy = (int)((visionmap.y0 - y0) / 0.05);

            vx = posx - vx;
            vy = posy - vy;

            if (lx >= 0 && lx < map.width && ly >= 0 && ly < map.height) {
                if (m_plaserGridState[ly * map.width + lx].CurrentState ==
                        Occupied ||
                    m_plaserGridState[ly * map.width + lx].CurrentState ==
                        Near_Obstacle ||
                    m_plaserGridState[ly * map.width + lx].CurrentState ==
                        danger ||
                    m_plaserGridState[ly * map.width + lx].CurrentState ==
                        neardanger) {
                    continue;
                }
            }

            if (vx >= 0 && vx < visionmap.width && vy >= 0 &&
                vy < visionmap.height) {
                if (m_pvisionGridState[vy * visionmap.width + vx]
                            .CurrentState == Occupied ||
                    m_pvisionGridState[vy * visionmap.width + vx]
                            .CurrentState == Near_Obstacle) {
                    continue;
                }
            }

            neighbor[numNeighbors++] = &(m_gblGrid[posy * GMapWidth + posx]);
        }
    }
    return (numNeighbors);
}

double CAstar::cost(Node *to, Node *from, GridMap &map, GridMap &visionmap) {
    double dx, dy;

    dx = ((NodeInfo *)to->nodeInfo)->x - ((NodeInfo *)from->nodeInfo)->x;
    dy = ((NodeInfo *)to->nodeInfo)->y - ((NodeInfo *)from->nodeInfo)->y;

    int glbx = 0;
    int glby = 0;
    glbx = ((NodeInfo *)to->nodeInfo)->x;
    glby = ((NodeInfo *)to->nodeInfo)->y;

    if (inObstacle(((NodeInfo *)to->nodeInfo)->x,
                   ((NodeInfo *)to->nodeInfo)->y)) {

        return (1e+7);
    } else {

        int x = (map.x0 - x0) / 0.05;
        int y = (map.y0 - y0) / 0.05;

        int ix = ((NodeInfo *)to->nodeInfo)->x - x;
        int iy = ((NodeInfo *)to->nodeInfo)->y - y;

        int vx = (visionmap.x0 - x0) / 0.05;
        int vy = (visionmap.y0 - y0) / 0.05;

        int vix = ((NodeInfo *)to->nodeInfo)->x - vx;
        int viy = ((NodeInfo *)to->nodeInfo)->y - vy;

        if (ix >= 0 && ix < map.width && iy >= 0 && iy < map.height) {
            if (m_plaserGridState[iy * map.width + ix].CurrentState ==
                Occupied) {
                return (1e+7);
            }
            if (m_plaserGridState[iy * map.width + ix].CurrentState ==
                Near_Obstacle) {
                return (1e+7);
            }

            if (vix >= 0 && vix < visionmap.width && viy >= 0 &&
                viy < visionmap.height) {
                if (m_pvisionGridState[viy * visionmap.width + vix]
                        .CurrentState == Occupied) {
                    return (1e+7);
                }
                if (m_pvisionGridState[viy * visionmap.width + vix]
                        .CurrentState == Near_Obstacle) {
                    return (1e+7);
                }
            }

            if (m_plaserGridState[iy * map.width + ix].CurrentState == danger) {
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    danger) {
                    return 50.0;
                }
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    neardanger) {
                    return 50.0;
                }
                return 50.0;
            }

            if (vix >= 0 && vix < visionmap.width && viy >= 0 &&
                viy < visionmap.height) {
                if (m_pvisionGridState[viy * visionmap.width + vix]
                        .CurrentState == danger) {
                    return 50.0;
                }
            }

            if (m_plaserGridState[iy * map.width + ix].CurrentState ==
                neardanger) {
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    danger) {
                    return 50.0;
                }
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    neardanger) {
                    return 30.0;
                }
                return 30.0;
            }

            if (vix >= 0 && vix < visionmap.width && viy >= 0 &&
                viy < visionmap.height) {
                if (m_pvisionGridState[viy * visionmap.width + vix]
                        .CurrentState == neardanger) {
                    return 30.0;
                }
            }

            ////////////////////////////////////////
            if (m_plaserGridState[iy * map.width + ix].CurrentState ==
                heighcost) {

                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    danger) {
                    return 50.0;
                }
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    neardanger) {
                    return 30.0;
                }
                return (15.0 + sqrt(dx * dx + dy * dy));
            }

            if (vix >= 0 && vix < visionmap.width && viy >= 0 &&
                viy < visionmap.height) {
                if (m_pvisionGridState[viy * visionmap.width + vix]
                        .CurrentState == heighcost) {
                    return (15.0 + sqrt(dx * dx + dy * dy));
                }
            }

            if (m_plaserGridState[iy * map.width + ix].CurrentState ==
                FreeSpace) {
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    danger) {
                    return 50.0;
                }
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    neardanger) {
                    return 30.0;
                }
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    heighcost) {
                    return (15.0 + sqrt(dx * dx + dy * dy));
                } else {
                    return (sqrt(dx * dx + dy * dy));
                }
            }
        }

        if (vix >= 0 && vix < visionmap.width && viy >= 0 &&
            viy < visionmap.height) {

            if (m_pvisionGridState[viy * visionmap.width + vix].CurrentState ==
                Occupied) {
                return (1e+7);
            }
            if (m_pvisionGridState[viy * visionmap.width + vix].CurrentState ==
                Near_Obstacle) {
                return (1e+7);
            }
            if (m_pvisionGridState[viy * visionmap.width + vix].CurrentState ==
                danger) {
                return 50.0;
            }
            if (m_pvisionGridState[viy * visionmap.width + vix].CurrentState ==
                neardanger) {

                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    danger) {
                    return 50.0;
                }
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    neardanger) {
                    return 30.0;
                }
                return 30.0;
            }
            if (m_pvisionGridState[viy * visionmap.width + vix].CurrentState ==
                heighcost) {
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    danger) {
                    return 50.0;
                }
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    neardanger) {
                    return 30.0;
                }
                return (15.0 + sqrt(dx * dx + dy * dy));
            }
            if (m_pvisionGridState[viy * map.width + vix].CurrentState ==
                FreeSpace) {
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    danger) {
                    return 50.0;
                }
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    neardanger) {
                    return 30.0;
                }
                if (m_pGridState[glby * GMapWidth + glbx].CurrentState ==
                    heighcost) {
                    return (15.0 + sqrt(dx * dx + dy * dy));
                } else {
                    return (sqrt(dx * dx + dy * dy));
                }
            }
        }

        if (m_pGridState[glby * GMapWidth + glbx].CurrentState == danger) {
            return 50.0;
        }
        if (m_pGridState[glby * GMapWidth + glbx].CurrentState == neardanger) {
            return 30.0;
        }

        if (m_pGridState[glby * GMapWidth + glbx].CurrentState == heighcost) {
            return (15.0 + sqrt(dx * dx + dy * dy));
        }

        return (sqrt(dx * dx + dy * dy));
    }
}

int CAstar::inObstacle(int x, int y) {
    if (m_pGridState[y * GMapWidth + x].CurrentState == Occupied) {
        return (1);
    }

    if (m_pGridState[y * GMapWidth + x].CurrentState == Near_Obstacle) {
        return (1);
    }

    if (m_pGridState[y * GMapWidth + x].CurrentState == danger) {
        return (1);
    }

    if (m_pGridState[y * GMapWidth + x].CurrentState == neardanger) {
        return (1);
    }
    return (0);
}

void CAstar::freeNode(Node *p) {
    if (p != NULL) {
        NodeInfo *ni;
        ni = (NodeInfo *)p->nodeInfo;
        ni->x = ni->y = -1;
        p->parent = NULL;
        p->next = NULL;
        p->state = NEW;
        p = NULL; // new add line
    }
}

int CAstar::ComputeSubGoal(Pose pose) {
    IPoint robot;
    Node *p, *p_next;
    NodeInfo *index, *index_next;
    int i = 0;
    robot.x = (int)pose.x;
    robot.y = (int)pose.y;
    robot = GlobalToGrid(robot.x, robot.y);

    p = (Node *)&(m_gblGrid[robot.y * GMapWidth + robot.x]);
    index = (NodeInfo *)p->nodeInfo;

    if ((Node *)p->parent != NULL) {
        for (i = 0; i < 1; i++) {
            if ((Node *)p->parent != NULL) {

                p_next = (Node *)p->parent;
                index_next = (NodeInfo *)p_next->nodeInfo;
                if (index_next->x == m_gblGoal[0] &&
                    index_next->y == m_gblGoal[1]) {
                    m_subgoal.x = (int)(m_RobotEnd.x);
                    m_subgoal.y = (int)(m_RobotEnd.y);
                    m_Magnitude = 24;
                    return 1;
                }
                p = p_next;
            }
        }

        index = (NodeInfo *)p->nodeInfo;

        if (GridInMapOrNot(index->x, index->y)) {

            // m_subgoal = GridCenterToGlobal(index->x,index->y );
            //			Magnitude = Magnitude_backup ;
            ////once more
            m_Magnitude = 20; // once more
        }
        return 1;
    } else if (index->x == robot.x && index->y == robot.y) {
        index = (NodeInfo *)p->nodeInfo;
        if (index->x == m_gblGoal[0] && index->y == m_gblGoal[1]) {

            m_subgoal.x = (int)(m_RobotEnd.x);
            m_subgoal.y = (int)(m_RobotEnd.y);
            return 1;
        } else {
            // ReportError("ComputeSubGoal Error1!\n");
            return 0;
        }
    } else {
        return 0;
    }
}

bool CAstar::plan(Pose thegoal, ProbMap &visMap, Pose curpose,
                  vector<vector<double>> &path, GridMap &lasermap,
                  GridMap &visionMap, int type) {
    vector<NodeInfo> vtPlanPath;

    SetStartPose(curpose); // m_robotCur
    SetEndPose(thegoal);   // m_RobotEnd

    // PlannerInitial(visMap);

    int ret = GlobalPlanner(vtPlanPath, lasermap, visionMap, type);

    if (ret == -1)
    {
        return false;
    }

    if (!vtPlanPath.empty()) {

        int num = vtPlanPath.size();

        for (int i = 0; i < num; i = i + 1) {
            NodeInfo node = vtPlanPath.at(i);

            vector<double> subGoal;
            subGoal.push_back(node.x * GGridSize + (double)GGridSize / 2 + x0);
            subGoal.push_back(node.y * GGridSize + (double)GGridSize / 2 + y0);

            // printf("step  is x = %f, y=%f\n",node.x * GGridSize +
            // (double)GGridSize/2+x0,node.y * GGridSize +
            // (double)GGridSize/2+y0);

            path.push_back(subGoal);
        }
    }
    return true;
}

int CAstar::PlanPixel(Pose thegoal, ProbMap &visMap, Pose curpose,
                      vector<NodeInfo> &vtPlanPath) {

    SetStartPose(curpose);
    SetEndPose(thegoal);

    PlannerInitial(visMap);
}
