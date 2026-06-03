#include "Astarplanner.h"

#include <queue>  // 包含std::queue

Node *CAstar::BFSSearch(Node** initial, int numInitial, 
                double costR[2], 
                GridMap& map, GridMap& visionmap) {
    std::queue<Node*> queue; // 使用std::queue来存储待扩展的节点
    Node *closedList = NULL;
    Node *current;
    Node *path;
    Node *neighbor[MAXNEIGHBORS]; // 最大邻居数
    int numNeighbors;
    long i;
    int gblExpand = 0;

    // 初始化队列，将初始节点放入队列
    for (i = 0; i < numInitial; i++) {
        queue.push(initial[i]);
    }

    while (!queue.empty()) {
        current = queue.front();  // 获取队列头部节点
        queue.pop();              // 将队列头部节点移除
        gblExpand++;

        current->state = CLOSED;  // 将当前节点标记为已扩展
        current->next = NULL;
        current->prev = NULL;

        // 检查是否已经到达目标
        if (robot(current)) {
            costR[0] = current->g; // 只有g值，因为没有启发式函数
            costR[1] = current->g; // 记录g值（即从起点到目标的代价）

            // 返回路径（可以根据需要追溯父节点）
            path = (Node *)current->parent;
            return path;
        }

        numNeighbors = getNeighbors(current, neighbor, map, visionmap);

        // 遍历邻居节点
        for (i = 0; i < numNeighbors; i++) {
            if (neighbor[i]->state == NEW) {
                neighbor[i]->parent = current;
                neighbor[i]->g = current->g + cost(neighbor[i], current, map, visionmap);
                neighbor[i]->state = OPEN;
                queue.push(neighbor[i]); // 将邻居节点加入队列
            }
        }

        // 防止无限扩展
        if (gblExpand > MAXNODES) {
            printf("Expanded more than the maximum allowable nodes (%d). Terminating\n", gblExpand);
            return NULL;
        }
    }

    return NULL; // 如果队列为空，说明没有找到路径
}
