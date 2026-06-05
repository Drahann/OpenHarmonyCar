#pragma once
#include "GNode.h"
class GXYTNode :
	public GNode
{
public:
	GXYTNode(void);
	~GXYTNode(void);



    int getDOF()
    {
        return 3;
    }

};

