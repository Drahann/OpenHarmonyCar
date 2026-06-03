#include "Astarplanner.h"

Node *CAstar::AStarSearch(Node** initial, int numInitial, 
                  double costR[2], 
                  GridMap& map, GridMap& visionmap) {
    Node *openList = NULL;
    Node *closedList = NULL;
    Node *current;
    Node *path;
    Node *neighbor[MAXNEIGHBORS]; // 最大邻居数
    int numNeighbors;
    long i;
    int gblExpand = 0;

    // 初始化开列表
    for (i = 0; i < numInitial; i++) {
        openList = insertOPEN(openList, initial[i]);
    }

    while (openList != NULL) {
        current = openList;
        openList = (Node *)openList->next;
        if (openList != NULL) {
            openList->prev = NULL;
        }
        gblExpand++;

        current->state = CLOSED;
        current->next = NULL;
        current->prev = NULL;

        // 检查是否已经到达目标
        if (robot(current)) {
            costR[0] = current->h + current->g; // f = g + h
            costR[1] = current->g; // 记录g值

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
                neighbor[i]->h = hfunction(neighbor[i]);
                neighbor[i]->f = neighbor[i]->g + neighbor[i]->h;
                openList = insertOPEN(openList, neighbor[i]);
                neighbor[i]->state = OPEN;
            } else {
                if (neighbor[i]->g > current->g + cost(neighbor[i], current, map, visionmap)) {
                    neighbor[i]->parent = current;
                    neighbor[i]->g = current->g + cost(neighbor[i], current, map, visionmap);
                    neighbor[i]->f = neighbor[i]->g + neighbor[i]->h;
                    openList = insertOPEN(openList, neighbor[i]);
                }
            }
        }

        if (gblExpand > MAXNODES) {
            printf("Expanded more than the maximum allowable nodes (%d). Terminating\n", gblExpand);
            return NULL;
        }
    }

    freeNode(openList);
    return NULL; // 没有路径
}
