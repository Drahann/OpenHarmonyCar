#pragma once

#include <vector>
#include <string>
#include <map>
#include <math.h>


using namespace std;

class KLD
{
	
	int absolute_min;
	int sample_min;
	double confidence;
	double max_error;
	double bin_size;
	vector<double> ztable;
	map<string,int> bins;
	
	int support_samples;
	int kld_samples;
	double zvalue;
	string ztableFile ;
	int maxBit;
	int maxSz;



public:
	KLD(void);
	KLD(double quantile, double err, double bsz, int samplemin, string name);
	~KLD(void);

	int update(vector<double> &sample);
	void reset();
	string convertToString(vector<int> vtindex);
	bool in_empty_bin(vector<double> &sample);
	//void build_table();
	//double[] get_sample(double mean, double var);

	bool build_table();

	
};

