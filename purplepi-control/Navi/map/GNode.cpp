#include "GNode.h"


GNode::GNode(void)
{
}


GNode::~GNode(void)
{
}

	void GNode::setAttribute(string s, vector<Pose> &o)
    {
      
        attributes.setAttribute(s,o);
    }

    void GNode::getAttribute(string s,vector<Pose> &o)
    {
      
        attributes.getAttribute(s,o);
    }
