#include "Astarplanner.h"

Node *CAstar::DStarSearch(
    Node **initial, int numInitial,
    // 			    double (*gcalc) (Node *),
    // 			    double (*hcalc) (Node *),
    // 			    int (*robotNode) (Node *),
    // 			    int (*neighbors) (Node *, Node
    // **), 			    double (*cost) (Node *, Node *),
    double costR[2], // (f = h + g, g) for the robot node,
                     // large values if never visited
    GridMap &map, GridMap &visionmap) {
    Node *openList;
    Node *closedList;
    Node *current;
    // Node           *p;
    Node *path;
    Node *neighbor[MAXNEIGHBORS]; // 25
    double kold;
    double fold;
    int numNeighbors;
    long i;
    int gblExpand = 0;

    // generate the open list
    openList = NULL;

    // generate the closed list
    closedList = NULL;

    // put the initial nodes on the open list
    for (i = 0; i < numInitial; i++)
        openList = insertOPEN(openList, initial[i]); // nitial[0]=m_RobotEnd

    while (openList != NULL) {
        // stage 1:
        //  assume the open list is always sorted (robot doesn't move while D*
        //  is running)
        current = openList;
        openList = (Node *)openList->next;
        if (openList != NULL)
            openList->prev = NULL;
        gblExpand++;

        // stage 2:	adu: I think it is going to initial the current openlist
        // first node to be expanded?
        //  kold = Get-KMIN()
        // kold = current->k;
        // fold = current->f;

        current->state = CLOSED; // CLOSED = 2;
        current->next = NULL;    // need to reset this back to NULL

        current->prev = NULL;

        // stage 3:	adu: means that the d star search has found the robot
        // node
        //  is the current node the goal node?
        // if (robot(current) && current->k == current->g)
        if (robot(current)) {
            // useless for now
            costR[0] = current->h + current->g;
            costR[1] = current->g;

            // robot node, and a LOWER node
            // If so, return a pointer to the parent node
            path = (Node *)current->parent;

            // set oldOpen to keep around these nodes
            // now return the path
            return (path);
        }

        numNeighbors = getNeighbors(current, neighbor, map, visionmap);

        for (i = 0; i < numNeighbors; i++) {
            if (neighbor[i]->state == NEW) // new node ,add to the openlist
            {
                neighbor[i]->parent = current;
                neighbor[i]->g =
                    current->g + cost(neighbor[i], current, map, visionmap);
                neighbor[i]->h = hfunction(neighbor[i]);
                neighbor[i]->f = neighbor[i]->g + neighbor[i]->h;
                openList = insertOPEN(openList, neighbor[i]);
                neighbor[i]->state = OPEN;

            } else {
                if (neighbor[i]->g >
                    current->g + cost(neighbor[i], current, map, visionmap)) {
                    neighbor[i]->parent = current;
                    neighbor[i]->g =
                        current->g + cost(neighbor[i], current, map, visionmap);
                    neighbor[i]->f = neighbor[i]->g + neighbor[i]->h;
                    openList = insertOPEN(openList, neighbor[i]);
                }
            }
        }

#if 0
		if (kold < current->g) {				// check if any of the neighbors have a better path to the
			// goal
			
				//my opinion: the following line is not appeared in the original paper;	
				//I still don't understand why the author put this line here;
			for (i = 0; i < numNeighbors; i++) 
			{
				
				if ((neighbor[i]->state != NEW) && LESSEQ(neighbor[i]->f, neighbor[i]->g, fold, kold) &&
					(current->g > neighbor[i]->g + cost(neighbor[i], current,map,visionmap))) 
				{
					
					// reset the back pointer to the better neighbor
					current->parent = neighbor[i];
					
					// calculate the new g value for the current node
					current->g = neighbor[i]->g + cost(neighbor[i], current,map,visionmap);
				}
			}
		}
		
		if (kold == current->g) 
		{		       
			for (i = 0; i < numNeighbors; i++) 
			{
				if ((neighbor[i]->state == NEW) ||
					((neighbor[i]->parent == current) && (neighbor[i]->g != current->g + cost( neighbor[i],current,map,visionmap))) ||
					((neighbor[i]->parent != current) && (neighbor[i]->g > current->g + cost( neighbor[i],current,map,visionmap)))) 
				{
					
					// set the back pointer
					neighbor[i]->parent = current;
					
					// insert the neighbor into OPEN with the new G value
					openList = insertOPEN(openList, neighbor[i], current->g + cost(current, neighbor[i],map,visionmap));
				}
			}
		}
		else 
		{				       
			for (i = 0; i < numNeighbors; i++) 
			{
				
				if ((neighbor[i]->state == NEW) ||
					((neighbor[i]->parent == current) && (neighbor[i]->g != current->g + cost( neighbor[i],current,map,visionmap)))) 
				{
					
					printf("inserted a neighbor with a new cost value\n");
					
					// set the back pointer
					neighbor[i]->parent = current;
					
					// insert the neighbor into OPEN with the new g value
					openList = insertOPEN(openList, neighbor[i], current->g + cost( neighbor[i],current,map,visionmap));
				}
				else 
				{
					// adu:		here is a little difference with the book
					if ((neighbor[i]->parent != current) && (neighbor[i]->g > current->g + cost(neighbor[i],current, map,visionmap))) 
					{
						
						//			    		printf("inserted self as a holding action\n");
						
						// insert the current node into OPEN as a holding action until its neighbors are optimal
						openList = insertOPEN(openList, current, current->g);
					}
					else if ((neighbor[i]->parent != current) &&
						(current->g > neighbor[i]->g + cost(neighbor[i], current,map,visionmap)) &&
						(neighbor[i]->state == CLOSED) && LESS(fold, kold, neighbor[i]->f, neighbor[i]->g)) 
					{
						
						//		    				printf("re-insert this CLOSED node -inserted neighbor as a holding action\n");
						
						// re-insert this CLOSED node since it is not optimal but already provides a better path
						openList = insertOPEN(openList, neighbor[i], neighbor[i]->g);
					}
				}
			}
		}
#endif

        // Test to see if we have expanded too many nodes without a solution
        if (gblExpand > MAXNODES) {
            printf("Expanded more than the maximum allowable nodes (%d). "
                   "Terminating\n",
                   gblExpand);

            return (NULL);
        }
    } // end of OPEN loop

    // if we got here, then there is no path to the goal
    freeNode(openList);
    return (NULL);
}