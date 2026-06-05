#pragma once

#include "../type.h"

#include <map>
#include <string>
#include <vector>


using namespace std;
class Attributes
{
public:

	map< string, vector<Pose> > attrs;
public:
	Attributes(void);
	~Attributes(void);

    


    // code can be null
    void setAttribute(string key, vector<Pose> &o);
    

    void  getAttribute(string key,vector<Pose> &o);
 


};

