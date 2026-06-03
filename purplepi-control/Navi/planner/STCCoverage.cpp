#include "STCCoverage.h"

STCCoverage::STCCoverage()
{

}

STCCoverage::~STCCoverage()
{

}

void STCCoverage::generateSTCPath(int* coverageGridMap, int gridMapLength, int gridMapWidth, IPoint startIPoint) {
    int startX = startIPoint.x;
    int startY = startIPoint.y;

    // 检查起点是否为空闲区域
    if (coverageGridMap[startY * gridMapWidth + startX] != 0) {
        cout << "The starting point is not an idle area!" << endl;
        return;
    }
    else {
        cout << "The starting point is an empty area!" << endl;
    }

    // 创建一个访问标记矩阵，标记哪些区域已经被访问
    cout << "Creating access token matrix..." << endl;
    std::vector<std::vector<bool>> visited(gridMapLength, std::vector<bool>(gridMapWidth, false));
    cout << "Access tag matrix created successfully!" << endl;
    cout << "gridMapLength is " << gridMapLength << ", gridMapWidth is " << gridMapWidth << endl;
    
    // 生成树的队列，存储树的节点
    std::queue<IPoint> treeQueue;
    treeQueue.push(startIPoint);
    cout << "startY is " << startY << ", startX is " << startX << endl;
    visited[startY][startX] = true;

    // 四个方向的偏移量：上、右、下、左
    int dx[4] = {0, 1, 0, -1};
    int dy[4] = {1, 0, -1, 0};

    // 生成树过程：BFS遍历
    cout << "Starting BFS to build the spanning tree..." << endl;
    while (!treeQueue.empty()) {
        IPoint current = treeQueue.front();
        treeQueue.pop();
        
        // 将当前节点加入路径并标记为已访问
        path.push_back(current);
        // 这里通过 visited 来标记已访问
        visited[current.y][current.x] = true;  // 标记该点为已访问
        cout << "Add the " << "(" << current.y << ", " << current.x << ") node to the path and mark it as visited" << endl;

        // 遍历当前节点的四个邻居
        for (int i = 0; i < 4; ++i) {
            cout << "Traverse the neighbors of the current node..." << endl;

            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            // 检查邻居是否合法且未访问过
            if (nx >= 0 && nx < gridMapWidth && ny >= 0 && ny < gridMapLength &&
                coverageGridMap[ny * gridMapWidth + nx] == 0 && // 邻居必须是空闲区域
                !visited[ny][nx]) { // 邻居没有被访问过
                
                // 记录当前节点的邻居
                adjList[current].push_back({nx, ny});
                treeQueue.push({nx, ny});
                visited[ny][nx] = true;  // 标记该邻居为已访问
            }
        }
    }
}

void STCCoverage::traverseTreeDFS(
    IPoint currentNode,
    std::unordered_map<IPoint, std::vector<IPoint>, PointHash, PointEqual>& adjList,
    std::unordered_map<IPoint, bool, PointHash, PointEqual>& visited)
{
    visited[currentNode] = true;

    // 前进：加入路径
    goPath.push_back(currentNode);
    std::cout << "Go ahead: (" << currentNode.x << ", " << currentNode.y << ")" << std::endl;

    for (const IPoint& neighbor : adjList[currentNode]) {
        if (!visited[neighbor]) {
            traverseTreeDFS(neighbor, adjList, visited);  // 递归访问

            // 回溯：加入路径
            goPath.push_back(currentNode);
            std::cout << "Back to: (" << currentNode.x << ", " << currentNode.y << ")" << std::endl;
        }
    }
}

void STCCoverage::startDFSTraversal() {
    std::unordered_map<IPoint, bool, PointHash, PointEqual> visited;
    
    // 假设生成树的根节点是path[0]
    if (!path.empty()) {
        traverseTreeDFS(path[0], adjList, visited);
    }
}