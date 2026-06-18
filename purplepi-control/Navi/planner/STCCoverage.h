#pragma once
#include "Gridmap.h"
#include "Astarplanner.h"

#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <math.h>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <utility>

// 自定义哈希函数
struct PointHash {
    size_t operator()(const IPoint& p) const {
        // 使用x和y的哈希值组合
        return std::hash<int>()(p.x) ^ std::hash<int>()(p.y);
    }
};

// 自定义比较函数，用于unordered_map的键相等判断
struct PointEqual {
    bool operator()(const IPoint& lhs, const IPoint& rhs) const {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }
};

class STCCoverage
{
public:
    // 使用自定义的哈希和比较函数
    std::unordered_map<IPoint, std::vector<IPoint>, PointHash, PointEqual> adjList;
    // 记录最小生成树生成过程点序列
    std::vector<IPoint> path;
    // 记录深度优先遍历最小生成树过程点序列
    std::vector<IPoint> goPath;

public:
    // 构造函数
    STCCoverage();
    // 析构函数
    ~STCCoverage();

    // 构建最小生成树
    void generateSTCPath(int* coverageGridMap, int gridMapLength, int gridMapWidth, IPoint startIPoint);
    // 兼容旧接口；当前遍历入口使用显式栈，避免大地图递归栈溢出。
    void traverseTreeDFS(IPoint currentNode, std::unordered_map<IPoint, std::vector<IPoint>, PointHash, PointEqual>& adjList, std::unordered_map<IPoint, bool, PointHash, PointEqual>& visited);
    // 开始以深度优先算法遍历生成树
    void startDFSTraversal();
};
