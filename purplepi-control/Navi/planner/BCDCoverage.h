#pragma once

#include "Astarplanner.h"
#include <map>
#include <vector>

struct BCDSlice
{
    int top;
    int bottom;
    int cellId;
};

struct BCDCell
{
    int id;
    int xmin;
    int xmax;
    int area;
    std::map<int, std::pair<int, int> > colSpan;

    BCDCell()
        : id(-1), xmin(0), xmax(0), area(0)
    {
    }
};

class BCDCoverage
{
public:
    std::vector<IPoint> fullPath;
    std::vector<int> cellOf;
    std::vector<BCDCell> cells;

public:
    BCDCoverage();
    ~BCDCoverage();

    void clear();
    bool buildPathFromGridState(GridState *gridState,
                                int width,
                                int height,
                                int xmin,
                                int ymin,
                                int xmax,
                                int ymax,
                                IPoint start,
                                int stride);

private:
    std::vector<unsigned char> freeMask;
    int mapWidth;
    int mapHeight;

    bool isGridFree(GridState *gridState, int x, int y);
    bool isFreeCell(int x, int y) const;
    bool buildFreeMask(GridState *gridState,
                       int xmin,
                       int ymin,
                       int xmax,
                       int ymax);
    std::vector<BCDSlice> collectColumnSlices(int x) const;
    bool slicesOverlap(const BCDSlice &a, const BCDSlice &b) const;
    int createCell();
    void addSliceToCell(int x, const BCDSlice &slice);
    void decompose();
    bool nearestFreePoint(IPoint start,
                          int xmin,
                          int ymin,
                          int xmax,
                          int ymax,
                          IPoint &result) const;
    std::vector<IPoint> buildCellPath(const BCDCell &cell,
                                      int stride,
                                      bool reversePath) const;
    std::vector<IPoint> bfsConnect(IPoint start, IPoint goal) const;
    int pointDistance2(IPoint a, IPoint b) const;
};
