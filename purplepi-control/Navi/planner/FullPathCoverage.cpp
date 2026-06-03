#include "FullPathCoverage.h"
#include "../NaviInterface.h"

// 计算叉积，判断三点是否构成左转（正值）或右转（负值）或直线（零）
int Graham::crossProduct(const IPoint& p, const IPoint& q, const IPoint& r) {
    return (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x);
}

// 比较函数，用于按极角排序
bool Graham::compare(const IPoint& p1, const IPoint& p2) {
    return p1.x < p2.x || (p1.x == p2.x && p1.y < p2.y);
}

// Graham扫描算法，返回凸包的点集
std::vector<IPoint> Graham::grahamScan(std::vector<IPoint>& points) {
    // 如果点集少于3个，无法构成凸包
    if (points.size() < 3) return points;

    // 找到最底部且最左的点作为起点
    IPoint p0 = *std::min_element(points.begin(), points.end(), compare);
    
    // 排序，按极角排序
    std::sort(points.begin(), points.end(), [this, p0](const IPoint& p1, const IPoint& p2) {
        int cp = crossProduct(p0, p1, p2);
        if (cp == 0) {
            return (p1.x - p0.x) * (p1.x - p0.x) + (p1.y - p0.y) * (p1.y - p0.y) < (p2.x - p0.x) * (p2.x - p0.x) + (p2.y - p0.y) * (p2.y - p0.y);
        }
        return cp > 0;
    });

    // 准备栈，初始化栈，将起点和第一个点加入
    std::vector<IPoint> stack;
    stack.push_back(p0);
    stack.push_back(points[0]);

    // 遍历排序后的点，构建凸包
    for (size_t i = 1; i < points.size(); ++i) {
        while (stack.size() >= 2 && crossProduct(stack[stack.size() - 2], stack.back(), points[i]) <= 0) {
            stack.pop_back(); // 若不是左转则出栈
        }
        stack.push_back(points[i]);
    }

    return stack;
}

FullCoverageAlg::FullCoverageAlg()
{
    algNum = -1;
    coverageGridMap = NULL;
    coverageState = NULL;
    isFinished = false;
    for (int i = 0; i < 8; i++)
    {
        roomVertex[i] = 0;
    }
}

FullCoverageAlg::~FullCoverageAlg()
{

}

vector<IPoint> FullCoverageAlg::getObstaclePoints(int GMapLength, int GMapWidth, GridState *m_pGridState) {
    vector<IPoint> obstaclePoints;

    // 遍历每个网格单元，检查是否是障碍物
    for (int y = 0; y < GMapLength; y++) {
        for (int x = 0; x < GMapWidth; x++) {
            // 获取当前网格单元的状态
            int index = y * GMapWidth + x;
            if (m_pGridState[index].CurrentState == Occupied || 
                m_pGridState[index].CurrentState == Near_Obstacle || 
                m_pGridState[index].CurrentState == danger || 
                m_pGridState[index].CurrentState == neardanger) {
                // 如果是障碍物，将其坐标加入障碍物点集
                IPoint point = {x, y};
                obstaclePoints.push_back(point);
            }
        }
    }
    return obstaclePoints;
}

vector<IPoint> FullCoverageAlg::getObstaclePointsFromMultipleMaps(int GMapLength, int GMapWidth, GridState *m_pGridState, GridState *m_plaserGridState, GridState *m_pvisionGridState) {
    vector<IPoint> obstaclePoints;

    // 遍历每个网格单元，检查是否是障碍物
    for (int y = 0; y < GMapLength; y++) {
        for (int x = 0; x < GMapWidth; x++) {
            int index = y * GMapWidth + x;

            // 检查不同地图的障碍物状态
            if (m_pGridState[index].CurrentState == Occupied || 
                m_pGridState[index].CurrentState == Near_Obstacle ||
                m_pvisionGridState[index].CurrentState == Occupied ||
                m_pvisionGridState[index].CurrentState == Near_Obstacle ||
                m_plaserGridState[index].CurrentState == Occupied ||
                m_plaserGridState[index].CurrentState == Near_Obstacle) {
                
                // 如果是障碍物，将其坐标加入障碍物点集
                IPoint point = {x, y};
                obstaclePoints.push_back(point);
            }
        }
    }
    return obstaclePoints;
}

vector<IPoint> FullCoverageAlg::autoGetRoomBoundary(
    int GMapLength,
    int GMapWidth,
    GridState* m_pGridState,
    bool &initFlag
) {
    // ************************ 注意：此函数未经调试，请先使用手动输入法！！！ ************************
    vector<IPoint> obstaclePoints;
    vector<IPoint> convexHull;
    obstaclePoints = getObstaclePoints(
        GMapLength,
        GMapWidth,
        m_pGridState
    );
    if (obstaclePoints.size() != 0)
    {
        printf("Obstacle scan success!\n");
        // 获取凸包
        convexHull = graham.grahamScan(obstaclePoints);
        if (convexHull.size() != 0)
        {
            printf("Convex hull built success!\n");
            // 使用printf输出凸包点集
            /*
            printf("Convex Hull Points: \n");
            for (const IPoint& p : convexHull) {
                Pose convexPose = pObject->astarPlanner.GridToGlobal(p.x, p.y);
                printf("(%lf, %lf)\n", convexPose.x, convexPose.y);
            }
            */
        }
        else
        {
            printf("Convex hull built failed!\n");
            initFlag = false;
        }
    }
    else
    {
        printf("Obstacle scan failed!\n");
        initFlag = false;
    }

    return convexHull;
}
/*
vector<IPoint> FullCoverageAlg::manualGetRoomBoundary(CAstar& astarPlanner, bool& initFlag) {
    int cnt = 0;
    vector<IPoint> convexHull;
    IPoint manualIPoint;
    double px = 0.0, py = 0.0;
    
    // 注意：在输入房间顶点时，必须按照 Pad 端平板显示房间的左下角、右下角、右上角、左上角的顺序输入！！！
    // 注意：如果输入顺序错误，将导致无法正确识别某一点是否处于房间内！！！
    cout << "Please enter the vertex coordinates (Pose) of the four corners of the room:" << endl;
    cout << "(Input order: lower left corner, lower right corner, upper right corner, upper left corner)" << endl;
    
    for (cnt = 0; cnt < 4; cnt++) {
        cout << "Start input for corner " << cnt + 1 << "..." << endl;

        while (true) {
            cout << "Enter x coordinate for corner " << cnt + 1 << ": ";
            if (!(cin >> px)) {
                cout << "Invalid input. Please enter a valid number." << endl;
                cin.clear();  // 清除错误状态
                cin.ignore(numeric_limits<streamsize>::max(), '\n');  // 忽略输入缓冲区的错误数据
                continue;
            }

            cout << "Enter y coordinate for corner " << cnt + 1 << ": ";
            if (!(cin >> py)) {
                cout << "Invalid input. Please enter a valid number." << endl;
                cin.clear();  // 清除错误状态
                cin.ignore(numeric_limits<streamsize>::max(), '\n');  // 忽略输入缓冲区的错误数据
                continue;
            }
            
            // 输入有效，跳出循环
            break;
        }

        // 输出输入的坐标
        cout << "You entered: " << px << " " << py << endl;

        // 将全局坐标转换为网格坐标
        px = 1.0 * px / 20;
        py = 1.0 * py / 20;
        manualIPoint = astarPlanner.GlobalToGrid(px, py);
        cout << "Converted to grid: " << manualIPoint.x << " " << manualIPoint.y << endl;
        
        convexHull.push_back(manualIPoint);
    }

    cout << "Manual get room boundary success!" << endl;
    initFlag = true;

    return convexHull;
}
*/

vector<IPoint> FullCoverageAlg::manualGetRoomBoundary(CAstar& astarPlanner, bool& initFlag) {
    int cnt = 0;
    vector<IPoint> convexHull;
    IPoint manualIPoint;
    double px = 0.0, py = 0.0;
    
    // 注意：在输入房间顶点时，必须按照 Pad 端平板显示房间的左下角、右下角、右上角、左上角的顺序输入！！！
    // 注意：如果输入顺序错误，将导致无法正确识别某一点是否处于房间内！！！
    cout << "Please enter the vertex coordinates (Pose) of the four corners of the room:" << endl;
    cout << "(Input order: lower left corner, lower right corner, upper right corner, upper left corner)" << endl;
    
    for (cnt = 0; cnt < 4; cnt++) {
        cout << "Start input for corner " << cnt + 1 << "..." << endl;

        px = roomVertex[cnt * 2];
        py = roomVertex[cnt * 2 + 1];

        // 输出输入的坐标
        cout << "You entered: " << px << " " << py << endl;

        // 将全局坐标转换为网格坐标
        // px = 1.0 * px / 20;
        // py = 1.0 * py / 20;
        manualIPoint = astarPlanner.GlobalToGrid(px, py);
        cout << "Converted to grid: " << manualIPoint.x << " " << manualIPoint.y << endl;
        
        convexHull.push_back(manualIPoint);
    }

    cout << "Manual get room boundary success!" << endl;
    initFlag = true;

    return convexHull;
}

// 计算向量的叉积（用于判断三角形的方向）
int FullCoverageAlg::crossProduct(const IPoint& p1, const IPoint& p2, const IPoint& p3) {
    return (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
}

// 判断点 p 是否在多边形 polygon 内部
bool FullCoverageAlg::isPointInsideRoom(const std::vector<IPoint>& polygon, const IPoint& p) {
    int n = polygon.size();
    bool inside = true;

    for (int i = 0; i < n; i++) {
        // 计算向量叉积，判断 p 点与每个边的方向
        int cp = crossProduct(polygon[i], polygon[(i + 1) % n], p);

        // 如果 p 点与某条边的方向相反，说明点在多边形外部
        if (cp < 0) {
            inside = false;
            break;
        }
    }

    return inside;
}

bool FullCoverageAlg::initCoverageState(int GMapLength, int GMapWidth, GridState* m_pGridState, vector<IPoint> convexHull, bool &initFlag)
{
    printf("Initializing coverage map ...\n");
    coverageState = (int*)malloc(sizeof(int) * GMapLength * GMapWidth);
    if (coverageState == NULL)
    {
        // 处理 malloc 失败的情况
        printf("Memory allocation failed!\n");
        initFlag = false;
    }
    int i = 0;
    IPoint currentIPoint;
    cout << "Convex hull is :" << endl;
    for (int k = 0; k < convexHull.size(); k++)
    {
        cout << convexHull.at(k).x << " " << convexHull.at(k).y << endl;
    }
    for (i = 0; i < GMapLength * GMapWidth; i++)
    {
        currentIPoint.y = i / GMapWidth;
        currentIPoint.x = i % GMapWidth;
        if (isPointInsideRoom(convexHull, currentIPoint))
        {
            if (m_pGridState[i].CurrentState == Occupied)
            {
                coverageState[i] = OBSTACLE;
            }
            else if (
                m_pGridState[i].CurrentState == Near_Obstacle ||
                m_pGridState[i].CurrentState == danger ||
                m_pGridState[i].CurrentState == neardanger
            ) {
                coverageState[i] = WARNINGAREA;
            }
            else
            {
                coverageState[i] = UNCOVERAGE_BUT_IN_ROOM;
            }
        }
        else
        {
            coverageState[i] = OUTOF_ROOM;
        }
        // 由于墙壁可能会被误识别为房间外的点，因此此处再次处理墙壁数据
        if (m_pGridState[i].CurrentState == Occupied)
        {
            coverageState[i] = OBSTACLE;
        }
    }

    printf("Coverage map initialized successfully!\n");
    return true;
}

void FullCoverageAlg::setValue(int GMapLength, int GMapWidth, IPoint currentIPoint)
{
    // 获取当前坐标
    int x = currentIPoint.x;
    int y = currentIPoint.y;

    // 假设 coverageState 是一个 2D 数组或对应存储位置的数组
    // 设置前后左右方向的矩形范围
    for (int i = -10; i <= 10; i++) {
        for (int j = -10; j <= 10; j++) {
            // 设置向前、向后、向左、向右的矩形区域
            // 例如向前是 x, y+方向 以及向后是 x, y-方向
            // 仅更新这些坐标范围内的 coverageState
            if (coverageState[(y + j) * GMapWidth + (x + i)] == UNCOVERAGE_BUT_IN_ROOM)
            {
                coverageState[(y + j) * GMapWidth + (x + i)] = COVERGAED_AND_IN_ROOM;
            }
        }
    }

    printf("Updated coverage map!\n");
}

// 函数：将覆盖状态从一维数组转换为二维地图，并将其写入txt文件，同时写入导航路径坐标序列
void FullCoverageAlg::writeCoverageMapToFile(int GMapLength, int GMapWidth, const std::string& filename) {
    // 打开文件
    std::ofstream outFile(filename);

    if (!outFile) {
        std::cerr << "Error opening file!" << std::endl;
        return;
    }

    // 写入地图的长宽（第一行）
    outFile << GMapLength << " " << GMapWidth << std::endl;

    // 将队列中的点直接输出到第二行
    while (!pathIPoint.empty()) {
        IPoint point = pathIPoint.front();
        pathIPoint.pop();  // 移除队列头部的点
        outFile << "(" << point.x << ", " << point.y << ") ";
    }
    outFile << std::endl;  // 换行

    // 将一维数组 coverageState 转换为二维数组形式并写入文件
    for (int y = 0; y < GMapLength; y++) {
        for (int x = 0; x < GMapWidth; x++) {
            int index = y * GMapWidth + x;  // 计算一维数组索引
            outFile << coverageState[index] << " ";  // 写入该位置的覆盖状态
        }
        outFile << std::endl;  // 每一行结束换行
    }

    outFile.close();  // 关闭文件
}

bool FullCoverageAlg::findNextGoal_Radius(IPoint curIPoint, CAstar &astarPlanner) {
    // 定义最大搜索半径（这里假设搜索一个固定范围的点，避免对整个地图遍历）
    const int maxSearchRadius = 200;

    // 遍历机器人当前位置附近的点
    for (int radius = 1; radius <= maxSearchRadius; ++radius) {
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dy = -radius; dy <= radius; ++dy) {
                // 计算新位置的坐标
                int nx = curIPoint.x + dx;
                int ny = curIPoint.y + dy;

                // 确保坐标在地图范围内
                if (nx >= 0 && ny >= 0 && nx < astarPlanner.GMapWidth && ny < astarPlanner.GMapLength) {
                    // 映射到一维数组的索引
                    int index = ny * astarPlanner.GMapWidth + nx;
                    
                    // 检查该位置的状态是否符合要求
                    if (coverageState[index] == UNCOVERAGE_BUT_IN_ROOM) {
                        // 定义检查的范围：向前、向后、向左、向右各10个单位
                        bool valid = true;
                        for (int dx = -10; dx <= 10; ++dx) {  // 水平方向：左右各10个单位
                            for (int dy = -10; dy <= 10; ++dy) {  // 垂直方向：上下各10个单位
                                // 确保检查点在地图范围内
                                int nx2 = nx + dx;
                                int ny2 = ny + dy;
                                if (nx2 >= 0 && ny2 >= 0 && nx2 < astarPlanner.GMapWidth && ny2 < astarPlanner.GMapLength) {
                                    int neighborIndex = ny2 * astarPlanner.GMapWidth + nx2;
                                    
                                    // 如果某个点是障碍物（2）或房间外（3），标记为无效
                                    if (coverageState[neighborIndex] == OBSTACLE || coverageState[neighborIndex] == OUTOF_ROOM) {
                                        valid = false;
                                        break;
                                    }
                                }
                            }
                            if (!valid) break;  // 如果发现无效点，立即退出检查
                        }

                        // 如果周围区域有效，返回当前点作为目标
                        if (valid) {
                            cout << "Discover a new uncovered area!" << endl;
                            isFinished = false;
                            nextGoal = astarPlanner.GridToGlobal(nx, ny);
                            cout << "Successfully returned to the target point!" << endl;
                            return true;
                        }
                    }
                }
            }
        }
    }

    cout << "No uncovered areas found!" << endl;
    isFinished = true;
    return false;  // 如果没有找到目标点，返回 false 表示未找到
}

bool FullCoverageAlg::findNextGoal_BFS(IPoint curIPoint, CAstar &astarPlanner) {
    cout << "Checking if there are any remaining areas that are not covered ..." << endl;

    // 定义四个方向（上下左右）
    const int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    // 创建一个队列来进行广度优先搜索
    queue<IPoint> q;

    // 使用 vector 替代动态分配二维数组
    vector<vector<bool>> visited(astarPlanner.GMapLength, vector<bool>(astarPlanner.GMapWidth, false));

    // 将起始点（机器人当前位置）加入队列
    q.push(curIPoint);
    visited[curIPoint.y][curIPoint.x] = true;

    // 广度优先搜索（BFS）
    while (!q.empty()) {
        IPoint current = q.front();
        q.pop();

        // 如果当前点的状态是未覆盖且在房间内，且周围的 20 x 20 区域没有障碍物或房间外的点
        int index = current.y * astarPlanner.GMapWidth + current.x;
        if (coverageState[index] == UNCOVERAGE_BUT_IN_ROOM) {
            // 检查周围 20 x 20 正方形区域（上下左右各 10 个单位）
            bool valid = true;
            for (int dx = -10; dx <= 10; ++dx) {  // 左右各 10 个单位
                for (int dy = -10; dy <= 10; ++dy) {  // 上下各 10 个单位
                    int nx = current.x + dx;
                    int ny = current.y + dy;
                    
                    // 确保该点在地图范围内
                    if (nx >= 0 && ny >= 0 && nx < astarPlanner.GMapWidth && ny < astarPlanner.GMapLength) {
                        int neighborIndex = ny * astarPlanner.GMapWidth + nx;
                        
                        // 如果周围点是障碍物或房间外，标记为无效
                        if (coverageState[neighborIndex] == OBSTACLE || coverageState[neighborIndex] == OUTOF_ROOM) {
                            valid = false;
                            break;
                        }
                    }
                }
                if (!valid) break;  // 如果发现无效点，立即退出检查
            }

            // 如果周围区域有效，返回当前点作为目标
            if (valid) {
                cout << "Discover a new uncovered area!" << endl;
                isFinished = false;
                nextGoal = astarPlanner.GridToGlobal(current.x, current.y);
                cout << "Successfully returned to the target point!" << endl;
                return true;
            }
        }

        // 扩展相邻节点
        for (int i = 0; i < 4; ++i) {
            int nx = current.x + directions[i][0];
            int ny = current.y + directions[i][1];

            // 确保新位置在地图范围内且未访问过
            if (nx >= 0 && ny >= 0 && nx < astarPlanner.GMapWidth && ny < astarPlanner.GMapLength && !visited[ny][nx]) {
                // 检查新位置是否为有效的目标点
                int newIndex = ny * astarPlanner.GMapWidth + nx;
                if (coverageState[newIndex] != OBSTACLE && coverageState[newIndex] != OUTOF_ROOM) {
                    // 将新位置加入队列
                    q.push({nx, ny});
                    visited[ny][nx] = true;
                }
            }
        }
    }

    // 没有找到目标点
    cout << "No uncovered areas found!" << endl;
    isFinished = true;
    return false;
}

bool FullCoverageAlg::findNextGoal_BFS_LimitDepth(IPoint curIPoint, CAstar &astarPlanner, int maxDepth) {
    cout << "Checking if there are any remaining areas that are not covered ..." << endl;

    // 定义四个方向（下、左、上、右），优先向下和向左扩展
    const int directions[4][2] = {{0, 1}, {-1, 0}, {0, -1}, {1, 0}};  // 优先检查“下”和“左”方向

    // 创建一个队列来进行广度优先搜索
    queue<pair<IPoint, int>> q;  // 队列存储的是点和当前深度

    // 使用 vector 替代动态分配二维数组
    vector<vector<bool>> visited(astarPlanner.GMapLength, vector<bool>(astarPlanner.GMapWidth, false));

    // 将起始点（机器人当前位置）加入队列，初始深度为0
    q.push({curIPoint, 0});
    visited[curIPoint.y][curIPoint.x] = true;

    // 记录最接近左下角的符合条件的目标点
    IPoint bestGoal = {-1, -1};  // 初始化为空点
    int bestDistance = INT_MAX;  // 初始化为一个很大的值

    // 左下角的坐标
    int lowerLeftX = 0;
    int lowerLeftY = astarPlanner.GMapLength - 1;

    // 广度优先搜索（BFS）
    while (!q.empty()) {
        auto current = q.front();
        q.pop();
        IPoint curPos = current.first;
        int depth = current.second;

        // 如果当前深度超过最大深度，停止搜索
        if (depth > maxDepth) {
            break;
        }

        // 如果当前点的状态是未覆盖且在房间内，且周围的 20 x 20 区域没有障碍物或房间外的点
        int index = curPos.y * astarPlanner.GMapWidth + curPos.x;
        if (coverageState[index] == UNCOVERAGE_BUT_IN_ROOM) {
            // 检查周围 20 x 20 正方形区域（上下左右各 10 个单位）
            bool valid = true;
            for (int dx = -10; dx <= 10; ++dx) {  // 左右各 10 个单位
                for (int dy = -10; dy <= 10; ++dy) {  // 上下各 10 个单位
                    int nx = curPos.x + dx;
                    int ny = curPos.y + dy;
                    
                    // 确保该点在地图范围内
                    if (nx >= 0 && ny >= 0 && nx < astarPlanner.GMapWidth && ny < astarPlanner.GMapLength) {
                        int neighborIndex = ny * astarPlanner.GMapWidth + nx;
                        
                        // 如果周围点是障碍物或房间外，标记为无效
                        if (coverageState[neighborIndex] == OBSTACLE || coverageState[neighborIndex] == OUTOF_ROOM) {
                            valid = false;
                            break;
                        }
                    }
                }
                if (!valid) break;  // 如果发现无效点，立即退出检查
            }

            // 如果周围区域有效，计算距离左下角的曼哈顿距离
            if (valid) {
                int distance = abs(curPos.x - lowerLeftX) + abs(curPos.y - lowerLeftY);
                
                // 如果当前点更接近左下角，更新最优目标点
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestGoal = curPos;
                }
            }
        }

        // 扩展相邻节点
        for (int i = 0; i < 4; ++i) {
            int nx = curPos.x + directions[i][0];
            int ny = curPos.y + directions[i][1];

            // 确保新位置在地图范围内且未访问过
            if (nx >= 0 && ny >= 0 && nx < astarPlanner.GMapWidth && ny < astarPlanner.GMapLength && !visited[ny][nx]) {
                // 检查新位置是否为有效的目标点
                int newIndex = ny * astarPlanner.GMapWidth + nx;
                if (coverageState[newIndex] != OBSTACLE && coverageState[newIndex] != OUTOF_ROOM) {
                    // 将新位置加入队列，并将深度加1
                    q.push({{nx, ny}, depth + 1});
                    visited[ny][nx] = true;
                }
            }
        }
    }

    // 如果找到了最接近左下角的目标点
    if (bestGoal.x != -1 && bestGoal.y != -1) {
        cout << "Found the most suitable goal point!" << endl;
        isFinished = false;
        nextGoal = astarPlanner.GridToGlobal(bestGoal.x, bestGoal.y);
        cout << "Successfully returned to the target point!" << endl;
        return true;
    }

    // 没有找到目标点
    cout << "No uncovered areas found!" << endl;
    isFinished = true;
    return false;
}

bool FullCoverageAlg::findNextGoal_BFS_LimitDistance(IPoint curIPoint, CAstar &astarPlanner, int maxDistance) {
    cout << "Checking if there are any remaining areas that are not covered ..." << endl;

    // 定义四个方向（下、左、上、右），优先向下和向左扩展
    const int directions[4][2] = {{0, 1}, {-1, 0}, {0, -1}, {1, 0}};  // 优先检查“下”和“左”方向

    // 创建一个队列来进行广度优先搜索
    queue<pair<IPoint, int>> q;  // 队列存储的是点和当前到起始点的距离

    // 使用 vector 替代动态分配二维数组
    vector<vector<bool>> visited(astarPlanner.GMapLength, vector<bool>(astarPlanner.GMapWidth, false));

    // 将起始点（机器人当前位置）加入队列，初始距离为0
    q.push({curIPoint, 0});
    visited[curIPoint.y][curIPoint.x] = true;

    // 记录最接近左下角的符合条件的目标点
    IPoint bestGoal = {-1, -1};  // 初始化为空点
    int bestDistance = INT_MAX;  // 初始化为一个很大的值

    // 左下角的坐标
    int lowerLeftX = 0;
    int lowerLeftY = astarPlanner.GMapLength - 1;

    // 广度优先搜索（BFS）
    while (!q.empty()) {
        auto current = q.front();
        q.pop();
        IPoint curPos = current.first;
        int distanceFromStart = current.second;

        // 如果当前点距离起始点超过最大距离，停止搜索
        if (distanceFromStart > maxDistance) {
            continue;
        }

        // 如果当前点的状态是未覆盖且在房间内，且周围的 20 x 20 区域没有障碍物或房间外的点
        int index = curPos.y * astarPlanner.GMapWidth + curPos.x;
        if (coverageState[index] == UNCOVERAGE_BUT_IN_ROOM) {
            // 检查周围 20 x 20 正方形区域（上下左右各 10 个单位）
            bool valid = true;
            for (int dx = -10; dx <= 10; ++dx) {  // 左右各 10 个单位
                for (int dy = -10; dy <= 10; ++dy) {  // 上下各 10 个单位
                    int nx = curPos.x + dx;
                    int ny = curPos.y + dy;
                    
                    // 确保该点在地图范围内
                    if (nx >= 0 && ny >= 0 && nx < astarPlanner.GMapWidth && ny < astarPlanner.GMapLength) {
                        int neighborIndex = ny * astarPlanner.GMapWidth + nx;
                        
                        // 如果周围点是障碍物或房间外，标记为无效
                        if (coverageState[neighborIndex] == OBSTACLE || coverageState[neighborIndex] == OUTOF_ROOM) {
                            valid = false;
                            break;
                        }
                    }
                }
                if (!valid) break;  // 如果发现无效点，立即退出检查
            }

            // 如果周围区域有效，计算距离左下角的曼哈顿距离
            if (valid) {
                int distance = abs(curPos.x - lowerLeftX) + abs(curPos.y - lowerLeftY);
                
                // 如果当前点更接近左下角，更新最优目标点
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestGoal = curPos;
                }
            }
        }

        // 扩展相邻节点
        for (int i = 0; i < 4; ++i) {
            int nx = curPos.x + directions[i][0];
            int ny = curPos.y + directions[i][1];

            // 确保新位置在地图范围内且未访问过
            if (nx >= 0 && ny >= 0 && nx < astarPlanner.GMapWidth && ny < astarPlanner.GMapLength && !visited[ny][nx]) {
                // 检查新位置是否为有效的目标点
                int newIndex = ny * astarPlanner.GMapWidth + nx;
                if (coverageState[newIndex] != OBSTACLE && coverageState[newIndex] != OUTOF_ROOM) {
                    // 将新位置加入队列，并计算到起始点的距离
                    q.push({{nx, ny}, distanceFromStart + 1});
                    visited[ny][nx] = true;
                }
            }
        }
    }

    // 如果找到了最接近左下角的目标点
    if (bestGoal.x != -1 && bestGoal.y != -1) {
        cout << "Found the most suitable goal point!" << endl;
        isFinished = false;
        nextGoal = astarPlanner.GridToGlobal(bestGoal.x, bestGoal.y);
        cout << "Successfully returned to the target point!" << endl;
        return true;
    }

    // 没有找到目标点
    cout << "No uncovered areas found!" << endl;
    isFinished = true;
    return false;
}

bool FullCoverageAlg::isCoveraged(Pose endpose, CAstar &astarPlanner) {
    // 定义八个方向（前、后、左、右、左上、右上、右下、左下）
    const int directions[8][2] = {
        {0, 1},   // 前
        {0, -1},  // 后
        {-1, 0},  // 左
        {1, 0},   // 右
        {-1, 1},  // 左上
        {1, 1},   // 右上
        {1, -1},  // 右下
        {-1, -1}  // 左下
    };

    // 获取endpose对应的坐标
    IPoint endIPoint = astarPlanner.GlobalToGrid(endpose.x, endpose.y);
    int x = endIPoint.x;
    int y = endIPoint.y;

    // 检查八个方向的状态
    for (int i = 0; i < 8; ++i) {
        int nx = x + directions[i][0];
        int ny = y + directions[i][1];

        // 确保新位置在地图范围内
        if (nx >= 0 && ny >= 0 && nx < astarPlanner.GMapWidth && ny < astarPlanner.GMapLength) {
            int index = ny * astarPlanner.GMapWidth + nx;

            // 如果在八个方向中有一个点状态是未覆盖且在房间内，则返回true
            if (coverageState[index] == COVERGAED_AND_IN_ROOM) {
                return true;
            }
        }
    }

    // 如果所有八个方向都没有符合条件的点，返回false
    return false;
}

void FullCoverageAlg::mapToGrid(int x, int y, int robotSize, int& gridX, int& gridY) {
    // 映射函数：将原始地图坐标映射到网格地图坐标
    gridX = x / robotSize;
    gridY = y / robotSize;
}

void FullCoverageAlg::mapToOriginal(int gridX, int gridY, int robotSize, int& x, int& y) {
    // 逆映射函数：将网格地图坐标映射回原始地图坐标
    // 计算原始地图坐标
    x = gridX * robotSize + robotSize / 2;  // 网格中心点的原始地图 x 坐标
    y = gridY * robotSize + robotSize / 2;  // 网格中心点的原始地图 y 坐标
}

void FullCoverageAlg::convertToGridMap(int GMapLength, int GMapWidth, int robotSize) {
    // 将原始地图转换为网格地图
    int gridWidth = GMapWidth / robotSize;
    int gridLength = GMapLength / robotSize;

    coverageGridMap = (int*)malloc(sizeof(int) * gridWidth * gridLength);
    if (coverageGridMap == NULL) {
        // 处理 malloc 失败的情况
        printf("Memory allocation failed!\n");
        return;
    }
    else
    {
        printf("Memory allocation success!\n");
    }
    // 初始化网格地图为全 0
    memset(coverageGridMap, 0, sizeof(int) * gridWidth * gridLength);

    for (int y = 0; y < GMapLength; y++) {
        for (int x = 0; x < GMapWidth; x++) {
            int gridX, gridY;
            mapToGrid(x, y, robotSize, gridX, gridY);  // 映射到网格

            // 计算一维索引
            int index = gridY * gridWidth + gridX;

            // 如果原始地图中的点是障碍物或房间外区域，则标记为占用
            int coverageStateIndex = y * GMapWidth + x;
            if (coverageState[coverageStateIndex] == OUTOF_ROOM ||
                coverageState[coverageStateIndex] == OBSTACLE ||
                coverageGridMap[index] == 1) {
                coverageGridMap[index] = 1;
            } else {
                coverageGridMap[index] = 0;
            }
        }
    }
}

void FullCoverageAlg::saveGridMapToFile(int gridLength, int gridWidth, const std::string& filename) {
    // 创建输出文件流对象
    std::ofstream outFile(filename);
    
    if (!outFile) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return;
    }
    
    // 在文件的第一行写入地图的长和宽
    outFile << gridLength << " " << gridWidth << std::endl;
    
    // 遍历网格地图并写入每个值
    for (int y = 0; y < gridLength; y++) {
        for (int x = 0; x < gridWidth; x++) {
            int index = y * gridWidth + x;
            // 写入每个网格的值（0或1）
            outFile << (coverageGridMap[index] == 0 ? "0 " : "1 ");
        }
        outFile << std::endl;  // 每行结束后换行
    }

    // 关闭文件流
    outFile.close();
    
    std::cout << "Grid map has been saved to file: " << filename << std::endl;
}

void FullCoverageAlg::writeGridMapToFile(int gridMapLength, int gridMapWidth, std::vector<IPoint> path, const string& filename) {
    // 打开文件进行写入
    ofstream outFile(filename);
    if (!outFile) {
        cerr << "Unable to open file!" << endl;
        return;
    }

    // 第一行：写网格地图的长和宽
    outFile << gridMapLength << " " << gridMapWidth << endl;

    // 第二行：写path的点序列
    for (size_t i = 0; i < path.size(); ++i) {
        outFile << "(" << path[i].x << ", " << path[i].y << ")";
        if (i < path.size() - 1) {
            outFile << " ";
        }
    }
    outFile << endl;

    // 从第三行开始：写coverageGridMap的信息
    for (int y = 0; y < gridMapLength; ++y) {
        for (int x = 0; x < gridMapWidth; ++x) {
            outFile << coverageGridMap[y * gridMapWidth + x] << " ";
        }
        outFile << endl;
    }

    outFile.close(); // 关闭文件
}

/*
bool FullCoverageAlg::ifIPointAroundLegal(IPoint &targetIPoint, CAstar &astarPlanner)
{
    int deltax[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    int deltay[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    cout << "Check if the targetIPoint legal ..." << endl;
    for (int i = 0; i < 8; i++) {
        int posx = targetIPoint.x + deltax[i];
        int posy = targetIPoint.y + deltay[i];

        if (posx >= 0 && posx < astarPlanner.GMapWidth && posy >= 0 && posy < astarPlanner.GMapLength) {
            if (astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == Occupied ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == Near_Obstacle ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == danger ||
                astarPlanner.m_pGridState[posy * astarPlanner.GMapWidth + posx].CurrentState == neardanger) {
                return false;
            }
        }
    }
    return true;
}
*/

bool FullCoverageAlg::ifIPointAroundLegal(IPoint &targetIPoint, CAstar &astarPlanner)
{
    cout << "Check if the 5x5 region around targetIPoint is legal ..." << endl;
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) {
            if (dx == 0 && dy == 0) continue; // 跳过中心点自身

            int posx = targetIPoint.x + dx;
            int posy = targetIPoint.y + dy;

            if (posx >= 0 && posx < astarPlanner.GMapWidth &&
                posy >= 0 && posy < astarPlanner.GMapLength) {

                int index = posy * astarPlanner.GMapWidth + posx;
                auto state = astarPlanner.m_pGridState[index].CurrentState;

                if (state == Occupied || state == Near_Obstacle || 
                    state == danger || state == neardanger || 
                    coverageState[index] == WARNINGAREA || coverageState[index] == OBSTACLE) {
                    return false;
                }
            }
        }
    }
    return true;
}
