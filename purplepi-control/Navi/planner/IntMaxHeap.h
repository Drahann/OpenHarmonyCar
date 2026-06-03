#pragma once

#include "../type.h"
#include <vector>

using namespace std;

class IntHeapPair
{
	public:
	IntHeapPair(void);
	~IntHeapPair(void);
    int o;
    double score;
};


class IntMaxHeap
{
public:
	IntMaxHeap(void);
	~IntMaxHeap(void);


	vector<int> objs;
    vector<double> scores;
    int    heapsize;


    /** Add a node to the heap, in O(log n) time. **/
    void add(int o, double score);

    int size();

    /** Remove and return the element with maximum score in O(log n) time.
     * Return Integer.MIN_VALUE if no elements
     **/
    int removeMax();

    int peekMax();
    bool peekMaxPair(IntHeapPair &heappair);

    /** same as removeMax, but returns the object and its score **/
    bool removeMaxPair(IntHeapPair &heappair);

    void swapNodes(int a, int b);
    void fixup(int parent);
};

