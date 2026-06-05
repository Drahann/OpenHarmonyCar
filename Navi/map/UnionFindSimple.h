#pragma once
class UnionFindSimple
{
public:
	UnionFindSimple(void);
	~UnionFindSimple(void);


	int *pdata; // alternating arent ids, rank, size.

    static  int SZ ;
	int ndatalength;

    /** @param maxid The maximum node id that will be referenced. **/

    UnionFindSimple(int maxid)
    {
        pdata = new int[maxid*SZ];

		ndatalength = maxid*SZ;
        reset();
    }

    // initializes each node to it's cluster

    inline void reset()
    {

        for (int i = 0; i < ndatalength/SZ; i++) {
            // everyone is their own cluster of size 1
            pdata[SZ*i+0] = i;
            pdata[SZ*i+1] = 1;
        }
    }

    inline int size()
    {
        return ndatalength/SZ;
    }

    inline int getSetSize(int id)
    {
        return pdata[SZ*getRepresentative(id)+1];
    }

    inline int getRepresentative(int id)
    {
        // terminal case: a node is its own parent.
        if (pdata[SZ*id]==id)
            return id;

        // otherwise, recurse...
        int root = getRepresentative(pdata[SZ*id]);

        // short circuit the path.
        pdata[SZ*id] = root;

        return root;
    }

    /** returns the id of the merged node. **/
    inline int connectNodes(int aid, int bid)
    {
        int aroot = getRepresentative(aid);
        int broot = getRepresentative(bid);

        if (aroot == broot)
            return aroot;

        int asz = pdata[SZ*aroot+1];
        int bsz = pdata[SZ*broot+1];

        if (asz > bsz) {
            pdata[SZ*broot] = aroot;
            pdata[SZ*aroot+1] += bsz;
            return aroot;
        } else {
            pdata[SZ*aroot] = broot;
            pdata[SZ*broot+1] += asz;
            return broot;
        }
    }

  








};

