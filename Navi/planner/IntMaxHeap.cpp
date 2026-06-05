//#include "StdAfx.h"
#include "IntMaxHeap.h"


IntHeapPair::IntHeapPair(void)
{
}


IntHeapPair::~IntHeapPair(void)
{
}



IntMaxHeap::IntMaxHeap(void)
{
	heapsize = 0;
}


IntMaxHeap::~IntMaxHeap(void)
{
}

   /** Add a node to the heap, in O(log n) time. **/
void IntMaxHeap::add(int o, double score)
{
        // grow?

	objs.push_back(o);
	scores.push_back(score);

    int node = heapsize;
    heapsize++;

    do {
            node = (node - 1) / 2;
            fixup(node);
        } while (node != 0);
}

int IntMaxHeap::size()
{
    return heapsize;
}

int IntMaxHeap::removeMax()
{
   if (heapsize == 0)
        return Integer_MIN_VALUE;
    int m = objs.at(0);
    objs.at(0) = objs.at(heapsize - 1);
    scores.at(0) = scores.at(heapsize - 1);
    objs.at(heapsize - 1) = Integer_MIN_VALUE;
    heapsize--;
       fixup(0);
     return (int) m;
}

int IntMaxHeap::peekMax()
    {
        if (heapsize == 0)
            return Integer_MIN_VALUE;
        return (int) objs.at(0);
    }

bool IntMaxHeap::peekMaxPair(IntHeapPair &heappair)
    {
        if (heapsize == 0)
            return false;
       
        heappair.o = (int) objs.at(0);
        heappair.score = scores.at(0);
        return true;
    }

    /** same as removeMax, but returns the object and its score **/
     bool IntMaxHeap::removeMaxPair(IntHeapPair &heappair)
    {
        if (heapsize == 0)
            return false;
        int m = objs.at(0);
        double s = scores.at(0);
        objs.at(0) = objs.at(heapsize - 1);
        scores.at(0) = scores.at(heapsize - 1);
		objs.pop_back();
		scores.pop_back();
       // objs[heapsize - 1] = Integer_MIN_VALUE;
        heapsize--;
        fixup(0);
       
        heappair.o = (int) m;
        heappair.score = s;
        return true;
    }

    void IntMaxHeap::swapNodes(int a, int b)
    {
        int t = objs.at(a);
        objs.at(a) = objs.at(b);
        objs.at(b)= t;
        double s = scores.at(a);
        scores.at(a) = scores.at(b);
        scores.at(b) = s;
    }

     void IntMaxHeap::fixup(int parent)
    {
        int left = parent * 2 + 1;
        int right = left + 1;

        // leaf node. exit.
        if (left >= heapsize)
            return;

        // only left node is valid
        if (right >= heapsize) {
            // do we need to swap the parent and the leaf?
            // we don't need to call fixupHeap recursively since the
            // left node must be a leaf.
            if (scores.at(left) > scores.at(parent))
            {
                swapNodes(left, parent);
                return;
            }
            return;
        }

        // general node case: both right and left are valid nodes.
        // if parent is the maximum, we're done.
        if (scores.at(parent) > scores.at(left) && scores.at(parent) > scores.at(right))
            return;

        // parent is less than either the left, right, or both
        // children
        if (scores.at(left) > scores.at(right)) {
            swapNodes(left, parent);
            fixup(left);

        } else {

            swapNodes(right, parent);
            fixup(right);
        }
    }

  


