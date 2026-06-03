#include "ant.h"

void ChangeValue(int* ip){
	*ip = 5;
}

void InitializeMatrix(int* ip){
	for (int i = 0; i<5; i++){
		ip[i]=0;
	}
}

void ChangeMatrix(int* ip){

	for (int i = 0; i<5; i++){
		ip[i]=1;
	}

}

void ShowMatrix(int* ip){
	for (int i = 0; i<5; i++){
		printf("The %d element is %d.\n", i+1, ip[i]);
	}
}

void SetMap(BYTE* map_input, BYTE *map_output, int width, int height){
	for (int i = 0; i<height; i++){
		for (int j = 0; j<width; j++){
			if (map_input[i*width+j]==255){
				map_output[i*width+j] = 'w';//ascII code of w is 119
			}
			else{
				map_output[i*width+j] = map_input[i*width+j];
			}
		}
	}
}

void ShowMap(BYTE *map_input, int a, int b){
	for (int i = 0; i<a; i++){
		for (int j = 0; j<b; j++){
			printf("The %d element of the map is %c.\n", i*b+j, map_input[i*b+j]);
		}
	}
}

void SliceConnectChangePointer(SliceConnect *scp, int x, int y){
	//scp->free_num = x;
	//scp->obs_num = y;
}

void SliceConnectChange(SliceConnect &sc, int x, int y){
	//sc.free_num = x;
	//sc.obs_num = y;
}

void ShowSliceConnect(SliceConnect sc){

	//printf("The free interval number is %d.\n", sc.free_num);
	//printf("The obstacle interval number is %d.\n", sc.obs_num);

	// print free interval vector
	if (sc.free_interval.size()>0){
		for (int i = 0; i<sc.free_interval.size(); i++){
		printf("The %d free interval is [%d, %d].\n", i+1, sc.free_interval[i].in+1, sc.free_interval[i].out+1);
		}
	}
	else{
		printf("The free inteval number is %d.\n",sc.free_interval.size());
		printf("There is no free inteval.\n");
	}


	/*
	// print obstacle interval vector
	if (sc.obs_interval.size()>0){
		//printf("We are here 11.\n");
		for (int i = 0; i<sc.obs_interval.size(); i++){
			printf("The %d obstacle interval is [%d, %d].\n", i+1, sc.obs_interval[i].in+1, sc.obs_interval[i].out+1);
		}
	}
	else{
		printf("The obstacle inteval number is %d.\n",sc.obs_interval.size());
		printf("There is no obstacle inteval.\n");
	}
	*/

}

// height >=2 
// function to get the connectivity of the slice
// good after test
// k is the number in the array

void SliceConnectivity(SliceConnect &sc, BYTE *map_data, int width, int height, int k){

	// clear for initialization
	sc.free_interval.clear();
	sc.obs_interval.clear();

	Interval free_interval, obs_interval;
	free_interval.in = C_up;
	free_interval.out = C_low;
	obs_interval.in = C_up;
	obs_interval.out = C_low;

	//sc.free_num = 0;
	//sc.obs_num = 0;

	if (height<2){
		printf("The input height is too small.\n");
	}
	else { // height >= 2
		for (int i = 1; i<height; i++){
			// we begin with the free space
			if (map_data[(i-1)*width+k]==FREE){

				if (free_interval.in>i-1) free_interval.in = i-1;
				if (free_interval.out<i-1) free_interval.out = i-1;

				// if the next line still free space
				if (map_data[i*width+k]==FREE){
					if (free_interval.in>i){free_interval.in = i;}
					free_interval.out = i;
					// reach the lower boundary
					if (i==height-1)
					sc.free_interval.push_back(free_interval);
				}
				// obstacle area
				else { // map_data[i*width+k]==WALL
					sc.free_interval.push_back(free_interval);
					free_interval.in = C_up;
					free_interval.out = C_low;
					// from free space to obstacle
					obs_interval.in = i;
					obs_interval.out = i; 
				}



			}

			// we begin with the obstalce
			else { // map_data[(i-1)*width+k]==WALL
				if (obs_interval.in>i-1) obs_interval.in = i-1;
				if (obs_interval.out<i-1) obs_interval.out = i-1;

				// if the next line is free space
				if (map_data[i*width+k]==FREE){
					//sc.obs_interval.push_back(obs_interval);
					obs_interval.in = C_up;
					obs_interval.out = C_low;
					// from obstacle to free space
					free_interval.in = i;
					free_interval.out = i;
				}
				// the next line is still obstacle
				else { // map_data[i*width+k]==WALL
					if(obs_interval.in>i) {obs_interval.in = i;}
					obs_interval.out = i;
					if (i==height-1){
						//sc.obs_interval.push_back(obs_interval);
					}
				}
				

			}
		}
	}

}


void IntegerVectorChange(vector<int> iv){

	iv.push_back(1);
	for (int i=0; i<iv.size(); i++){
		//printf("The %d element in the vector is %d.\n", i+1, iv[i]);
	}

}


void IntegerVectorChangePointer(vector<int> &ip){

	ip.push_back(2);

}


// function to get the intersection of the intervals
// interval i1 and i2 must be no empty
int Intersection(Interval i1, Interval i2){

	if (i1.in<=i2.out&&i2.in<=i1.out){
		//printf("The intersection is [%d, %d].\n", max(i1.in, i2.in)+1, min(i1.out, i2.out)+1);
		return 1;
	}
	else {
		//printf("There is no intersection.\n");
		return 0;
	}

}

// i1<=sc1.free_interval.size()
// return an integer: -1 means error, 0 means sc1.free_interval is empty, 1 means normal
int FreeIntervalIntersectionNumber(vector<int> &iv, int i1, SliceConnect sc1, SliceConnect sc2){

	// clear for initialization
	iv.clear();

	if (i1>sc1.free_interval.size()){
		printf("The i1 is too big, no such interval number.\n");
		return -1;
	}
	else{
		if (sc1.free_interval.size()==0) {
			return 0;
		}

		else {

			for (int i2=0; i2<sc2.free_interval.size();i2++){
				//printf("sc2 free interval size is %d.\n",sc2.free_interval.size());
				if (Intersection(sc1.free_interval[i1], sc2.free_interval[i2])){
					//printf("now push %d\n",i2);
					iv.push_back(i2);
				}
			}
			return 1;
		}
	}

}

// map_input is mapx; map_output is sliced_map
int SliceDecomposition(BYTE *map_input, int width, int height, int *map_output){


	for (int x = 0; x< height; x++){
		for (int y = 0; y < width; y++){
			if (map_input[x*width+y]=='w'){
					map_output[x*width+y]=0;
			}
			else {
				map_output[x*width+y]=1;
			}
		}
	}
	
	//ShowSliceDecomposition(map_output, width, height);

	int num = 0;
	int i0 = 0;
	SliceConnect sc0;
	SliceConnectivity(sc0, map_input, width, height, i0);
	//ShowSliceConnect(sc0);

	while (i0<width){
		if (sc0.free_interval.size()==0){
			//printf("We are here 00.\n");
			i0++;
			SliceConnectivity(sc0, map_input, width, height, i0);
		}
		else {
			//printf("We are here 11.\n");
			break;
		}
	}
	printf("The i0 number is %d.\n", i0);
	// then we get the first i0 with the non empty free interval

	SliceConnect sc1, sc2;
	// sc2.free_interval[j] intersects sc1.free_interval
	vector<int> inter_num_array_21;
	// sc1.free_interval[j] intersects sc2.free_interval
	vector<int> inter_num_array_12;


	int i = i0;

	for (int i = i0; i<width ;i++){
	//i = i0+5;
	//for (int i = i0; i<i0+10 ;i++){

		SliceConnectivity(sc1, map_input, width, height, i-1);
		SliceConnectivity(sc2, map_input, width, height, i);

		for (int j=0; j<sc2.free_interval.size(); j++){
		//int j = 0;

			//printf("array_21 size = %d\n",inter_num_array_21.size());
			//printf("array_12 size = %d\n",inter_num_array_12.size());

			FreeIntervalIntersectionNumber(inter_num_array_21, j, sc2, sc1);

			// size >= 2 means regions disappear || size ==0 means new region emerges
			// there we have the problem
			// when initialization, size = 0 naturally

			if (inter_num_array_21.size()>=2){
				//printf("We are here 00.\n");
				num++;
				for (int k=sc2.free_interval[j].in; k<=sc2.free_interval[j].out;k++){
					map_output[k*width+i]=num;
				}
			}

			else if (inter_num_array_21.size()==0){
				//printf("We are here 11.\n");
				num++;
				for (int k=sc2.free_interval[j].in; k<=sc2.free_interval[j].out;k++){
					map_output[k*width+i]=num;
				}
			}
			else {  // inter_num_array_21.size()==1
				// then we have to consider inter_num_array_12.size() 
				FreeIntervalIntersectionNumber(inter_num_array_12, inter_num_array_21[0], sc1, sc2);
				// size>=2 means new region emerges
				if (inter_num_array_12.size()>=2){
					//printf("We are here 22.\n");
					num++;
					for (int k=sc2.free_interval[j].in; k<=sc2.free_interval[j].out;k++){
						map_output[k*width+i]=num;
					}
				}
				else {  // size == 1 means no change of connectivity
					//printf("We are here 33.\n");
					for (int k=sc2.free_interval[j].in; k<=sc2.free_interval[j].out;k++){
						int temp = sc1.free_interval[inter_num_array_21[0]].in;
						map_output[k*width+i]=map_output[temp*width+i-1];
					}					
				}
				
			}

		}

	}
	//map_output[52]=0;
	printf("There are %d regions in the sliced map.\n", num);
	return num;

}


void ShowSliceDecomposition(int *map_input, int width, int height){
	for (int i = 0; i<height; i++){
		for (int j = 0; j<width; j++){
			printf("%d ", map_input[i*width+j]);
		}
		printf("\n");
	}
}


void CopySliceDecomposition(int *map_input, int width, int height, int *map_output){

	for (int i = 0; i<height; i++){
		for (int j = 0; j<width; j++){
			map_output[i*width+j] = map_input[i*width+j];
		}
	}

}

void ComparasonSliceDecomposition(int *map1, int *map2, int width, int height){
	int n = 0;
	for (int i = 0; i<height; i++){
		for (int j = 0; j<width; j++){
			if (map1[i*width+j] != map2[i*width+j]){
				n++;
				printf("Different coordinate is (%d, %d)\n", i, j);
				printf("map1(%d, %d)=%d\n", i, j, map1[i*width+j]);
				printf("map2(%d, %d)=%d\n", i, j, map2[i*width+j]);
			}
		}
	}
	printf("We have %d differences.\n", n);

}

// initialization of connectivity map
void InitializationConnectivityMap(int region_num, int *mat_output){
	for (int x = 0; x<region_num; x++){
		for (int y = 0; y<region_num; y++){
			mat_output[x*region_num+y] = 0;
		}	
	}
}

void RegionConnectivity(int *map_input, int width, int height, int region_num, int *mat_output){

	// initialization of connectivity map, all zeros
	InitializationConnectivityMap(region_num, mat_output);
	//ShowRegionConnectivity(region_num, map_output);

	for (int k = 1; k<=region_num; k++){
		for (int i = 0; i<height; i++){
			for (int j = 0; j<width-1; j++){
				if (map_input[i*width+j]==k&&map_input[i*width+j+1]!=k&&map_input[i*width+j+1]!=0){
					mat_output[(map_input[i*width+j+1]-1)*region_num+k-1]=1;
					mat_output[(k-1)*region_num+map_input[i*width+j+1]-1]=1;
				} 
			}
		}
	}

	//ShowRegionConnectivity(region_num, mat_output);

}

void ShowRegionConnectivity(int region_num, int *mat_output){

	for (int x = 0; x<region_num; x++){
		for (int y = 0; y<region_num; y++){
			printf("%d",mat_output[x*region_num+y]);
		}
		printf("\n");
	}

}


void ShowRegionDistance(int region_num, double *mat_output){

	for (int x = 0; x<region_num; x++){
		for (int y = 0; y<region_num; y++){
			// for full display in one page
			printf("%d",int(ceil(mat_output[x*region_num+y])));
			//printf("%f ",mat_output[x*region_num+y]);
		}
		printf("\n");
	}

}
/*********************************v1.3 change x y coordinate*************************************/
// centroid with top left coordinate
void GetCentroidPoseGrid(GridMap &map, int *sliced_map, int region_num, Pose *centroid){

	for (int k=1; k<=region_num; k++){
		//printf("01\n");
		//int k = 1;
		int num = 0;
		long int x = 0;
		long int y = 0;
		int label = 0;
		int x0 = 0;
		int y0 = 0;
		// get the centroid in the pixel map
		for (int i=0; i<map.height; i++){
		//int i = 0;
			for (int j=0; j<map.width; j++){
		//int j = 0;
				if (sliced_map[i*map.width+j]==k){
					// record the top left coordinate
					if (label==0){
						x0 = j;
						y0 = i;
						if (map.data[y0*map.width+x0]==0){
							//printf("The %d region's top left corner is (%d, %d),\n", k, x0, y0);
							//printf("and it is in the free space.\n");
							label=1;				
						}				
					}
					num++;
					x += j;
					y += i;
				}
			}
		}
		label = 0;
		int(x/num);
		int(y/num);
		//printf("The %d region's centroid is (%d, %d).\n", k, int(x/num), int(y/num));

		//printf("01\n");
		

		
		// if centroid is in some obstacles, then make it as the top left coordinate
		if (map.data[int(y/num)*map.width+int(x/num)]==255){
			centroid[k-1].x = x0;
			centroid[k-1].y = y0;
			//printf("The %d region's centroid is in some obstacle,\n", k);
			//printf("so we choose the top left corner as the cnetroid.\n", k);
		}
		else{
			centroid[k-1].x = int(x/num);
			centroid[k-1].y = int(y/num);
			//printf("The %d region's centroid is in the free space.\n", k);
		}
		//printf("The %d region's centroid in the real world is (%f, %f).\n", k, centroid[k-1].x,centroid[k-1].y);

		


		//centroid[k-1].x = map.x0+x0*map.metersPerPixel;
		//centroid[k-1].y = map.y0+y0*map.metersPerPixel;

	}	

}

/*********************************v1.3 change x y coordinate*************************************/
// centroid with top left coordinate
void GetCentroidPose(GridMap &map, int *sliced_map, int region_num, Pose *centroid){

	for (int k=1; k<=region_num; k++){
		int num = 0;
		long int x = 0;
		long int y = 0;
		int label = 0;
		int x0 = 0;
		int y0 = 0;
		// get the centroid in the pixel map
		for (int i=0; i<map.height; i++){
			for (int j=0; j<map.width; j++){
				if (sliced_map[i*map.width+j]==k){
					// record the top left coordinate
					if (label==0){
						x0 = j;
						y0 = i;
						if (map.data[y0*map.width+x0]==0){
							//printf("The %d region's top left corner is (%d, %d),\n", k, x0, y0);
							//printf("and it is in the free space.\n");
							label=1;				
						}				
					}
					num++;
					x += j;
					y += i;
				}
			}
		}
		label = 0;
		int(x/num);
		int(y/num);
		//printf("The %d region's centroid is (%d, %d).\n", k, int(x/num), int(y/num));
		

		
		// if centroid is in some obstacles, then make it as the top left coordinate
		if (map.data[int(y/num)*map.width+int(x/num)]==255){
			centroid[k-1].x = map.x0+x0*map.metersPerPixel;
			centroid[k-1].y = map.y0+y0*map.metersPerPixel;
			//printf("The %d region's centroid is in some obstacle,\n", k);
			//printf("so we choose the top left corner as the cnetroid.\n", k);
		}
		else{
			centroid[k-1].x = map.x0+int(x/num)*map.metersPerPixel;
			centroid[k-1].y = map.y0+int(y/num)*map.metersPerPixel;
			//printf("The %d region's centroid is in the free space.\n", k);
		}
		//printf("The %d region's centroid in the real world is (%f, %f).\n", k, centroid[k-1].x,centroid[k-1].y);

		


		//centroid[k-1].x = map.x0+x0*map.metersPerPixel;
		//centroid[k-1].y = map.y0+y0*map.metersPerPixel;

	}	

}

void ChangeCentroidToGlobal(GridMap &map, int region_num, Pose *centroid_grid, Pose *centroid_global){

	for (int i = 0; i<region_num; i++){
		centroid_global[i].x = map.x0+centroid_grid[i].x*map.metersPerPixel;
		centroid_global[i].y = map.y0+centroid_grid[i].y*map.metersPerPixel;
	}
}

void ShowCentroidPose(int region_num, Pose *centroid){

	for (int i = 0; i<region_num; i++){

		printf("The %d centroid coordinate is (%f, %f).\n", i+1, centroid[i].x, centroid[i].y);
	}

}




void RegionDistance(GridMap &map, int *connectivity, int region_num, Pose *centroid, double *region_dist){

	CAstar astar;
	vector<vector<double> > path;
	//vector<NodeInfo> vtPath;

	// initialization of the region distance 
	for (int i=0; i<region_num; i++){
		for (int j=0; j<region_num; j++){
				region_dist[i*region_num+j] = 0;
				//printf("%d, %d %p\n", i, j,(region_dist+i*region_num+j));
				//*(region_dist+i*region_num+j) = 0;
		}
	}
	//printf("hh\n");
	//ShowRegionDistance(region_num, region_dist);

	for (int i=0; i<region_num; i++){
		for (int j=0; j<region_num; j++){
	//int i = 0;
	//int j = 0;
			path.clear();
			//vtPath.clear();
			// symetric, so i>j is enough


			//if (i>j && connectivity[i*region_num+j]==1){
			if (connectivity[i*region_num+j]==1){
				//astar.PlanPixel(centroid[i], map, centroid[j], vtPath);
				//astar.plan(centroid[i], map, centroid[j], path);
				region_dist[i*region_num+j] = path.size()*map.metersPerPixel;
				region_dist[j*region_num+i] = region_dist[i*region_num+j];
				//printf("region_dist(%d, %d)=%f.\n",i+1, j+1, vtPath.size()*map.metersPerPixel);
				//printf("region_dist(%d, %d)=%f.\n",i+1, j+1, path.size()*map.metersPerPixel);
			}

		}
	}
	/*
	for (int i=0; i<region_num; i++){
		for (int j=0; j<region_num; j++){
			if (region_dist[i*region_num+j]!=0){
				region_dist[j*region_num+i] = region_dist[i*region_num+j];
			}
		}
	}
	*/
	//ShowRegionDistance(region_num, region_dist);

}

void ComparisonConnectDist(int region_num, int *connectivity, double *region_dist){

	for (int i=0; i<region_num; i++){
		for (int j=0; j<region_num; j++){
			if (connectivity[i*region_num+j]==0&&region_dist[i*region_num+j]==0){
				printf("1");
			}
			else if (connectivity[i*region_num+j]!=0&&region_dist[i*region_num+j]!=0){
				printf("1");
			}
			else{
				printf("0");
			}
				
		}
		printf("\n");
	}

}


void IdentifyConnectDist(int region_num, int *connectivity, double *region_dist){
	int sum = 0;
	int n = 0;
	for (int i = 0; i<region_num; i++){
		for (int j = 0; j<region_num; j++){
			sum += region_dist[i*region_num+j]*connectivity[i*region_num+j];
			n += connectivity[i*region_num+j];
		}
	}
	double dist = sum/n;
	for (int i = 0; i<region_num; i++){
		for (int j = 0; j<region_num; j++){
			if (connectivity[i*region_num+j]==1 && region_dist[i*region_num+j]==0){
				region_dist[i*region_num+j] = dist;
			}
		}
	}
}

// esti_dist^(-1)
void TauMatrix(int region_num, double *region_dist, double ave_dist, double* mat_output){

	for (int i = 0; i<region_num; i++){
		for (int j = 0; j<region_num; j++){
			if (region_dist[i*region_num+j]!=0){
				// can not get the average distance value before the first iteration
				mat_output[i*region_num+j]=pow(ave_dist,-1);
				//printf("%d ",int(region_dist[i*region_num+j]));
				//printf("%.10lf ",mat_output[i*region_num+j]);
			}
			else {
				mat_output[i*region_num+j]=0;
			}
		}
		//printf("\n");
	}
	//ShowTauMatrix(region_num, mat_output);

}

void ShowTauMatrix(int region_num, double *mat_output){

	for (int x = 0; x<region_num; x++){
		for (int y = 0; y<region_num; y++){
			// for full display in one page
			//printf("%d",int(mat_output[x*region_num+y]));
			printf("%.10lf ",mat_output[x*region_num+y]);
		}
		printf("\n");
	}

}


void EtaMatrix(int region_num, double *mat_input, double *mat_output){

	for (int i=0; i<region_num; i++){
		for (int j=0; j<region_num; j++){
			if(mat_input[i*region_num+j]==0){
				mat_output[i*region_num+j]=0;
			}
			else {
				mat_output[i*region_num+j]=pow(mat_input[i*region_num+j],-1);
				//printf("%.10lf ",mat_output[i*region_num+j]);
			}
		}
		//printf("\n");
	}

}


void ShowEtaMatrix(int region_num, double *mat_input){

	for (int x = 0; x<region_num; x++){
		for (int y = 0; y<region_num; y++){
			// for full display in one page
			//printf("%d",int(mat_output[x*region_num+y]));
			printf("%.10lf ",mat_input[x*region_num+y]);
		}
		printf("\n");
	}

}


void ShowParent(int region_num, vector<int> &parent){

	// show parent
	for (int i = 0; i<region_num; i++){
		for(int j = 0; j<region_num; j++){
			printf("%d ", parent[j]);
		}
		printf("\n");
	}

}

void AntColonyResize(int region_num, AntColony *ant){

	for (int i = 0; i<region_num; i++){
		ant[i].parent.resize(region_num);
		ant[i].visited.resize(region_num);
	}

}

void InitializationAntColony(int region_num, AntColony *ant){

	// initialization
	for (int i = 0; i<region_num; i++){
		// put one ant at every region
		// thus route never empty
		ant[i].route.clear();
		ant[i].route.push_back(i+1);
		ant[i].dist = 0.0;
		for(int j = 0; j<region_num; j++){
			ant[i].parent[j] = -1;
			ant[i].visited[j] = 0;
		}
	}

	for (int i = 0; i<region_num; i++){
		int index = ant[i].route.at(ant[i].route.size()-1)-1;
		ant[i].parent[index] = 0; // no parent
		ant[i].visited[index] = 1;// visited
	}


}

void ShowAntColony(int region_num, AntColony *ant){

	// show parent
	for (int i = 0; i<region_num; i++){
		for(int j = 0; j<region_num; j++){
			//printf("%d ", ant[i].parent[j]);
			printf("%d ", ant[j].parent[i]);
		}
		printf("\n");
	}

	printf("\n");

	// show visited
	for (int i = 0; i<region_num; i++){
		for(int j = 0; j<region_num; j++){
			//printf("%d ", ant[i].parent[j]);
			printf("%d ", ant[j].visited[i]);
		}
		printf("\n");
	}

}

void AntNeighbour(AntColony *ant, int region_num, double *region_dist){

	// need to clear the vector every time
	for (int i = 0; i<region_num; i++){
		ant[i].neighbour.clear();
	}

	for (int i = 0; i<region_num; i++){
		int k = ant[i].route.at(ant[i].route.size()-1);
		//printf("step %d k is %d\n", i+1, k);
		for (int j = 0; j<region_num; j++){
			if (region_dist[(k-1)*region_num+j]!=0){
				ant[i].neighbour.push_back(j+1);
			}
		}
	}
}


void AntNeighbourConnect(AntColony *ant, int region_num, int *region_connect){

	// need to clear the vector every time
	for (int i = 0; i<region_num; i++){
		ant[i].neighbour.clear();
	}

	for (int i = 0; i<region_num; i++){
		int k = ant[i].route.at(ant[i].route.size()-1);
		//printf("step %d k is %d\n", i+1, k);
		for (int j = 0; j<region_num; j++){
			if (region_connect[(k-1)*region_num+j]!=0){
				ant[i].neighbour.push_back(j+1);
			}
		}
	}	

}


void ShowAntNeighbour(AntColony *ant, int region_num){

	for (int i = 0; i<region_num; i++){
		printf("The %d ant's current neighbour is region", i+1);
		for (int j = 0; j<ant[i].neighbour.size(); j++){
			printf(" %d", ant[i].neighbour[j]);
		}
		printf(".\n");
	}
}

// v_input1 = neighbour; v_input2 = unvisited
void VectorIntersection(vector<int> &v_input1, vector<int> &v_input2, vector<int> &v_output){

	v_output.clear();

	for (int i = 0; i<v_input1.size();i++){
		//printf("process %d\n", i+1);
		int j = 0;
		while (j<v_input2.size()){
			//printf("step %d\n", j+1);
			//printf("%d %d\n", v_input1.at(i), v_input2.at(j));
			if (v_input1.at(i)==v_input2.at(j)){
				v_output.push_back(v_input1.at(i));
				//printf("push %d\n", v_input1.at(i));
				break;
			}
			j++;

		}
	}

}


void NeighbourInterVisited(vector<int> &neighbour, vector<int> &visited, vector<int> &inter){

	inter.clear();

	for (int i = 0; i<neighbour.size();i++){
		//printf("process %d\n", i+1);
		int j = 0;
		while (j<visited.size()){
			//printf("step %d\n", j+1);
			//printf("%d %d\n", neighbour.at(i), visited.at(neighbour.at(i)-1));
			if (visited[neighbour.at(i)-1]==0){
				//printf("push %d\n", neighbour.at(i));
				inter.push_back(neighbour.at(i));
				break;
			}
			j++;

		}
	}

}



void ShowIntegerVector(vector<int> &vi){

	if (vi.size()>0){
		printf("The integer vector is");
		for (int i = 0; i<vi.size(); i++){
			printf(" %d", vi.at(i));	
		}
		printf(".\n");
		printf("There are %d elements in this vector.\n", vi.size());	
	}
	else {
			printf("The vector is empty.\n");	
	}
}


void UnvisitedRegion(vector<int> &ur){

	ur.clear();

}

void AntRandomWalk(AntColony *ant, int region_num, int* region_connect, double *region_dist, vector<int> &route){

	int select = 0;
	vector<int> inter;
	int k = 3;
	while (!IsOneAntFinished(ant[k].visited)){
		AntNeighbour(ant, region_num, region_dist);
		//AntNeighbourConnect(ant, region_num, region_connect);
		//printf("The neighbour\n");
		//ShowIntegerVector(ant[k].neighbour);
		NeighbourInterVisited(ant[k].neighbour, ant[k].visited, inter);
		//printf("The intersection\n");
		//ShowIntegerVector(inter);
		if (inter.size()>0){
			// select from 0 to n-1, index
			//printf("We select");
			select = random(inter.size());
			//printf(" %d\n", select);
			// inter.at(select) region
			ant[k].parent.at(inter.at(select)-1) = ant[k].route.back();
			// select index
			ant[k].visited.at(inter.at(select)-1) = 1;
			ant[k].route.push_back(inter.at(select));
		}
		else {
			//printf("We go backwards.\n");
			int back = ant[k].parent.at(ant[k].route.back()-1);
			ant[k].visited.at(ant[k].route.back()-1) = 1;
			ant[k].route.push_back(back);
		}
	}

}


void InitializationFinished(int region_num, int *finished){

	for (int i = 0; i<region_num; i++){
		finished[i] = 0;
		//finished[i] = 1;
	}
	//finished[61] = 0;
}

void ShowFinished(int region_num, int *finished){

	for (int i = 0; i<region_num; i++){
		if (finished[i]==1){
			printf("The %d ant has finished.\n", i+1);
		}
		else {
			printf("The %d ant does not finish.\n", i+1);
		}
	}
}

void InitializationRestart(int region_num, int *restart){

	for (int i = 0; i<region_num; i++){
		restart[i]=0;
	}

}

void ShowRestart(int region_num, int *restart){

	for (int i = 0; i<region_num; i++){
		if (restart[i]==1){
			printf("The %d ant has restarted.\n", i+1);
		}
		else {
			printf("The %d ant does not finish the first round.\n", i+1);
		}
	}

}


bool IsOneAntFinished(vector<int> &visited){

	bool finished = true;
	for (int i = 0; i<visited.size(); i++){
		if (visited.at(i)==0){
			finished = false;
			break;
		}
	}
	return finished;
}

bool AllAntsFinished(int region_num, int *finished){

	bool finish = true;
	int i = 0;
	while (i<region_num){
		if (finished[i]==0){
			//printf("i is %d.\n",i);
			finish = false;
			break;
		}
		i++;
	}
	if (finish == true){
		//printf("All the ants have finished their route.\n");
	}
	else {
		//printf("Not all the ants have finished their route.\n");
	}
	return finish;
}


void ShowVisited(vector<int> &vi){

	printf("The visited vector is\n");
	for (int i = 0; i<vi.size(); i++){
		printf("%d", vi.at(i));
	}
	printf("\n");
}

double PseudoRandomValue(double input){
	double output = 0.0;
	// decrease the value to improve the search space from 0.9 to 0.8
	if (input>=0 &&input<=0.1) output=0.8;
	else if (input>=0.9 && input<=1) output=0.8;
	// decrease the value to improve the search space from 0.45 to 0.35
	else if (input> 0.1 && input< 0.9) output = 0.35;
	else output=0.0;
	
	return output;

}

int MaxValueNumInProbability(int length, double *vi){

	double value = 0.0;
	int num = 0;
	for (int i = 0; i<length; i++){
		if (value<vi[i]){
			value = vi[i];
			//printf("max value in step %d is %f\n", i+1, vi[i]);
			num = i;
		}
	}
	return num;
}

int MinLengthAntNum(int region_num, AntColony *ant){
	int n = 0;
	int min = C_up;
	for (int i=0; i<region_num; i++){
		if (min>ant[i].route.size()){
			n = i;
			min = ant[i].route.size();
		}
	}

	return n;
}


int MaxLengthAntNum(int region_num, AntColony *ant){
	int n = 0;
	int max = 0;
	for (int i=0; i<region_num; i++){
		if (max<ant[i].route.size()){
			n = i;
			max = ant[i].route.size();
		}
	}

	return n;
}

double MinDistAntNum(int region_num, AntColony *ant){

	int n = 0;
	double min = C_up;
	for (int i=0; i<region_num; i++){
		if (min>ant[i].dist){
			n = i;
			min = ant[i].dist;
		}
	}

	return n;

}
double MaxDistAntNum(int region_num, AntColony *ant){

	int n = 0;
	double max = 0.0;
	for (int i=0; i<region_num; i++){
		if (max<ant[i].dist){
			n = i;
			max = ant[i].dist;
		}
	}

	return n;

}



void FindExtremeAnt(int num, int region_num, AntColony *ant, AntColony *best, AntColony *worst){

	int min_length_num = 0, max_length_num = 0;
	int min_dist_num = 0, max_dist_num = 0;

	min_length_num = MinLengthAntNum(region_num, ant);
	max_length_num = MaxLengthAntNum(region_num, ant);
	min_dist_num = MinDistAntNum(region_num, ant);
	max_dist_num = MaxDistAntNum(region_num, ant);
	 best[num*2+0] = ant[min_length_num];
	 best[num*2+1] = ant[min_dist_num];
	worst[num*2+0] = ant[max_length_num];
	worst[num*2+1] = ant[max_dist_num];

}

void FindBestRouteIteration(AntColony *best, vector<int> &vi){
	vi.clear();
	int n = 0;
	int length = C_up;
	for (int i = 0; i<ITER_NUM; i++){
		if (length>best[2*i+0].route.size()){
			n = i;
			length = best[2*i+0].route.size();
		}
	}
	vi.swap(best[2*n+0].route);
	//ShowIntegerVector(vi);
}

void AntColonyRegionRoute(int region_num, int *region_connect, double *region_dist, vector<int> &route){

	srand((int)time(0));
	double Tau[region_num*region_num];
	TauMatrix(region_num, region_dist, 1.0, Tau);
	//ShowTauMatrix(region_num, Tau);

	double Eta[region_num*region_num];
	EtaMatrix(region_num, region_dist, Eta);
	//ShowEtaMatrix(region_num, Eta);
	AntColony ant[region_num];
	AntColony best[2*ITER_NUM];
	AntColony worst[2*ITER_NUM];
	
	int num = 0;
	double ratio = 0;
	int select = 0;
	double q0 = 0;
	double q = 0; 
	// intersection vector
	vector<int> inter;

	for (num = 0; num<ITER_NUM; num++){
	//for (num = 0; num<2; num++){

		//printf("It is the %d iteration.\n", num+1);

		// resize parent and visited
		AntColonyResize(region_num, ant);
		// initialization for parent and visited
		InitializationAntColony(region_num, ant);
		// here every time need to reinitialize ant class

		ratio = num/ITER_NUM;
		q0 = PseudoRandomValue(ratio);
		//printf("%f\n", q0);


		// for every ant k
		for (int k = 0; k<region_num; k++){
		//int k = 10;
			// if ant k has not finished
			while (!IsOneAntFinished(ant[k].visited)){
				AntNeighbour(ant, region_num, region_dist);
				//AntNeighbourConnect(ant, region_num, region_connect);
				//printf("The neighbour\n");
				//ShowIntegerVector(ant[k].neighbour);
				NeighbourInterVisited(ant[k].neighbour, ant[k].visited, inter);
				//printf("The intersection\n");
				//ShowIntegerVector(inter);
				if (inter.size()>0){
					q = RandomZeroOne();
					double P_sum = 0.0;
					// q<q0 probable at the beginning and the end
					if (q<q0){
						double P[inter.size()];
						//printf("intersection length is %d\n", inter.size());
						for (int j=0; j<inter.size(); j++){
							P[j] = pow(Tau[(ant[k].route.back()-1)*region_num + inter[j]-1],ALPHA)*pow(Eta[(ant[k].route.back()-1)*region_num + inter[j]-1],BETA);
							//printf("Tau[j] is %f\n", Tau[(ant[k].route.back()-1)*region_num + inter[j]-1]);
							//printf("Eta[j] is %f\n", Eta[(ant[k].route.back()-1)*region_num + inter[j]-1]);
							//printf("P[j] is %f\n", P[j]);
							P_sum += P[j];
						}
						for (int j=0; j<inter.size(); j++){
							P[j] /= P_sum;
							//printf("%f\n", P[j]);
						}
						select = MaxValueNumInProbability(inter.size(),P);
						//printf("We use tau and eta to choose %d\n", select);
					
					}
					// q>q0 random walk
					else {
						// select from 0 to n-1, index
						//printf("We select");
						select = random(inter.size());
						//printf(" %d\n", select);
					}
					// inter.at(select) region
					ant[k].parent.at(inter.at(select)-1) = ant[k].route.back();
					ant[k].dist += region_dist[(ant[k].route.back()-1)*region_num + inter.at(select)-1];
					// select index
					ant[k].visited.at(inter.at(select)-1) = 1;
					ant[k].route.push_back(inter.at(select));


				}
				else {
					//printf("We go backwards.\n");
					int back = ant[k].parent.at(ant[k].route.back()-1);
					ant[k].dist += region_dist[(ant[k].route.back()-1)*region_num + back-1];
					ant[k].visited.at(ant[k].route.back()-1) = 1;
					ant[k].route.push_back(back);
				}
			}
			//printf("For the %d ant\n", k+1);
			//ShowIntegerVector(ant[k].route);
			//printf("Distance is %f\n", ant[k].dist);
		}

		FindExtremeAnt(num, region_num, ant, best, worst);
		//printf("Iter %d best route length %d (with distance %f), best distance %f (with length %d)\n", num+1, best[2*num+0].route.size(), best[2*num+0].dist, best[2*num+1].dist, best[2*num+1].route.size());
		//printf("Iter %d worst route length %d (with distance %f), worst distance %f (with length %d)\n", num+1, worst[2*num+0].route.size(), worst[2*num+0].dist, worst[2*num+1].dist, worst[2*num+1].route.size());

		
		if (num == 0){
			//printf("We are here to calculate the average distance.\n");
			double ave_dist = 0.0;
			for (int w = 0; w<region_num; w++){
				ave_dist += ant[w].dist; 
			}
			ave_dist = ave_dist/region_num;
			//printf("During the first iteration, we estimate the average distance %f", ave_dist);
			TauMatrix(region_num, region_dist, ave_dist, Tau);
		}
			
		// update the pheromone
		// off-line for the best and worst route
		int a=0, b=0;
		for (int x = 0; x< best[2*num+0].route.size()-1; x++){
			a = best[2*num+0].route.at(x)-1;
			b = best[2*num+0].route.at(x+1)-1;
			Tau[a*region_num+b] = (1-RHO)*Tau[a*region_num+b] + RHO*pow(best[2*num+1].dist,-1);
		}

		for (int x = 0; x< worst[2*num+0].route.size()-1; x++){
			a = worst[2*num+0].route.at(x)-1;
			b = worst[2*num+0].route.at(x+1)-1;
			Tau[a*region_num+b] = (1-RHO)*Tau[a*region_num+b] + RHO*pow(worst[2*num+1].dist,-1);
		}

		// calculation of Tau_max and Tau_min
		double Tau_max = 0.0;
		for (int x = 1; x< worst[2*num+0].route.size()+1; x++){
			Tau_max += pow(RHO,worst[2*num+0].route.size()-x)*pow(best[2*num+1].dist,-1);
		}

		double Tau_min = 0.0;
		for (int x = 1; x< best[2*num+0].route.size()+1; x++){
			Tau_min += pow(RHO,best[2*num+0].route.size()-x)*pow(worst[2*num+1].dist,-1);
		}
		// restriction by Tau_max and Tau_min
		for (int x = 0; x<region_num; x++){
			for (int y = 0; y<region_num; y++){
				if (Tau[x*region_num+y]>Tau_max){
					Tau[x*region_num+y]=Tau_max;
				}
				else if (Tau[x*region_num+y]<Tau_min){
					Tau[x*region_num+y]=Tau_min;
				}
				else { }
			}
		}
		

	}
	FindBestRouteIteration(best, route);
	//ShowIntegerVector(route);
	//printf("Iter 1 best route length %d, best distance %f\n", best[0].route.size(), best[0].dist);
	//printf("Iter 2 best route length %d, best distance %f\n", best[1].route.size(), best[1].dist);

}


void RepeatAntColony(int repeat, int region_num, int *connectivity_mat, double *region_dist, vector<int> &route_best){

	vector<int> route;
	AntColonyRegionRoute(region_num, connectivity_mat, region_dist, route);
	route_best.swap(route);
	ShowIntegerVector(route_best);

	for (int i = 0; i<repeat; i++){
		//printf("%d repetition of ant colony\n", i+1);
		printf("%d\n", i+1);
		AntColonyRegionRoute(region_num, connectivity_mat, region_dist, route);
		if (route.size()<route_best.size()){
			route_best.clear();
			route_best.swap(route);
		}
		ShowIntegerVector(route_best);
	}

}

// 0 1 left up, 2 3 left down, 4 5 right up, 6 7 right down
// 10000 10000, 10000 0      , 0 10000     , 0 0 
// #define C_up  10000
// #define C_low  0
void InitializationCardinalPoints(int region_num, double *cp, double *bound_lr){

	for (int i = 0; i<region_num; i++){
		cp[i*8 + 0] = bound_lr[i*2 + 0];
		cp[i*8 + 1] = C_up;
		cp[i*8 + 2] = bound_lr[i*2 + 0];
		cp[i*8 + 3] = C_low;
		cp[i*8 + 4] = bound_lr[i*2 + 1];
		cp[i*8 + 5] = C_up;
		cp[i*8 + 6] = bound_lr[i*2 + 1];
		cp[i*8 + 7] = C_low;

	}
}

/***************************v1.2 addition for vertical cp*******************************/
void InitializationCardinalPointsVertical(int region_num, double *cp_vertical, double *bound_ud){

	for (int i = 0; i<region_num; i++){
		cp_vertical[i*8 + 0] = C_up;
		cp_vertical[i*8 + 1] = bound_ud[i*2 + 0];
		cp_vertical[i*8 + 2] = C_up;
		cp_vertical[i*8 + 3] = bound_ud[i*2 + 1];
		cp_vertical[i*8 + 4] = C_low;
		cp_vertical[i*8 + 5] = bound_ud[i*2 + 0];
		cp_vertical[i*8 + 6] = C_low;
		cp_vertical[i*8 + 7] = bound_ud[i*2 + 1];
	}
}


void ShowCardinalPoints(int region_num, double *cp){

	for (int i = 0; i<region_num; i++){
		printf("Region %d: %f, %f, %f, %f, %f, %f，%f, %f\n", i+1, cp[i*8+0], cp[i*8+1], cp[i*8+2], cp[i*8+3], cp[i*8+4], cp[i*8+5], cp[i*8+6], cp[i*8+7]);
	}
}

void ReadRouteTxtFile(const char* fileName, vector<int> &route){
	route.clear();
	ifstream infile;
	infile.open(fileName);
	int temp = 0;
	//printf("temporary value is %d.\n", temp);

	while (infile){
		infile >> temp;
		//printf("temporary value is %d.\n", temp);
		route.push_back(temp);
		//printf("Save...\n");
		//break;
	}
	infile.close();


}

void RegionLeftAndRightPoints(int *slice_map, int width, int height, int region_num, double *cp){

	for (int i = 0; i<height; i++){
		for (int j = 0; j< width; j++){
			for (int k = 0; k<region_num; k++){
				if (slice_map[i*width+j]==k+1){
				//if (slice_map[i*width+j]==k+1){
					// left
					if (cp[8*k+0]>j) {cp[8*k+0] = j;}
					if (cp[8*k+2]>j) {cp[8*k+2] = j;}
					// right
					if (cp[8*k+4]<j) {cp[8*k+4] = j;}
					if (cp[8*k+6]<j) {cp[8*k+6] = j;}
				}
			}
		}
	}
}


void RegionCardinalPoints(int *slice_map, int width, int height, int region_num, double *cp){
	for (int i = 0; i<height; i++){
		for (int j = 0; j< width; j++){
			for (int k = 0; k<region_num; k++){
				if (slice_map[i*width+j]==k+1){
					if (j==cp[8*k+0]){
						if(cp[8*k+1]>i){cp[8*k+1]=i;}
					}
					if (j==cp[8*k+2]){
						if(cp[8*k+3]<i){cp[8*k+3]=i;}
					}
					if (j==cp[8*k+4]){
						if(cp[8*k+5]>i){cp[8*k+5]=i;}
					}
					if (j==cp[8*k+6]){
						if(cp[8*k+7]<i){cp[8*k+7]=i;}
					}
				}
			}
		}
	}
}

/***************************v1.2 addition for vertical cp*******************************/
void RegionCardinalPointsVertical(int *slice_map, int width, int height, int region_num, double *cp_vertical){
	for (int i = 0; i<height; i++){
		for (int j = 0; j< width; j++){
			for (int k = 0; k<region_num; k++){
				if (slice_map[i*width+j]==k+1){
					if (i==cp_vertical[8*k+1]){
						if(cp_vertical[8*k+0]>j){cp_vertical[8*k+0]=j;}
					}
					if (i==cp_vertical[8*k+3]){
						if(cp_vertical[8*k+2]>j){cp_vertical[8*k+2]=j;}
					}
					if (i==cp_vertical[8*k+5]){
						if(cp_vertical[8*k+4]<j){cp_vertical[8*k+4]=j;}
					}
					if (i==cp_vertical[8*k+7]){
						if(cp_vertical[8*k+6]<j){cp_vertical[8*k+6]=j;}
					}
				}
			}
		}
	}
}



void ChangeRegionCardinalPointsFromGridToGlobal(int region_num, double *cp, GridMap &map, double *cp_global){

	for (int i = 0; i<region_num; i++){
		cp_global[8*i+0] = map.x0 + cp[8*i+0]*map.metersPerPixel;
		cp_global[8*i+1] = map.y0 + cp[8*i+1]*map.metersPerPixel;
		cp_global[8*i+2] = map.x0 + cp[8*i+2]*map.metersPerPixel;
		cp_global[8*i+3] = map.y0 + cp[8*i+3]*map.metersPerPixel;
		cp_global[8*i+4] = map.x0 + cp[8*i+4]*map.metersPerPixel;
		cp_global[8*i+5] = map.y0 + cp[8*i+5]*map.metersPerPixel;
		cp_global[8*i+6] = map.x0 + cp[8*i+6]*map.metersPerPixel;
		cp_global[8*i+7] = map.y0 + cp[8*i+7]*map.metersPerPixel;
	}
}

void OutputTxtFile(const char* fileName, int width, int height, int *sliced_map){

	ofstream outfile;
	outfile.open(fileName);
	//outfile <<"You are such an ass hole."<<endl;
	for (int i = 0; i<height; i++){
		for (int j = 0; j<width; j++){

			//printf("%d ", sliced_map[i*map.width+j]);
			outfile << sliced_map[i*width+j] << " ";
		}
		//printf("\n");
		outfile << endl;
	}
	printf("success for print\n");
	outfile.close();

}


void OutputMapToTxtFile(const char* fileName, int width, int height, BYTE *mapx){

	ofstream outfile;
	outfile.open(fileName);
	//outfile <<"You are such an ass hole."<<endl;
	for (int i = 0; i<height; i++){
		for (int j = 0; j<width; j++){
			if (mapx[i*width+j]==119 || mapx[i*width+j]=='w'){
				outfile << 2 << " ";
			}
			else {
				outfile << 0 << " ";
			}
		}
		outfile << endl;
	}
	printf("success for print\n");
	outfile.close();

}

void OutputRouteToTxtFile(const char* fileName, vector<vector<double> > route){

	ofstream outfile;
	outfile.open(fileName);
	//outfile <<"You are such an ass hole."<<endl;
	for (int i = 0; i<route.size(); i++){
		outfile << route.at(i).at(0) << " " << route.at(i).at(1);
		outfile << endl;
	}
	printf("success for print\n");
	outfile.close();

}

void OutputByteToTxtFile(const char* fileName, int width, int height, BYTE *map_data){

	ofstream outfile;
	outfile.open(fileName);
	//outfile <<"You are such an ass hole."<<endl;
	for (int i = 0; i<height; i++){
		for (int j = 0; j<width; j++){

			//printf("%d ", sliced_map[i*map.width+j]);
			outfile << (int)(map_data[i*width+j]) << " ";
		}
		//printf("\n");
		outfile << endl;
	}
	printf("success for printing original map\n");
	outfile.close();

}


void OutputRegionConnectTxtFile(const char* fileName, int region_num, int *connect_mat){

	ofstream outfile;
	outfile.open(fileName);
	//outfile <<"You are such an ass hole."<<endl;
	for (int i = 0; i<region_num; i++){
		for (int j = 0; j<region_num; j++){
			if (connect_mat[i*region_num+j]==1){
				outfile << "(" << i+1 << ", " << j+1 << ") ";
			}
		}
		outfile << endl;
	}
	outfile.close();

}

void OutputFile(const char* fileName){
	ofstream outfile;
	outfile.open(fileName);
	outfile <<"You are such an ass hole."<<endl;
	//for (int i = 0; i<map.height; i++){
	//	for (int j = 0; j<map.width; j++){
	//for (int i = 0; i<3; i++){
	//	for (int j = 0; j<5; j++){
	//		printf("%d ", sliced_map[i*map.width+j]);
			//outfile << sliced_map[i*map.width+j] << " ";
	//	}
	//	printf("\n");
		//outfile << endl;
	//}
	outfile.close();
}

void OutputRouteTxtFile(const char* fileName, vector<int> &route){

	ofstream outfile;
	outfile.open(fileName);
	for (int i = 0; i<route.size(); i++){
		outfile << route.at(i) << " ";
	}
	outfile << endl;
	outfile.close();

}


// #define C_up  10000
// #define C_low  0

void InitializationLeftAndRightBound(int region_num, double *cp){

	for (int i = 0; i<region_num; i++){
		cp[i*2 + 0] = C_up;
		cp[i*2 + 1] = C_low;
	}

}


void GetLeftAndRightBound(int *slice_map, int width, int height, int region_num, double *cp){

	for (int i = 0; i<height; i++){
		for (int j = 0; j< width; j++){
			for (int k = 0; k<region_num; k++){
				if (slice_map[i*width+j]==k+1){
					// left
					if (cp[2*k+0]>j) {cp[2*k+0] = j;}

					// right
					if (cp[2*k+1]<j) {cp[2*k+1] = j;}

				}
			}
		}
	}

}


void InitializationUpAndDownBound(int region_num, double *cp){

	for (int i = 0; i<region_num; i++){
		cp[i*2 + 0] = C_up;
		//cp[i*2 + 0] = 400;
		cp[i*2 + 1] = C_low;
	}

}

void GetUpAndDownBound(int *slice_map, int width, int height, int region_num, double *cp){

	//printf("slice 52= %d \n", slice_map[0*width+52]);
	for (int i = 0; i<height; i++){
		for (int j = 0; j< width; j++){
			for (int k = 0; k<region_num; k++){
				if (slice_map[i*width+j]==k+1){
					// up
					if (cp[2*k+0]>i) {
						cp[2*k+0] = i;
						/*
						if (i == 0){
							printf("Impossible Sliced_map(%d, %d) is %d.\n", i, j, slice_map[i*width+j]);
							//printf("Impossible (i, j) is (%d, %d).\n", i, j);
						}
						*/
					}

					// down
					if (cp[2*k+1]<i) {cp[2*k+1] = i;}

				}
			}
		}
	}

}

void ShowBounds(int region_num, double *cp){

	for (int i = 0; i<region_num; i++){
		printf("%f, %f\n",cp[i*2+0], cp[i*2+1]);
	}
}


void GetSurface(int region_num, double *bound_lr, double *bound_ud, double *surface){
	for (int i = 0; i<region_num; i++){
		surface[i] = (bound_lr[2*i+1]-bound_lr[2*i+0]+1)*(bound_ud[2*i+1]-bound_ud[2*i+0]+1);
	}
}

void ShowSurface(int region_num, double *surface){
	for (int i = 0; i<region_num; i++){
		printf("surface of region %d is %f\n", i+1, surface[i]);
	}
}


void InitializationStartAndEndPoints(vector<int> &route, double *start, double *end){

	for (int i = 0; i<route.size(); i++){
		start[3*i + 0]=0;
		start[3*i + 1]=0;
		start[3*i + 2]=0;
		end[3*i + 0]=0;
		end[3*i + 1]=0;
		end[3*i + 2]=0;
	}
}

void ShowStartAndEndPoints(vector<int> &route, double *start, double *end){

	for (int i = 0; i<route.size(); i++){
		printf("Step %d Region %d: start %f (%f, %f), end %f (%f, %f).\n", i+1, route.at(i), start[3*i+2], start[3*i+0], start[3*i+1], end[3*i+2], end[3*i+0], end[3*i+1]);
	}
}

double GetDistanceBetweenTwoPoints(double x1, double y1, double x2, double y2){

	double dist = 0.0;
	dist = sqrt(pow(x1-x2,2) + pow(y1-y2,2));
	return dist;

}


// left up 1, left down 2, right up 3, right down 4
void GetStartAndEndPointsGrid(vector<int> &route, double *cp, double *start, double *end){

	double dist;
	int r1 = 0;
	int r2 = 0;

	r1 = route.at(0)-1;
	r2 = route.at(1)-1;

	// start point for the first region
	if (r1>r2){
		// right up
		start[0] = cp[8*r1 + 4];
		start[1] = cp[8*r1 + 5];
		start[2] = 3;
	}
	else {
		// left up
		start[0] = cp[8*r1 + 0];
		start[1] = cp[8*r1 + 1];
		start[2] = 1;
	}

	for (int i = 0; i<route.size()-1; i++){
		r1 = route.at( i ) - 1;
		r2 = route.at(i+1) - 1;
		dist = C_up;
		if (r1>r2){
			// r2 right up r1 left up
			if (dist > GetDistanceBetweenTwoPoints(cp[8*r2+4],cp[8*r2+5],cp[8*r1+0],cp[8*r1+1])){
				dist = GetDistanceBetweenTwoPoints(cp[8*r2+4],cp[8*r2+5],cp[8*r1+0],cp[8*r1+1]);
				end[3*i+0] = cp[8*r1+0];
				end[3*i+1] = cp[8*r1+1];
				end[3*i+2] = 1;
				start[3*(i+1)+0] = cp[8*r2+4];
				start[3*(i+1)+1] = cp[8*r2+5];
				start[3*(i+1)+2] = 3;
			}
			// r2 right up r1 left down
			if (dist > GetDistanceBetweenTwoPoints(cp[8*r2+4],cp[8*r2+5],cp[8*r1+2],cp[8*r1+3])){
				dist = GetDistanceBetweenTwoPoints(cp[8*r2+4],cp[8*r2+5],cp[8*r1+2],cp[8*r1+3]);

				end[3*i+0] = cp[8*r1+2];
				end[3*i+1] = cp[8*r1+3];
				end[3*i+2] = 2;
				start[3*(i+1)+0] = cp[8*r2+4];
				start[3*(i+1)+1] = cp[8*r2+5];
				start[3*(i+1)+2] = 3;
			}
			// r2 right down r1 left up
			if (dist > GetDistanceBetweenTwoPoints(cp[8*r2+6],cp[8*r2+7],cp[8*r1+0],cp[8*r1+1])){
				dist = GetDistanceBetweenTwoPoints(cp[8*r2+6],cp[8*r2+7],cp[8*r1+0],cp[8*r1+1]);

				end[3*i+0] = cp[8*r1+0];
				end[3*i+1] = cp[8*r1+1];
				end[3*i+2] = 1;
				start[3*(i+1)+0] = cp[8*r2+6];
				start[3*(i+1)+1] = cp[8*r2+7];
				start[3*(i+1)+2] = 4;
			}
			// r2 right down r1 left down
			if (dist > GetDistanceBetweenTwoPoints(cp[8*r2+6],cp[8*r2+7],cp[8*r1+2],cp[8*r1+3])){
				dist = GetDistanceBetweenTwoPoints(cp[8*r2+6],cp[8*r2+7],cp[8*r1+2],cp[8*r1+3]);

				end[3*i+0] = cp[8*r1+2];
				end[3*i+1] = cp[8*r1+3];
				end[3*i+2] = 2;
				start[3*(i+1)+0] = cp[8*r2+6];
				start[3*(i+1)+1] = cp[8*r2+7];
				start[3*(i+1)+2] = 4;
			}
		}
		// r1<=r2
		else {
			// r1 right up r2 left up
			if (dist > GetDistanceBetweenTwoPoints(cp[8*r1+4],cp[8*r1+5],cp[8*r2+0],cp[8*r2+1])){
				dist = GetDistanceBetweenTwoPoints(cp[8*r1+4],cp[8*r1+5],cp[8*r2+0],cp[8*r2+1]);
				end[3*i+0] = cp[8*r1+4];
				end[3*i+1] = cp[8*r1+5];
				end[3*i+2] = 3;
				start[3*(i+1)+0] = cp[8*r2+0];
				start[3*(i+1)+1] = cp[8*r2+1];
				start[3*(i+1)+2] = 1;
			}
			// r1 right up r2 left down
			if (dist > GetDistanceBetweenTwoPoints(cp[8*r1+4],cp[8*r1+5],cp[8*r2+2],cp[8*r2+3])){
				dist = GetDistanceBetweenTwoPoints(cp[8*r1+4],cp[8*r1+5],cp[8*r2+2],cp[8*r2+3]);
				end[3*i+0] = cp[8*r1+4];
				end[3*i+1] = cp[8*r1+5];
				end[3*i+2] = 3;
				start[3*(i+1)+0] = cp[8*r2+2];
				start[3*(i+1)+1] = cp[8*r2+3];
				start[3*(i+1)+2] = 2;
			}
			// r1 right down r2 left up
			if (dist > GetDistanceBetweenTwoPoints(cp[8*r1+6],cp[8*r1+7],cp[8*r2+0],cp[8*r2+1])){
				dist = GetDistanceBetweenTwoPoints(cp[8*r1+6],cp[8*r1+7],cp[8*r2+0],cp[8*r2+1]);
				end[3*i+0] = cp[8*r1+6];
				end[3*i+1] = cp[8*r1+7];
				end[3*i+2] = 4;
				start[3*(i+1)+0] = cp[8*r2+0];
				start[3*(i+1)+1] = cp[8*r2+1];
				start[3*(i+1)+2] = 1;
			}
			// r1 right down r2 left down
			if (dist > GetDistanceBetweenTwoPoints(cp[8*r1+6],cp[8*r1+7],cp[8*r2+2],cp[8*r2+3])){
				dist = GetDistanceBetweenTwoPoints(cp[8*r1+6],cp[8*r1+7],cp[8*r2+2],cp[8*r2+3]);
				end[3*i+0] = cp[8*r1+6];
				end[3*i+1] = cp[8*r1+7];
				end[3*i+2] = 4;
				start[3*(i+1)+0] = cp[8*r2+2];
				start[3*(i+1)+1] = cp[8*r2+3];
				start[3*(i+1)+2] = 2;
			}
		}
	}

	// end point for the last region

	if (r1>r2){
		// left down
		end[3*(route.size()-1)+0] = cp[8*r2 + 2];
		end[3*(route.size()-1)+1] = cp[8*r2 + 3];
		end[3*(route.size()-1)+2] = 2;
	}
	else {
		// right down
		end[3*(route.size()-1)+0] = cp[8*r2 + 6];
		end[3*(route.size()-1)+1] = cp[8*r2 + 7];
		end[3*(route.size()-1)+1] = 4;
	}

}

void CopyArray(double *a, double *a_copy, int num){

	for (int i = 0; i<num; i++){
		a_copy[i] = a[i];
	}
}

void GetUpperAndLowerGrid(int num, vector<int> &route, int *sliced_map, double *start_grid, double *end_grid, 	vector<vector<double> > &upper, vector<vector<double> > &lower, int odd){
/*
	// region index
	index = route.at(num)-1;
	//upper.clear();
	//lower.clear();
	if (odd == 0){// return
		
	}
	else if (odd == 1){// no return
		
	}
	else {}
*/

}


double sign(double f){

	if (f>0) return 1.0;
	else if (f<0) return -1.0;
	else return 0.0;

}


/***********************v1.1 correct bound error****************************/
/***********************v1.2 correct change status****************************/
// min = y.at(0) max = y.at(1)
void MaxMinYInOneRegionForX(int region, double *cp, int *sliced_map, GridMap &map, double x, vector<double> &y){
	y.clear();
	int min = C_up;
	int max = C_low;
	bool bstatus = false;

	for (int i = 0; i<map.height; i++){
		//printf("step %d\n", i+1);
		if (sliced_map[i*map.width+int(x)]==region){
			if (min>i) min = i;
			if (max<i) max = i;
			bstatus = true;
		}
	}
	y.push_back(double(min));
	y.push_back(double(max));

	if (!bstatus){

		// closer to the left bound
		if (abs(x-cp[8*(region-1)+0])<=abs(x-cp[8*(region-1)+4])){
			y.at(0) = cp[8*(region-1)+1];
			y.at(1) = cp[8*(region-1)+3];
		}
		// closer to the right bound
		else {
			y.at(0) = cp[8*(region-1)+5];
			y.at(1) = cp[8*(region-1)+7];
		}
	}
	else {
		return;
	}

}

/***********************v1.2 determination of cleaning direction****************************/
// problem of getting x for each y
// min = x.at(0) max = x.at(1)
void MaxMinXInOneRegionForY(int region, double *cp_vertical, int *sliced_map, GridMap &map, double y, vector<double> &x){
	x.clear();
	int min = C_up;
	int max = C_low;
	bool bstatus = false;

	for (int i = 0; i<map.width; i++){
		//printf("step %d\n", i+1);
		if (sliced_map[int(y)*map.width+i]==region){
			if (min>i) min = i;
			if (max<i) max = i;
			bstatus = true;
		}
	}
	x.push_back(double(min));
	x.push_back(double(max));

	if (!bstatus){

		// closer to the up bound
		if (abs(y-cp_vertical[8*(region-1)+1])<=abs(y-cp_vertical[8*(region-1)+5])){
			//x.at(0) = cp_vertical[8*(region-1)+1];
			//x.at(1) = cp_vertical[8*(region-1)+3];
			x.at(0) = cp_vertical[8*(region-1)+0];
			x.at(1) = cp_vertical[8*(region-1)+2];
		}
		// closer to the down bound
		else {
			//x.at(0) = cp_vertical[8*(region-1)+5];
			//x.at(1) = cp_vertical[8*(region-1)+7];
			x.at(0) = cp_vertical[8*(region-1)+4];
			x.at(1) = cp_vertical[8*(region-1)+6];
		}
	}
	else {
		return;
	}

}


void GetPathOneRegionGridOptim(int num, vector<int> &route, double *start_grid, double *end_grid, double *cp, double *surface, Pose *centroid_grid, int step, int *sliced_map, GridMap &map, vector<vector<double> > &part){
	//ShowCardinalPoints(region_num, cp);
	part.clear();
	// region index
	int index = route.at(num)-1;
	vector<vector<double> > upper;
	vector<vector<double> > lower;
	int width = int (cp[8*index+4] - cp[8*index+0]);
	int n = int ((cp[8*index+4] - cp[8*index+0])/step);
	int remain = int ((cp[8*index+4] - cp[8*index+0]))%step;
	printf("width is %d\n", width);
	printf("n is %d\n", n);
	printf("remain is %d\n", remain);
	
	int parity = int(abs(end_grid[3*num+2]-start_grid[3*num+2]))%2;
	printf("difference is %d\n", int(abs(end_grid[3*num+2]-start_grid[3*num+2])));
	printf("parity is %d\n", parity);

	vector<double> coord;
	vector<double> y;

	if (surface[index]<=step*1.0){
		// if surface small enough, simply pass the centroid
		// to cover the region
		coord.clear();
		coord.push_back(start_grid[3*num+0]);
		coord.push_back(start_grid[3*num+1]);
		part.push_back(coord);

		coord.clear();
		coord.push_back(double(centroid_grid[index].x));
		coord.push_back(double(centroid_grid[index].y));
		part.push_back(coord);

		coord.clear();
		coord.push_back(end_grid[3*num+0]);
		coord.push_back(end_grid[3*num+1]);
		part.push_back(coord);

	}

	// surface is big enough to explore
	else {	
		// 
		if (width<=step){
			if (parity==0){
				// one return
				coord.clear();

			}
			else {
				//  parity==1, no need to turn

				coord.clear();
				coord.push_back(start_grid[3*num+0]);
				coord.push_back(start_grid[3*num+1]);
				part.push_back(coord);
				coord.clear();
				MaxMinYInOneRegionForX(route.at(num), cp, sliced_map, map, start_grid[3*num+0], y);
				printf("region %d\n", route.at(num));
				printf("route index %d\n", num);
				if (start_grid[3*num+1]==y.at(1)) {
					coord.push_back(start_grid[3*num+0]);
					coord.push_back(y.at(0));
					part.push_back(coord);
				}
				else {
					coord.push_back(start_grid[3*num+0]);
					coord.push_back(y.at(1));
					part.push_back(coord);
				}
			}
		}
		// width>=step
		else {
			// get one more return
			if (((n%2) != parity && remain >= 1)||((n%2) == parity && remain == 0)){
				printf("we have to turn\n");
				double x[n+2];
				x[0] = start_grid[3*num+0];
				x[1] = start_grid[3*num+0]+0.5*sign(end_grid[3*num+0]-start_grid[3*num+0])*step;
				//printf("sign is %f\n", sign(end_grid[3*num+0]-start_grid[3*num+0]));
				//printf("increment is %f\n", 0.5*sign(end_grid[3*num+0]-start_grid[3*num+0])*step);
				for (int i = 1; i<n; i++){
					x[i] = start_grid[3*num+0]+sign(end_grid[3*num+0]-start_grid[3*num+0])*i*step;
				}
				x[n+1] = end_grid[3*num+0];

				for (int i = 0; i<n+2; i++){
					printf("x[%d]=%f\n", i+1, x[i]);
				}		


			}

			// no need to return
			// (n%2) == parity && remain >= 1)||((n%2) != parity && remain == 0)	
			else{
				printf("no need to turn\n");
				double x[n+1];
				x[0] = start_grid[3*num+0];
				for (int i = 1; i<n+1; i++){
					x[i] = start_grid[3*num+0]+sign(end_grid[3*num+0]-start_grid[3*num+0])*i*step;
				}
				x[n] = end_grid[3*num+0];

				for (int i = 0; i<n+1; i++){
					printf("x[%d]=%f\n", i+1, x[i]);
				}
			}

		}
	}
	
	// step 19, region 17, n = 4, parity = 1
	// step 96, region 59
	// step 94, region 58
	// step 58, region 45, n = 6, parity = 1
	// step 87, region 60, n = , parity = 
	// step 91, region 60, n = 1, parity = 1
	// step 95, region 57, n = 0, parity = 1
	// step 79, region 43, n = 0, parity = 1
	// step 8, region 1, n = 0, parity = 1
	// step 26, region 20, n = 0, parity = 0
	// step 12, region 6, n = 0, parity = 1
	// step 13, region 5, n = 0, parity = 1
	// step 47, region 19, n = 0, parity = 1
	//GetPathOneRegionGridOptim(12, route_best, start_grid, end_grid, cp, surface, centroid_grid, step, sliced_map, map, part);

}



/********************************v1.4 find small regions*************************************/
void FindSmallSurface(double *surface, int region_num, int step, vector<int> &small_surface){
	small_surface.clear();
	for (int i = 0; i<region_num; i++){
		if (surface[i]<=double(step*1.0)){
			small_surface.push_back(i+1);
		}
	}
	//ShowIntegerVector(small_surface);
}
/********************************v1.4 delete small regions*************************************/
void DeleteSmallRegionsFromRoute(vector<int> &route, vector<int> &small_surface, vector<int> &simple_route){
	simple_route.clear();
	for (int i = 0; i<route.size(); i++){
		bool add = true;
		//printf("i = %d\n", i);
		for (int j = 0; j<small_surface.size(); j++){
			//printf("j = %d\n", j);
			if (route.at(i)==small_surface.at(j)){
				//printf("small surface j = %d, region %d\n", j, route.at(i));
				add = false;
				break;
			}
		}
		if (add) {
			simple_route.push_back(route.at(i));
		}
	}
	//ShowIntegerVector(simple_route);
}


/***********************v1.1 reduce the overlapping****************************/
/***********************v1.2 add determination direction****************************/
/***********************v1.3 neglect small region, go once each line****************************/
// primary algorithm for coverage in one region
/*
void GetPathOneRegionGridPrime(int num, vector<int> &route, double *start_grid, double *end_grid, double *cp, double *cp_vertical, double *surface, Pose *centroid_grid, int step, int *sliced_map, GridMap &map, vector<vector<double> > &part){
	//ShowCardinalPoints(region_num, cp);
	part.clear();
	// region index
	int index = route.at(num)-1;
	//printf("we consider region %d\n", route.at(num));
	int width = int (cp[8*index+4] - cp[8*index+0]+1);
	int height = int (cp_vertical[8*index+3] - cp_vertical[8*index+1]+1);
	int n = width/step;
	int remain = width%step;
	int n_vertical = height/step;
	int remain_vertical = height%step;
	//printf("width is %d\n", width);
	//printf("n is %d\n", n);
	//printf("remain is %d\n", remain);
	

	vector<double> coord1, coord2;
	vector<double> x;
	vector<double> y;
	int parity = 0;

	// if region surface too small
	// found surface determination
	if (surface[index]<=double(step*1.0)){
		printf("surface too small in region %d\n", index+1);
		part.clear();
	}

	// surface is big enough to explore
	else {  // determination of explore direction
		if (height>=width){
			// if height is longer than width
			// width = step*n + remain
			// 0 <= remain < step
			// suppose that the right side of the machine works
			// use cardinal points instead
			// simpler

			double coord_x[n+1];
			printf("we go vertically\n");
			printf("in region %d\n", index+1);

			if (remain>0 && remain<=0.5*step){
				printf("case 1\n");
				for (int i = 0; i<n; i++){
					coord1.clear();
					coord2.clear();
					y.clear();
					coord_x[i] = cp[8*index+0]+(i+0.5)*step;
					coord1.push_back(coord_x[i]);
					coord2.push_back(coord_x[i]);
					MaxMinYInOneRegionForX(route.at(num), cp, sliced_map, map, coord_x[i], y);
					parity = (parity + i)%2;

					coord1.push_back(y.at(parity));
					coord2.push_back(y.at(1-parity));
					part.push_back(coord1);
					part.push_back(coord2);
				}

				coord1.clear();
				coord2.clear();
				y.clear();
				coord_x[n] = cp[8*index+4];
				coord1.push_back(coord_x[n]);
				coord2.push_back(coord_x[n]);
				MaxMinYInOneRegionForX(route.at(num), cp, sliced_map, map, coord_x[n], y);
				parity = (parity + n)%2;
				coord1.push_back(y.at(parity));
				coord2.push_back(y.at(1-parity));
				part.push_back(coord1);
				part.push_back(coord2);
			}
			else { //(remain>0.5*step && remain<=step)
				printf("case 2\n");
				for (int i = 0; i<n+1; i++){
					coord1.clear();
					coord2.clear();
					y.clear();
					coord_x[i] = cp[8*index+0]+(i+0.5)*step;
					coord1.push_back(coord_x[i]);
					coord2.push_back(coord_x[i]);
					MaxMinYInOneRegionForX(route.at(num), cp, sliced_map, map, coord_x[i], y);
					parity = (parity + i)%2;
					coord1.push_back(y.at(parity));
					coord2.push_back(y.at(1-parity));
					part.push_back(coord1);
					part.push_back(coord2);
				}

			}

		//vector<vector<double> > part_global;
		//part_global.swap(part);
		//ChangeVectorVectorTypeDoubleFromGridToGlobal(part_global, map);
		//ShowVectorVectorTypeDouble(part_global);

		}

		else {  // height<width
			// determination of explore direction

			// if height is longer than width
			// width = step*n + remain
			// 0 <= remain < step
			// suppose that the right side of the machine works
			// use cardinal points instead
			// simpler

			printf("we go horizontally\n");
			printf("in region %d\n", index+1);

			double coord_y[n_vertical+1];
			if (remain_vertical>0 && remain_vertical<=0.5*step){
				printf("we are here 1\n");
				for (int i = 0; i<n_vertical; i++){
					coord1.clear();
					coord2.clear();
					x.clear();
					coord_y[i] = cp_vertical[8*index+1]+(i+0.5)*step;
					MaxMinXInOneRegionForY(route.at(num), cp_vertical, sliced_map, map, coord_y[i], x);
					parity = (parity + i)%2;
					//printf("we are here 11\n");

					coord1.push_back(x.at(parity));
					coord2.push_back(x.at(1-parity));
					coord1.push_back(coord_y[i]);
					coord2.push_back(coord_y[i]);
					part.push_back(coord1);
					part.push_back(coord2);
				}
				//printf("we are here 22\n");
				coord1.clear();
				coord2.clear();
				x.clear();
				coord_y[n] = cp_vertical[8*index+3];
				MaxMinXInOneRegionForY(route.at(num), cp_vertical, sliced_map, map, coord_y[n], x);
				parity = (parity + n)%2;
				coord1.push_back(x.at(parity));
				coord2.push_back(x.at(1-parity));
				coord1.push_back(coord_y[n]);
				coord2.push_back(coord_y[n]);
				part.push_back(coord1);
				part.push_back(coord2);
			}
			else { //(remain>0.5*step && remain<=step)
				printf("we are there 2\n");
				for (int i = 0; i<n_vertical+1; i++){
					coord1.clear();
					coord2.clear();
					x.clear();
					coord_y[i] = cp_vertical[8*index+1]+(i+0.5)*step;
					MaxMinXInOneRegionForY(route.at(num), cp_vertical, sliced_map, map, coord_y[i], x);
					parity = (parity + i)%2;
					coord1.push_back(x.at(parity));
					coord2.push_back(x.at(1-parity));
					coord1.push_back(coord_y[i]);
					coord2.push_back(coord_y[i]);
					part.push_back(coord1);
					part.push_back(coord2);
				}

			}

		//vector<vector<double> > part_global;
		//part_global.swap(part);
		//ChangeVectorVectorTypeDoubleFromGridToGlobal(part_global, map);
		//ShowVectorVectorTypeDouble(part_global);

		}

	//vector<vector<double> > part_global;
	//part_global.swap(part);
	//ChangeVectorVectorTypeDoubleFromGridToGlobal(part_global, map);
	//ShowVectorVectorTypeDouble(part_global);

	//ShowArrayTypeDouble(coord_x, n+1);
	//ShowVectorVectorTypeDouble(part);
	}
	


}
*/


/***********************v1.1 reduce the overlapping****************************/
/***********************v1.2 add determination direction****************************/
/***********************v1.3 neglect small region, go once each line****************************/
/***********************v1.4 optimize path between regions****************************/
// primary algorithm for coverage in one region
void GetPathOneRegionGridPrime(int num, int cp_num, vector<int> &route, double *cp, double *cp_vertical, double *surface, int step, int *sliced_map, GridMap &map, vector<vector<double> > &part){

	part.clear();
	// region index
	int index = route.at(num)-1;
	//printf("we consider region %d\n", route.at(num));
	int power = 0;
	int parity = 0;

	int width = int (cp[8*index+4] - cp[8*index+0] + 1);
	int height = int (cp_vertical[8*index+3] - cp_vertical[8*index+1] + 1);
	//printf("width is %d\n", width);
	//printf("height is %d\n", height);
	//printf("step length is %d\n", step);

	int n = width/step;
	int remain = width%step;
	//printf("n is %d\n", n);
	//printf("remain is %d\n", remain);

	int n_vertical = height/step;
	int remain_vertical = height%step;
	//printf("n_vertical is %d\n", n_vertical);
	//printf("remain_vertical is %d\n", remain_vertical);
	
	vector<double> coord1, coord2;
	vector<double> x;
	vector<double> y;

	// if region surface too small
	// found surface determination
	if (surface[index]<=double(step*1.0)){
		printf("surface too small in region %d\n", index+1);
		part.clear();
	}

	// surface is big enough to explore
	else {  // determination of explore direction
		if (height>=width){
			// if height is longer than width
			// width = step*n + remain
			// 0 <= remain < step
			// suppose that the right side of the machine works
			// use cardinal points instead
			// simpler

			double coord_x[n+1];
			printf("we go vertically\n");
			printf("in region %d\n", index+1);

			if (cp_num == 1 || cp_num == 4){
				power = cp_num;
			}
			else if (cp_num == 2 || cp_num == 3){
				power = cp_num - 1;
			}
			else {
				printf("wrong cp number\n");
			}
			//printf("power = %d\n", power);
			// initialization parity vertically
			parity = (cp_num - 1)%2;
			//printf("parity = %d\n", parity);



			if (remain>0 && remain<=0.5*step){
				printf("case 1\n");
				printf("n = %d\n", n);
				for (int i = 0; i<n; i++){
					printf("iter %d\n", i);
					coord1.clear();
					coord2.clear();
					y.clear();
					//printf("11111111111111111111111111\n");
					coord_x[i] = (0.5*(pow(-1,(power+1))+1))*cp[8*index+0]+(i+0.5)*pow(-1,(power+1))*step+(0.5*(pow(-1,power)+1))*cp[8*index+4];
					coord1.push_back(coord_x[i]);
					coord2.push_back(coord_x[i]);
					//printf("22221111111111111111111111\n");
					MaxMinYInOneRegionForX(route.at(num), cp, sliced_map, map, coord_x[i], y);
					//printf("33331111111111111111111111\n");
					//printf("y_min=%f, y_max=%f\n", y.at(0), y.at(1));
					//printf("parity = %d\n", parity);
					coord1.push_back(y.at(parity));
					//printf("44441111111111111111111111\n");
					coord2.push_back(y.at(1-parity));
					//printf("55551111111111111111111111\n");
					part.push_back(coord1);
					//printf("66661111111111111111111111\n");
					part.push_back(coord2);
					//printf("77771111111111111111111111\n");
					// update parity
					parity = (parity + 1)%2;
				}

				coord1.clear();
				coord2.clear();
				y.clear();
				//printf("1111\n");
				//coord_x[n] = cp[8*index+4];
				coord_x[n] = (0.5*(pow(-1,power)+1))*cp[8*index+0]+(0.5*(pow(-1,(power+1))+1))*cp[8*index+4];
				coord1.push_back(coord_x[n]);
				coord2.push_back(coord_x[n]);
				//printf("2222\n");
				MaxMinYInOneRegionForX(route.at(num), cp, sliced_map, map, coord_x[n], y);
				coord1.push_back(y.at(parity));
				coord2.push_back(y.at(1-parity));
				part.push_back(coord1);
				part.push_back(coord2);

			}
			else { //(remain>0.5*step && remain<=step)
				printf("case 2\n");
				for (int i = 0; i<n+1; i++){
					coord1.clear();
					coord2.clear();
					y.clear();
					coord_x[i] = (0.5*(pow(-1,(power+1))+1))*cp[8*index+0]+(i+0.5)*pow(-1,(power+1))*step+(0.5*(pow(-1,power)+1))*cp[8*index+4];
					coord1.push_back(coord_x[i]);
					coord2.push_back(coord_x[i]);
					MaxMinYInOneRegionForX(route.at(num), cp, sliced_map, map, coord_x[i], y);
					coord1.push_back(y.at(parity));
					coord2.push_back(y.at(1-parity));
					part.push_back(coord1);
					part.push_back(coord2);
					// update parity
					parity = (parity + 1)%2;
				}

			}

		//vector<vector<double> > part_global;
		//part_global.swap(part);
		//ChangeVectorVectorTypeDoubleFromGridToGlobal(part_global, map);
		//ShowVectorVectorTypeDouble(part_global);

		}

		else {  // height<width
			// determination of explore direction

			// if height is longer than width
			// width = step*n + remain
			// 0 <= remain < step
			// suppose that the right side of the machine works
			// use cardinal points instead
			// simpler

			printf("we go horizontally\n");
			printf("in region %d\n", index+1);

			double coord_y[n_vertical+1];

			if (cp_num == 1 || cp_num == 4){
				parity = cp_num%2;
			}
			else if (cp_num == 2 || cp_num == 3){
				parity = (cp_num - 1)%2;
			}
			else {
				printf("wrong cp number\n");
			}
			//printf("parity = %d\n", parity);

			// initialization parity vertically
			power = cp_num;
			//printf("power = %d\n", power);



			if (remain_vertical>0 && remain_vertical<=0.5*step){
				printf("case 3\n");
				for (int i = 0; i<n_vertical; i++){
					coord1.clear();
					coord2.clear();
					x.clear();
					coord_y[i] = (0.5*(pow(-1,power)+1))*cp[8*index+1]+(i+0.5)*pow(-1,power)*step+(0.5*(pow(-1,(power+1))+1))*cp[8*index+3];
					MaxMinXInOneRegionForY(route.at(num), cp_vertical, sliced_map, map, coord_y[i], x);

					coord1.push_back(x.at(parity));
					coord2.push_back(x.at(1-parity));
					coord1.push_back(coord_y[i]);
					coord2.push_back(coord_y[i]);
					part.push_back(coord1);
					part.push_back(coord2);
					// update parity
					parity = (parity + 1)%2;
				}
				//printf("we are here 22\n");
				coord1.clear();
				coord2.clear();
				x.clear();
				coord_y[n] = (0.5*(pow(-1,(power+1))+1))*cp[8*index+1]+(0.5*(pow(-1,power)+1))*cp[8*index+3];
				MaxMinXInOneRegionForY(route.at(num), cp_vertical, sliced_map, map, coord_y[n], x);
				coord1.push_back(x.at(parity));
				coord2.push_back(x.at(1-parity));
				coord1.push_back(coord_y[n]);
				coord2.push_back(coord_y[n]);
				part.push_back(coord1);
				part.push_back(coord2);
			}
			else { //(remain>0.5*step && remain<=step)
				printf("case 4\n");
				for (int i = 0; i<n_vertical+1; i++){
					coord1.clear();
					coord2.clear();
					x.clear();
					coord_y[i] = (0.5*(pow(-1,power)+1))*cp[8*index+1]+(i+0.5)*pow(-1,power)*step+(0.5*(pow(-1,(power+1))+1))*cp[8*index+3];
					MaxMinXInOneRegionForY(route.at(num), cp_vertical, sliced_map, map, coord_y[i], x);
					coord1.push_back(x.at(parity));
					coord2.push_back(x.at(1-parity));
					coord1.push_back(coord_y[i]);
					coord2.push_back(coord_y[i]);
					part.push_back(coord1);
					part.push_back(coord2);
					// update parity
					parity = (parity + 1)%2;
				}

			}

		//vector<vector<double> > part_global;
		//part_global.swap(part);
		//ChangeVectorVectorTypeDoubleFromGridToGlobal(part_global, map);
		//ShowVectorVectorTypeDouble(part_global);

		}

		//ShowVectorVectorTypeDouble(part);
		//vector<vector<double> > part_global;
		//part_global.swap(part);
		//ChangeVectorVectorTypeDoubleFromGridToGlobal(part_global, map);
		//ShowVectorVectorTypeDouble(part_global);

		//ShowArrayTypeDouble(coord_x, n+1);
		//ShowVectorVectorTypeDouble(part);
	}
	
	//printf("one region prime finished\n");

}

// point in matrix p(x,y) in region
int FindNextNearestCardinalPoint(double x, double y, int region, double *cp){
	int num = 0;
	double dist_min = C_up;
	double dist;
	for (int i = 0; i < 4; i++){
		//printf("step %d\n", i+1);
		dist = GetDistanceBetweenTwoPoints(x, y, cp[8*(region-1)+2*i+0], cp[8*(region-1)+2*i+1]);
		//printf("region %d, cp(%f, %f)\n", region, cp[8*(region-1)+2*i+0], cp[8*(region-1)+2*i+1]);
		if (dist_min > dist) {
			dist_min = dist;
			//printf("dist_min = %f\n", dist_min);
			num = i+1;
		}
	}
	printf("cardinal point number is %d\n", num);
	return num;
}



void ShowArrayTypeDouble(double *array, int num){

	for (int i = 0; i<num; i++){
		printf("array[%d]=%f\n", i+1, array[i]);
	}
}


void ChangeVectorVectorTypeDoubleFromGridToGlobal(vector<vector<double> > &part, GridMap &map){

	for (int i = 0; i<part.size(); i++){
		part.at(i).at(0) = map.x0 + part.at(i).at(0)*map.metersPerPixel;
		part.at(i).at(1) = map.y0 + part.at(i).at(1)*map.metersPerPixel;
	}
}


// num [0, route.size()-1]
void InsideOneRegion(int num, vector<int> &route, double *start, double *end, double *cp, double *surface, 	Pose *centroid_grid, int *sliced_map, GridMap &map, int *visited, vector<vector<double> > &part){
	part.clear();
	// region index
	int index = route.at(num)-1;
	double dist = 0.0;
	// region num is visited, go directly from start to end
	//printf("visited[index]=%d\n",visited[index]);

	// region visited
	if (visited[index] == 1){

		CAstar ap;
		Pose p1;
		Pose p2;

		p1.x = start[3*index+0];
		p1.y = start[3*index+1];
		p2.x = end[3*index+0];
		p2.y = end[3*index+1];
		//printf("p1(%f,%f) p2(%f,%f)\n",p1.x, p1.y, p2.x, p2.y);
		dist = GetDistanceBetweenTwoPoints(p1.x, p1.y, p2.x, p2.y);
		// less than 1 meter, go directly
		if (dist<1){
			vector<double> c1,c2;
			c1.push_back(p1.x);
			c1.push_back(p1.y);
			c2.push_back(p2.x);
			c2.push_back(p2.y);
			part.push_back(c1);
			part.push_back(c2);
		}
		else{
			//ap.plan(p2, map, p1, part);
		}
	}
	// region unvisited
	else {
		
		
		
		
		visited[index] = 1;
	}
}

// p2 is goal, p1 is current position
void RouteBetweenTwoPoses(Pose &p1, Pose &p2, CAstar ap, GridMap &map, vector<vector<double> > &part){

	part.clear();
	//printf("p1(%f,%f) p2(%f,%f)\n",p1.x, p1.y, p2.x, p2.y);
	double dist = GetDistanceBetweenTwoPoints(p1.x, p1.y, p2.x, p2.y);
	// less than 1 meter, go directly
	if (dist<1){
		vector<double> c1,c2;
		c1.push_back(p1.x);
		c1.push_back(p1.y);
		c2.push_back(p2.x);
		c2.push_back(p2.y);
		part.push_back(c1);
		part.push_back(c2);
	}
	else{
		//ap.plan(p2, map, p1, part);
	}

}

void OptimAstarRoute(vector<vector<double> > &inter, vector<vector<double> > &inter_replace){

	inter_replace.clear();
	// ASTAR_OPTIM = 50
	int n = inter.size()/ASTAR_OPTIM;
	int remain = inter.size()%ASTAR_OPTIM;
	inter_replace.push_back(inter.at(0));
	if (remain == 0){
		for (int j = 1; j<n; j++){
			inter_replace.push_back(inter.at(j*ASTAR_OPTIM-1));
		}
	}
	else { // 1<=remain<ASTAR_OPTIM
		for (int j = 1; j<n; j++){
			inter_replace.push_back(inter.at(j*ASTAR_OPTIM+remain-1));
		}
	}
	inter_replace.push_back(inter.at(inter.size()-1));

}

/**************************************v1.3 remove visited regions****************************************/
void DeleteRepteatedRegion(vector<int> &new_route, vector<int> &route, int region_num, int *visited){
	new_route.clear();
	InitializationVisited(region_num, visited);
	int index;
	for (int i = 0; i<route.size(); i++){
		index = route.at(i)-1;
		if (visited[index]==0){
			new_route.push_back(index+1);
			visited[index]=1;
		}
	}
	//ShowIntegerVector(new_route);
	
	
}

/**************************************v1.2 change vertical points****************************************/
/**************************************v1.3 remove visited regions****************************************/
/**************************************v1.4 optimize between regions****************************************/
void AllRegionPrime(vector<int> &route, double *cp, double *cp_global, double *cp_vertical, double *cp_vertical_global, double *surface, int step, int *sliced_map, GridMap &map, vector<vector<double> > &route_global){

	CAstar ap;
	Pose p1;
	Pose p2;
	int index1;
	int index2;
	vector<vector<double> > part;
	vector<vector<double> > inter;
	vector<vector<double> > inter_replace;
	vector<double> end;
	int cp_num;


	part.clear();
	GetPathOneRegionGridPrime(0, 1, route, cp, cp_vertical, surface, step, sliced_map, map, part);
	//ShowVectorVectorTypeDouble(part);
	// end in grid
	end.clear();
	end.push_back(part.at(part.size()-1).at(0));
	end.push_back(part.at(part.size()-1).at(1));
	//printf("end = (%f, %f)\n", end.at(0), end.at(1));

	cp_num = FindNextNearestCardinalPoint(end.at(0), end.at(1), route.at(1), cp);
	//printf("cp number is %d\n", cp_num);

	// part has been changed to global
	ChangeVectorVectorTypeDoubleFromGridToGlobal(part, map);
	ConnectVectorVectorTypeDouble(part, route_global);

	//printf("11111.\n");

	p1.x = part.at(part.size()-1).at(0);
	p1.y = part.at(part.size()-1).at(1);
	p2.x = cp_global[8*(route.at(1)-1)+2*(cp_num-1)+0];
	p2.y = cp_global[8*(route.at(1)-1)+2*(cp_num-1)+1];

	inter.clear();
	RouteBetweenTwoPoses(p1, p2, ap, map, inter);
	inter_replace.clear();
	OptimAstarRoute(inter, inter_replace);
	printf("we pass from region %d to region %d\n", route.at(0), route.at(1));
	ShowVectorVectorTypeDouble(inter_replace);

	ConnectVectorVectorTypeDouble(inter_replace, route_global);
	ShowVectorVectorTypeDouble(route_global);
	//printf("22222.\n");

	if (route.size()<2){
		printf("region number too small.\n");
	}
	for (int i = 1; i<route.size()-1; i++){

		// region index
		index1 = route.at(i) - 1;
		index2 = route.at(i+1) - 1;

		part.clear();
		GetPathOneRegionGridPrime(i, cp_num, route, cp, cp_vertical, surface, step, sliced_map, map, part);
		printf("666666.\n");
		end.clear();
		end.push_back(part.at(part.size()-1).at(0));
		end.push_back(part.at(part.size()-1).at(1));

		// cp number in  region index2+1
		cp_num = FindNextNearestCardinalPoint(end.at(0), end.at(1), index2+1, cp);

		// part has been changed to global
		ChangeVectorVectorTypeDoubleFromGridToGlobal(part, map);
		ConnectVectorVectorTypeDouble(part, route_global);

		//printf("222222.\n");

		p1.x = part.at(part.size()-1).at(0);
		p1.y = part.at(part.size()-1).at(1);
		p2.x = cp_global[8*index2+2*(cp_num-1)+0];
		p2.y = cp_global[8*index2+2*(cp_num-1)+1];
		//printf("333333.\n");
		//printf("333333.\n");

		//printf("p1=(%f,%f), p2=(%f,%f)\n", p1.x, p1.y, p2.x, p2.y);
		//ShowCardinalPoints(18, cp_global);

		inter.clear();
		RouteBetweenTwoPoses(p1, p2, ap, map, inter);
		//printf("444444.\n");

		inter_replace.clear();
		OptimAstarRoute(inter, inter_replace);
		printf("we pass from region %d to region %d\n", index1+1, index2+1);
		//ShowVectorVectorTypeDouble(inter_replace);

		ConnectVectorVectorTypeDouble(inter_replace, route_global);
		//ShowVectorVectorTypeDouble(route_global);

	}

	part.clear();
	GetPathOneRegionGridPrime(route.size()-1, cp_num, route, cp, cp_vertical, surface, step, sliced_map, map, part);
	// part has been changed to global
	ChangeVectorVectorTypeDoubleFromGridToGlobal(part, map);
	ConnectVectorVectorTypeDouble(part, route_global);
	ShowVectorVectorTypeDouble(route_global);



/*
	for (int i = 1; i<route.size(); i++){
	//int i = 0;
		part.clear();
		inter.clear();
		inter_replace.clear();
		// region index
		index1 = route.at(i) - 1;
		index2 = route.at(i+1) - 1;

		//printf("we are in region %d\n", route.at(i));
		// region is unvisited, 
		// so visit the region
		//printf("we are always here %d\n", i);
		printf("in region %d\n", index1+1);
		GetPathOneRegionGridPrime(i, 1, route, cp, cp_vertical, surface, step, sliced_map, map, part);
		ChangeVectorVectorTypeDoubleFromGridToGlobal(part, map);
		ShowVectorVectorTypeDouble(part);

		// v1.4 optimize between regions
		// between two regions
		// have to change into global

		//p1.x = cp_global[8*index1+6];
		//p1.y = cp_global[8*index1+7];
		//p2.x = cp_global[8*index2+2];
		//p2.y = cp_global[8*index2+3];
		p1.x = cp_global[8*index1+4];
		p1.y = cp_global[8*index1+5];
		p2.x = cp_global[8*index2+0];
		p2.y = cp_global[8*index2+1];

		RouteBetweenTwoPoses(p1, p2, ap, map, inter);
		inter_replace.clear();
		OptimAstarRoute(inter, inter_replace);

		printf("we pass from region %d to region %d\n", route.at(i), route.at(i+1));
		ShowVectorVectorTypeDouble(inter_replace);

		ConnectVectorVectorTypeDouble(part, route_global);
		ConnectVectorVectorTypeDouble(inter_replace, route_global);
	}

	// the last region

	GetPathOneRegionGridPrime(route.size()-1, 1, route, cp, cp_vertical, surface, step, sliced_map, map, part);
	ChangeVectorVectorTypeDoubleFromGridToGlobal(part, map);
	ShowVectorVectorTypeDouble(inter_replace);

	ConnectVectorVectorTypeDouble(part, route_global);

	printf("route_global size of %d\n", route_global.size());
	//ShowVectorVectorTypeDouble(route_global);
*/
}



void ShowVectorVectorTypeDouble(vector<vector<double> > part){

	if(!part.empty()) 
	{
			
		int num = part.size();
		
		for (int i=0;i<num;i=i+1)
		{		
			//vector<double> goal = part.at(i);
			//printf("step %d is x = %f, y=%f\n", i+1 , goal.at(0), goal.at(1));
			printf("step %d is x = %f, y=%f\n", i+1 , part.at(i).at(0), part.at(i).at(1));	
		}
	}
	else {
		printf("The vector vector double is empty.\n");
	}

}

void ConnectVectorVectorTypeDouble(vector<vector<double> > &part, vector<vector<double> > &route){

	for (int i = 0; i<part.size(); i++){
		route.push_back(part.at(i));
	}
}

void InitializationVisited(int region_num, int *visited){

	for (int i = 0; i<region_num; i++){
		visited[i] = 0;
	}
}

void ShowVisited(int region_num, int *visited){

	for (int i = 0; i<region_num; i++){
		printf("%d",visited[i]);
	}
	printf("\n");
}

void CoordinatesFromGridToGlobal(double *start, double *end, GridMap &map, vector<int> &route){

	for (int i = 0; i<route.size(); i++){
		//start[3*i+0] = start[3*i+0]*map.metersPerPixel+0.5*map.metersPerPixel+map.x0;
		//start[3*i+1] = start[3*i+1]*map.metersPerPixel+0.5*map.metersPerPixel+map.y0;
		//end[3*i+0] = end[3*i+0]*map.metersPerPixel+0.5*map.metersPerPixel+map.x0;
		//end[3*i+1] = end[3*i+1]*map.metersPerPixel+0.5*map.metersPerPixel+map.y0;
		start[3*i+0] = start[3*i+0]*map.metersPerPixel+map.x0;
		start[3*i+1] = start[3*i+1]*map.metersPerPixel+map.y0;
		end[3*i+0] = end[3*i+0]*map.metersPerPixel+map.x0;
		end[3*i+1] = end[3*i+1]*map.metersPerPixel+map.y0;
	}

}


void RegionCoverage(GridMap &map,Pose &interface,vector<vector<double> > &route_global)
{

	// load map
	// load map
	//GridMap map;
	//map.loadMap(fileName);

	// mapx with 'w'(int 119) and 0
	BYTE mapx[map.width * map.height];
	// obstacle is 'w'(int 119) and free space is 0 not '0'
	SetMap(map.data, mapx, map.width, map.height);
	OutputMapToTxtFile("map.txt", map.width, map.height, mapx);

	// sliced map with region number
	int sliced_map[map.width * map.height];
	int region_num = SliceDecomposition(mapx, map.width, map.height, sliced_map);
	//ShowSliceDecomposition(sliced_map, map.width, map.height);
	//OutputTxtFile("sliced_abc.txt", map.width, map.height, sliced_map);

	//int sliced_map_org[map.width * map.height];
	//CopySliceDecomposition(sliced_map, map.width, map.height, sliced_map_org);

	//int *connectivity_mat = new int(region_num*region_num);
	int connectivity_mat[region_num*region_num];
	RegionConnectivity(sliced_map, map.width, map.height, region_num, connectivity_mat);
	//OutputRegionConnectTxtFile("connect_mat", region_num, connectivity_mat);
	//ComparasonSliceDecomposition(sliced_map_org, sliced_map, map.width, map.height);
	//ShowRegionConnectivity(region_num, connectivity_mat);

	vector<int> route_best;

	// horizontal x
	// vertical y 
	// sliced_map[y*map.width+x]

	Pose centroid_grid[region_num];
	Pose centroid_global[region_num];
	//double region_dist[region_num*region_num];
	//printf("1 region_num = %d \n",region_num);
	//double *region_dist = new double(region_num*region_num);
	double *region_dist = (double*)malloc(region_num*region_num*sizeof(double));

	GetCentroidPoseGrid(map, sliced_map, region_num, centroid_grid);
	//ShowCentroidPose(region_num, centroid_grid);
	ChangeCentroidToGlobal(map, region_num, centroid_grid, centroid_global);
	//ShowCentroidPose(region_num, centroid_global);

	// most time-consuming process: A star algorithm for each distance between regions
	RegionDistance(map, connectivity_mat, region_num, centroid_global, region_dist);
	//ShowRegionDistance(region_num, region_dist);
	// identify connectivity mat and distance mat
	IdentifyConnectDist(region_num, connectivity_mat, region_dist);
	//ComparisonConnectDist(region_num, connectivity_mat, region_dist);
	//ShowRegionDistance(region_num, region_dist);
	
	// using Ant Colony method to get the route between the regions
	// get the global minimum after 5 repetition
	RepeatAntColony(5, region_num, connectivity_mat, region_dist, route_best);
	//OutputRouteTxtFile("best_route.txt", route_best);

	// then for the coverage inside each region
	double cp[8*region_num];
	double cp_global[8*region_num];
	double cp_vertical[8*region_num];
	double cp_vertical_global[8*region_num];
	double bound_lr[2*region_num];
	double bound_ud[2*region_num];
	double surface[region_num];

	InitializationLeftAndRightBound(region_num, bound_lr);
	//ShowBounds(region_num, bound_lr);
	GetLeftAndRightBound(sliced_map, map.width, map.height, region_num, bound_lr);
	//ShowBounds(region_num, bound_lr);
	InitializationUpAndDownBound(region_num, bound_ud);
	//ShowBounds(region_num, bound_ud);
	GetUpAndDownBound(sliced_map, map.width, map.height, region_num, bound_ud);
	//ShowBounds(region_num, bound_ud);
	GetSurface(region_num, bound_lr, bound_ud, surface);
	//ShowSurface(region_num, surface);
	
	// cardinal points under left and right bound 
	InitializationCardinalPoints(region_num, cp, bound_lr);
	RegionCardinalPoints(sliced_map, map.width, map.height, region_num, cp);
	ChangeRegionCardinalPointsFromGridToGlobal(region_num, cp, map, cp_global);
	//ShowCardinalPoints(region_num, cp);
	printf("\n");
	//ShowCardinalPoints(region_num, cp_global);

	// cardinal points vertical under up and down bound 
	InitializationCardinalPointsVertical(region_num, cp_vertical, bound_ud);
	RegionCardinalPointsVertical(sliced_map, map.width, map.height, region_num, cp_vertical);
	ChangeRegionCardinalPointsFromGridToGlobal(region_num, cp_vertical, map, cp_vertical_global);
	//ShowCardinalPoints(region_num, cp_vertical);
	//ShowCardinalPoints(region_num, cp__vertical_global);

	//ReadRouteTxtFile("best_route96.txt", route_best);
	// get rid of the last repetitive vector element
	//route_best.pop_back();
	//ShowIntegerVector(route_best);
	//printf("The size of the route vector is %d.\n", route_best.size());
	


	//vector<vector<double> > route_global;	
	vector<vector<double> > part;
	vector<vector<double> > part_replace;
	CAstar astar;

	// remain to get from outside
	//Pose interface;
	//interface.x = cp_global[8*(route_best.at(0)-1)+6];
	//interface.y = cp_global[8*(route_best.at(0)-1)+7];	
	Pose begin;
	begin.x = cp_global[8*(route_best.at(0)-1)+2];
	begin.y = cp_global[8*(route_best.at(0)-1)+3];
	RouteBetweenTwoPoses(interface, begin, astar, map, part);
	OptimAstarRoute(part, part_replace);
	ConnectVectorVectorTypeDouble(part_replace, route_global);

	int visited[region_num];
	InitializationVisited(region_num, visited);
	//ShowVisited(region_num, visited);
	vector<int> new_route;
	DeleteRepteatedRegion(new_route, route_best, region_num, visited);
	//ShowIntegerVector(new_route);

	int step = 2*CLEAN_WIDTH/map.metersPerPixel;
	vector<int> small_surface;
	FindSmallSurface(surface, region_num, step, small_surface);
	ShowIntegerVector(small_surface);
	printf("111\n");
	FindSmallSurface(surface, region_num, 2*step, small_surface);
	ShowIntegerVector(small_surface);
	vector<int> simple_route;
	DeleteSmallRegionsFromRoute(new_route, small_surface, simple_route);
	ShowIntegerVector(simple_route);

	AllRegionPrime(simple_route, cp, cp_global, cp_vertical, cp_vertical_global, surface, step, sliced_map, map, route_global);
	OutputRouteToTxtFile("route_global.txt", route_global);
	free(region_dist);

	//FindNextNearestCardinalPoint(130, 69, 15, cp);
	//GetPathOneRegionGridPrime(6, 1, simple_route, cp, cp_vertical, surface, step, sliced_map, map, part);
	//ShowVectorVectorTypeDouble(route_global);
	//printf("%d\n", step);

	//vector<double> y;
	//MaxMinYInOneRegionForX(10, cp, sliced_map, map, 120.0, y);
	//printf("y(0) = %f, y(1) = %f\n", y.at(0), y.at(1));


	//vector<double> x;
	//MaxMinXInOneRegionForY(10, cp_vertical, sliced_map, map, 12.0, x);
	//printf("x(0) = %f, x(1) = %f\n", x.at(0), x.at(1));


	//delete connectivity_mat;
	//delete region_dist;


	//printf("6/4=%d\n",6/4);
	//printf("6%4=%d\n",6%4);


	//double t = GetDistanceBetweenTwoPoints(1.0,2.0,2.0,1.0);
	//printf("%f\n", t);

	//if (map.data[0]==0){
	//	printf("You are right free!\n");
	//}
	//if (map.data[0]==255){
	//	printf("You are wrong wall!\n");
	//}
	//printf("Good job for region route!\n");


	//double d = pow(1.5,-2);
	//printf("%.10f\n", d);
}
