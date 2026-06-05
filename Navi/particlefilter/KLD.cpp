
#include "KLD.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <time.h>
#include <sys/time.h>

KLD::KLD(void)
{
	absolute_min=10;
	ztableFile = "e:/ztable.data";
	maxBit = 3;
	maxSz = 99;
}

KLD::KLD(double quantile, double err, double bsz, int samplemin, string name)
{
		if (name != " ")
			ztableFile = name;
		sample_min = samplemin;
		if (sample_min < absolute_min)
			kld_samples=absolute_min;
		else kld_samples=sample_min;

		
		quantile = quantile>=0?quantile:0;
		quantile = quantile<0.999?quantile:.999; //限制在0-0.999
		confidence=quantile-0.5; // ztable is from right side of mean
		confidence=min(0.49998,(double)(max((double)0,confidence)));//限制在0-0.49998

		max_error=err; //0.5
		bin_size=bsz;  //0.5

		zvalue=4.1f;
		build_table();
 		for (int i=0; i<ztable.size();i++)
 			if(ztable.at(i) >= confidence) {
 				zvalue=i/100.0f;
 				break;
 			}
		zvalue = 3.0899999f;

}
KLD::~KLD(void)
{
}

int KLD::update(vector<double> &sample){
		if (in_empty_bin(sample)) 
		{
			support_samples++;
			
			if (support_samples >=2) {
				int k=support_samples-1;
				k=(int)ceil(  (double)(k)/(2*max_error)*  pow( 1-2.0/(9.0*k)+sqrt(2.0/(9.0*k))*zvalue,  3 )   );
				if (k > kld_samples)
					kld_samples=k;
			}
		}
		
		return kld_samples;
	}
	
void KLD::reset(){
		if (sample_min < absolute_min)  //absolute_min = 10
			kld_samples=absolute_min;
		else kld_samples=sample_min;
		support_samples=0;
		bins.clear();
	}


string KLD::convertToString(vector<int> vtindex)
{
		string s;
		int i =0;
	struct timeval tpstart;

	gettimeofday(&tpstart,NULL);
	srand(tpstart.tv_usec);

	
		for(int i=0; i<vtindex.size();i++)
		{
			char ch[50];
			int num = vtindex.at(i);
			maxSz = 99;
			int res = 0;

			if(num>maxSz)
			{
				res = maxSz;
			}
			else
			{
				res = num;

			}

			sprintf(ch, "%03d",res);
			s.append(ch);	
			//printf("  s  : %s\n", s.c_str());

		}	 
		return s;
}
		  
bool KLD::in_empty_bin(vector<double> &sample)
{
		vector<int> binIdx ;
		  
		for (int i=0; i<sample.size(); i++)
		{
			binIdx.push_back((int)floor(sample.at(i)/bin_size));//bin_size = 0.5

		}
			  
		string s = convertToString(binIdx);

		if(bins.count(s))
		{
			return false;
		}else 
		{
			//system.out.println(index+" is new "+convertToString(binIdx));
			bins.insert(make_pair(s,0));
		}
		return true;
	}

bool KLD::build_table()
{
	ifstream infile;
	infile.open("/mnt/cf/mapfile/ztable.data",ios::in);

	if(!infile)
		return false;
	
	double data;

	while(!infile.eof())
	{		
			infile>>data;
			ztable.push_back(data);	
	}
	
	infile.close();

	printf("read ztable over\n");
	return true;


}
	/*
void KLD::build_table()
{
		try{
			FileReader fr = new FileReader(this.ztableFile);
			BufferedReader ins = new BufferedReader(fr);
		    String line = null;
		    ArrayList<String> input = new ArrayList<String>();		
		    while ((line = ins.readLine()) != null) {
		    	if (line.startsWith("#") || line.trim().length()==0)
		    		continue;
		    	input.add(line);
		    }
		    String[]toks;
		    ztable = new vector<double>();
		    for(int i=0;i<input.size();i++)
			{
				String s=input.get(i);
				toks = s.split("\\s+");
				for(int iin=1;iin<toks.length;iin++){
					ztable.add(Double.parseDouble(toks[iin]));
				}
			}
		}catch(IOException e){System.out.println(e);}
	}	
	
static double[] KLD::get_sample(double mean, double var){
		return new double[]{mean+new Random().nextGaussian()*var,
				mean+new Random().nextGaussian()*var,
				mean+new Random().nextGaussian()*var};
	}
*/
