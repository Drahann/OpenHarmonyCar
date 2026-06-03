#include "Attributes.h"


Attributes::Attributes(void)
{
}


Attributes::~Attributes(void)
{
}


 void Attributes::setAttribute(string key, vector<Pose> &o)
 {
        
      if((attrs[key].size())>0 )
	  {
		  attrs[key].clear();
		  attrs[key] = o;

	  }
	  else
	  {
		  attrs[key] = o;

	  }
 }

void  Attributes::getAttribute(string key,vector<Pose> &o )
{   
	if((attrs[key].size())>0 )
	{
		o = attrs[key];
	}
}

