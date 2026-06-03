#pragma once


#include "../type.h"
#include "GNode.h"
#include "Attributes.h"

#include <map>
#include <string>
#include <vector>


using namespace std;
class Graph
{
public:
	Graph(void);
	~Graph(void);
  /** the set of all constraints. **/
  //  public ArrayList<GEdge> edges = new ArrayList<GEdge>();

    /** the set of all nodes. **/
    vector<GNode> nodes;

    /** Each node may have a different number of degrees of
        freedom. The stateIndices array allows us to rapidly lookup
        the state vector index corresponding to a particular
        node. (These are computed on-demand by getStateIndex().)
    **/
    vector<int> stateIndices;

    /** What is the index in g.nodes for a given GNode? This is
     * updated on demand by indexOf. **/
    map<GNode, int> nodeIndices ;

    Attributes attributes;


};

