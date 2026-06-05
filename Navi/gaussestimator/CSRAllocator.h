#pragma once
#include "Vec.h"
#include <vector>

using namespace std;

class CSRAllocator
{

public:
	CSRAllocator()
	{
	};

public:
	vector< vector<CSRVec> > m_cache;

	vector<CSRVec> getBucket(int capacity, int up);
	CSRVec get(int length, int mincapacity);
	void put(CSRVec v);
};
