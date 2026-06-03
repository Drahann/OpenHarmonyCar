//#include "StdAfx.h"
#include "UnionFindSimple.h"

int UnionFindSimple::SZ = 2;
UnionFindSimple::UnionFindSimple(void)
{
	pdata = 0;
}


UnionFindSimple::~UnionFindSimple(void)
{
	if(pdata!=0)
		delete [] pdata;
}
