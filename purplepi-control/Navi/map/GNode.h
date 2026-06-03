#pragma once

#include "../type.h"
#include <map>
#include <string>
#include <vector>


using namespace std;

#include "Attributes.h"
class GNode
{
public:
	Pose state;

    /** Initial value of the node. NEVER NULL. **/
    Pose init;

    /** Ground truth of the node, may be null **/
    Pose truth;

    Attributes attributes;

	GNode(void);
	~GNode(void);


	void setAttribute(string s, vector<Pose> &o);
    void getAttribute(string s,vector<Pose> &o);

};

