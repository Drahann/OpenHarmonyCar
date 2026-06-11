#include "BCDCoverage.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <queue>

BCDCoverage::BCDCoverage()
    : mapWidth(0), mapHeight(0)
{
}

BCDCoverage::~BCDCoverage()
{
}

void BCDCoverage::clear()
{
    fullPath.clear();
    cellOf.clear();
    cells.clear();
    freeMask.clear();
    mapWidth = 0;
    mapHeight = 0;
}

bool BCDCoverage::isGridFree(GridState *gridState, int x, int y)
{
    int state = gridState[y * mapWidth + x].CurrentState;
    return state != Occupied &&
           state != Near_Obstacle &&
           state != danger &&
           state != neardanger;
}

bool BCDCoverage::isFreeCell(int x, int y) const
{
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        return false;
    }
    return freeMask[y * mapWidth + x] != 0;
}

bool BCDCoverage::buildFreeMask(GridState *gridState,
                                int xmin,
                                int ymin,
                                int xmax,
                                int ymax)
{
    if (gridState == NULL || mapWidth <= 0 || mapHeight <= 0) {
        return false;
    }

    freeMask.assign(mapWidth * mapHeight, 0);
    for (int y = ymin; y <= ymax; ++y) {
        for (int x = xmin; x <= xmax; ++x) {
            if (isGridFree(gridState, x, y)) {
                freeMask[y * mapWidth + x] = 1;
            }
        }
    }
    return true;
}

std::vector<BCDSlice> BCDCoverage::collectColumnSlices(int x) const
{
    std::vector<BCDSlice> slices;
    int y = 0;
    while (y < mapHeight) {
        while (y < mapHeight && !isFreeCell(x, y)) {
            ++y;
        }
        if (y >= mapHeight) {
            break;
        }

        BCDSlice slice;
        slice.top = y;
        slice.cellId = -1;
        while (y < mapHeight && isFreeCell(x, y)) {
            ++y;
        }
        slice.bottom = y - 1;
        slices.push_back(slice);
    }
    return slices;
}

bool BCDCoverage::slicesOverlap(const BCDSlice &a, const BCDSlice &b) const
{
    return a.top <= b.bottom && b.top <= a.bottom;
}

int BCDCoverage::createCell()
{
    BCDCell cell;
    cell.id = (int)cells.size();
    cells.push_back(cell);
    return cell.id;
}

void BCDCoverage::addSliceToCell(int x, const BCDSlice &slice)
{
    if (slice.cellId < 0 || slice.cellId >= (int)cells.size()) {
        return;
    }

    BCDCell &cell = cells[slice.cellId];
    if (cell.colSpan.empty()) {
        cell.xmin = x;
        cell.xmax = x;
    } else {
        cell.xmin = std::min(cell.xmin, x);
        cell.xmax = std::max(cell.xmax, x);
    }
    cell.colSpan[x] = std::make_pair(slice.top, slice.bottom);
    cell.area += slice.bottom - slice.top + 1;

    for (int y = slice.top; y <= slice.bottom; ++y) {
        cellOf[y * mapWidth + x] = slice.cellId;
    }
}

void BCDCoverage::decompose()
{
    cells.clear();
    cellOf.assign(mapWidth * mapHeight, -1);

    std::vector<BCDSlice> prevSlices;
    for (int x = 0; x < mapWidth; ++x) {
        std::vector<BCDSlice> curSlices = collectColumnSlices(x);
        std::vector<int> prevOverlapCount(prevSlices.size(), 0);
        std::vector<std::vector<int> > curOverlaps(curSlices.size());

        for (size_t i = 0; i < curSlices.size(); ++i) {
            for (size_t j = 0; j < prevSlices.size(); ++j) {
                if (slicesOverlap(curSlices[i], prevSlices[j])) {
                    curOverlaps[i].push_back((int)j);
                    prevOverlapCount[j]++;
                }
            }
        }

        for (size_t i = 0; i < curSlices.size(); ++i) {
            if (curOverlaps[i].size() == 1) {
                int prevIndex = curOverlaps[i][0];
                if (prevOverlapCount[prevIndex] == 1) {
                    curSlices[i].cellId = prevSlices[prevIndex].cellId;
                }
            }
            if (curSlices[i].cellId < 0) {
                curSlices[i].cellId = createCell();
            }
            addSliceToCell(x, curSlices[i]);
        }

        prevSlices = curSlices;
    }
}

bool BCDCoverage::nearestFreePoint(IPoint start,
                                   int xmin,
                                   int ymin,
                                   int xmax,
                                   int ymax,
                                   IPoint &result) const
{
    if (isFreeCell(start.x, start.y)) {
        result = start;
        return true;
    }

    int bestDist = std::numeric_limits<int>::max();
    bool found = false;
    for (int y = ymin; y <= ymax; ++y) {
        for (int x = xmin; x <= xmax; ++x) {
            if (!isFreeCell(x, y)) {
                continue;
            }
            IPoint p;
            p.x = x;
            p.y = y;
            int dist = pointDistance2(start, p);
            if (dist < bestDist) {
                bestDist = dist;
                result = p;
                found = true;
            }
        }
    }
    return found;
}

std::vector<IPoint> BCDCoverage::buildCellPath(const BCDCell &cell,
                                               int stride,
                                               bool reversePath) const
{
    std::vector<IPoint> path;
    if (cell.colSpan.empty()) {
        return path;
    }

    std::vector<int> columns;
    for (std::map<int, std::pair<int, int> >::const_iterator it =
             cell.colSpan.begin();
         it != cell.colSpan.end(); ++it) {
        if (columns.empty() ||
            it->first - columns.back() >= stride ||
            it->first == cell.xmax) {
            columns.push_back(it->first);
        }
    }
    if (columns.back() != cell.xmax) {
        columns.push_back(cell.xmax);
    }

    bool downward = true;
    for (size_t i = 0; i < columns.size(); ++i) {
        int x = columns[i];
        std::map<int, std::pair<int, int> >::const_iterator it =
            cell.colSpan.find(x);
        if (it == cell.colSpan.end()) {
            continue;
        }

        IPoint p1;
        IPoint p2;
        p1.x = x;
        p2.x = x;
        if (downward) {
            p1.y = it->second.first;
            p2.y = it->second.second;
        } else {
            p1.y = it->second.second;
            p2.y = it->second.first;
        }

        path.push_back(p1);
        if (p1.x != p2.x || p1.y != p2.y) {
            path.push_back(p2);
        }
        downward = !downward;
    }

    if (reversePath) {
        std::reverse(path.begin(), path.end());
    }
    return path;
}

std::vector<IPoint> BCDCoverage::bfsConnect(IPoint start, IPoint goal) const
{
    std::vector<IPoint> empty;
    if (!isFreeCell(start.x, start.y) || !isFreeCell(goal.x, goal.y)) {
        return empty;
    }
    if (start.x == goal.x && start.y == goal.y) {
        empty.push_back(start);
        return empty;
    }

    std::vector<int> parent(mapWidth * mapHeight, -1);
    std::queue<IPoint> q;
    int startIndex = start.y * mapWidth + start.x;
    int goalIndex = goal.y * mapWidth + goal.x;
    parent[startIndex] = startIndex;
    q.push(start);

    const int dx[4] = {1, 0, -1, 0};
    const int dy[4] = {0, 1, 0, -1};
    while (!q.empty()) {
        IPoint cur = q.front();
        q.pop();
        if (cur.x == goal.x && cur.y == goal.y) {
            break;
        }
        for (int i = 0; i < 4; ++i) {
            IPoint next;
            next.x = cur.x + dx[i];
            next.y = cur.y + dy[i];
            if (!isFreeCell(next.x, next.y)) {
                continue;
            }
            int nextIndex = next.y * mapWidth + next.x;
            if (parent[nextIndex] >= 0) {
                continue;
            }
            parent[nextIndex] = cur.y * mapWidth + cur.x;
            q.push(next);
        }
    }

    if (parent[goalIndex] < 0) {
        return empty;
    }

    std::vector<IPoint> path;
    int index = goalIndex;
    while (index != startIndex) {
        IPoint p;
        p.x = index % mapWidth;
        p.y = index / mapWidth;
        path.push_back(p);
        index = parent[index];
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}

int BCDCoverage::pointDistance2(IPoint a, IPoint b) const
{
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    return dx * dx + dy * dy;
}

bool BCDCoverage::buildPathFromGridState(GridState *gridState,
                                         int width,
                                         int height,
                                         int xmin,
                                         int ymin,
                                         int xmax,
                                         int ymax,
                                         IPoint start,
                                         int stride)
{
    clear();
    if (gridState == NULL || width <= 0 || height <= 0) {
        return false;
    }

    mapWidth = width;
    mapHeight = height;
    xmin = std::max(0, std::min(xmin, width - 1));
    xmax = std::max(0, std::min(xmax, width - 1));
    ymin = std::max(0, std::min(ymin, height - 1));
    ymax = std::max(0, std::min(ymax, height - 1));
    if (xmin > xmax) {
        std::swap(xmin, xmax);
    }
    if (ymin > ymax) {
        std::swap(ymin, ymax);
    }
    stride = std::max(1, stride);

    if (!buildFreeMask(gridState, xmin, ymin, xmax, ymax)) {
        return false;
    }

    IPoint current;
    if (!nearestFreePoint(start, xmin, ymin, xmax, ymax, current)) {
        return false;
    }

    decompose();
    std::vector<int> remaining;
    for (size_t i = 0; i < cells.size(); ++i) {
        if (cells[i].area > 0) {
            remaining.push_back((int)i);
        }
    }

    while (!remaining.empty()) {
        int bestRemainingIndex = -1;
        bool bestReverse = false;
        int bestDistance = std::numeric_limits<int>::max();
        std::vector<IPoint> bestPath;

        for (size_t i = 0; i < remaining.size(); ++i) {
            const BCDCell &cell = cells[remaining[i]];
            std::vector<IPoint> forward = buildCellPath(cell, stride, false);
            std::vector<IPoint> reverse = buildCellPath(cell, stride, true);
            if (!forward.empty()) {
                int dist = pointDistance2(current, forward.front());
                if (dist < bestDistance) {
                    bestDistance = dist;
                    bestRemainingIndex = (int)i;
                    bestReverse = false;
                    bestPath = forward;
                }
            }
            if (!reverse.empty()) {
                int dist = pointDistance2(current, reverse.front());
                if (dist < bestDistance) {
                    bestDistance = dist;
                    bestRemainingIndex = (int)i;
                    bestReverse = true;
                    bestPath = reverse;
                }
            }
        }

        if (bestRemainingIndex < 0 || bestPath.empty()) {
            break;
        }

        std::vector<IPoint> connector = bfsConnect(current, bestPath.front());
        if (!connector.empty()) {
            for (size_t i = 0; i < connector.size(); ++i) {
                if (fullPath.empty() ||
                    fullPath.back().x != connector[i].x ||
                    fullPath.back().y != connector[i].y) {
                    fullPath.push_back(connector[i]);
                }
            }
        } else if (fullPath.empty() ||
                   fullPath.back().x != bestPath.front().x ||
                   fullPath.back().y != bestPath.front().y) {
            fullPath.push_back(bestPath.front());
        }

        for (size_t i = 0; i < bestPath.size(); ++i) {
            if (fullPath.empty() ||
                fullPath.back().x != bestPath[i].x ||
                fullPath.back().y != bestPath[i].y) {
                fullPath.push_back(bestPath[i]);
            }
        }

        current = bestPath.back();
        remaining.erase(remaining.begin() + bestRemainingIndex);
        (void)bestReverse;
    }

    return fullPath.size() >= 2;
}
