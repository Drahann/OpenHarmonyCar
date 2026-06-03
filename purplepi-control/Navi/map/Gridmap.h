#pragma once

#include "../type.h"

#include <vector>
#include <assert.h>
#include "UnionFindSimple.h"
#include "../math/MathUtil.h"
#include <vector>
using namespace std;

class particles
{
public:
	particles(void);
	~particles(void);

	Pose pose;
	double weight;
	double tempweight;
	double prop;
};



class LUT
{
public:
    double metersPerPixel;   //每个像素点代表的空间大小

   // int *plut;
	vector<int> vtlut;
	int length;
};

class IntArray2D
{
public:
    int *vs;
	int *vd;
    int m_dim1, m_dim2;

    IntArray2D(int dim1, int dim2)
    {
		vs = NULL;
		if(dim1>0 && dim2>0)
		{
			m_dim1 = dim1;
			m_dim2 = dim2;

			vs = new int[m_dim1*m_dim2];           //数组
			vd = new int[m_dim1*m_dim2];
			memset(vs,0,sizeof(int)*m_dim1*m_dim2);     // m_dim1*m_dim2 个int的大小
			memset(vd,0,sizeof(int)*m_dim1*m_dim2);
		}
    };

	~IntArray2D(void)
	{
		if(vs!=NULL)
			delete [] vs;
		if(vd != NULL)
			delete [] vd;
	};


    inline int get(int d1, int d2)
    {
        return vs[d1*m_dim2 + d2];
    };

	inline int getvd(int d1, int d2)
	{
		 return vd[d1*m_dim2 + d2];
	}

    inline int set(int d1, int d2, int v)
    {
        vs[d1*m_dim2 + d2] = v;
		return 1;
    };
	inline int Getdim1()
	{
		return m_dim1;
	}
	inline int Getdim2()
	{
		return m_dim2;
	}

    void plusEquals(int d1, int d2, int v)
    {
        vs[d1*m_dim2 + d2] += v;
		if( v > 0)
		{
			vd[d1*m_dim2 + d2] += 1;
		}
    };
};

class ProbMap
{
	public:
		ProbMap(void);
		~ProbMap(void);

		void setValue(double x, double y, double v);
		void drawLine(double xa, double ya, double xb, double yb, double fill);
		void makePixels(double dx0, double dy0, int nwidth, int nheight, double dmetersPerPixel, double ndefaultFill, bool broundUpDimensions);
		void setValueIndexSafe(int ix, int iy, double v);
		void fill(double v);
		double sq(double v);
		
	public:
		
		double x0, y0;        // minimum x, y (lower-left corner of lower-left pixel);
		double metersPerPixel;

		int    width, height; // in pixels. Always a multiple of four.
		double   *data;

		double   defaultFill;

};

class GridMap
{
public:
	GridMap(void);
	~GridMap(void);

	inline int toIndexX(double x){
    	return (int)((x-x0)/metersPerPixel);
    };
    
    inline int toIndexY(double y){
    	return (int)((y-y0)/metersPerPixel);
    };
    
   inline double toX(int x){
    	return x*metersPerPixel+x0;
    };
    
    inline double toY(int y){
    	return y*metersPerPixel+y0;
    };

	void makeMeters(double dx0, double dy0, double sizex, double sizey, double dmetersPerPixel, int ndefaultFill);

	void makePixels(double dx0, double dy0, int nwidth, int nheight, double dmetersPerPixel, int ndefaultFill, bool broundUpDimensions);
	void cropMeters(double xmin, double ymin, double _width, double _height, bool roundUpDimensions,GridMap &gm);
	void cropPixels(int xmin, int ymin, int _width, int _height, bool roundUpDimensions,GridMap &gm);
	void crop(bool roundUpDimensions,GridMap &gm);
	void resizeMeters(double x0_m, double y0_m, double width_m, double height_m, bool roundUpDimensions);
	void resizePixels(int xmin, int ymin, int _width, int _height, bool roundUpDimensions,GridMap &gm);
	void resizePixels(int xmin, int ymin, int _width, int _height, bool roundUpDimensions);

	void clear();


    /** Write the provided value to every grid element **/
	void fill(int v);


    /** Map every current value of the gridmap to a new value. The values array should be 255. **/
	void map(BYTE values[]);

    /** Modify data with all new values. **/
	void setData(BYTE values[],int valuelengh);

	void copy(GridMap &gm);

	GridMap& operator=(GridMap &gm);

	double average();
	double sq(double v);
	int sgn(double v);
	void arraySet(BYTE d[], int offset, int length, BYTE value);
	void arrayMove(BYTE d[], int srcoffset, int destoffset, int len);
	void recenter(double cx0, double cy0, double maxDistance);

	double evaluatePath(Pose xy0, Pose xy1, bool negativeOn255) ;
	double evaluatePath(vector<Pose> &xys, bool negativeOn255);


	int getValue(double x, double y);
	int getValueIndex(int ix, int iy);
	int getValueIndexSafe(int ix, int iy);
	int getValueIndexSafe(int ix, int iy, int def);

	void drawCircleMax(double cx, double cy, double r, BYTE fill);
	void drawCircle(double cx, double cy, double r, BYTE fill);
	void drawLine(double xa, double ya, double xb, double yb, BYTE fill);
	void drawLineMax(double xa, double ya, double xb, double yb, BYTE fill);
	void drawLineInterpolate(double xa, double ya, double xb, double yb, int f0, int f1);

	 void getXY0(double &dx0, double &dy0);
	 void getXY1(double &dx1, double &dy1);
	 void setValue(double x, double y, BYTE v);
	 void setValueIndex(int ix, int iy, BYTE v);
	 void setValueIndexSafe(int ix, int iy, BYTE v);
	 bool hasNeighbor(int ix, int iy, BYTE v);
	 void edges(BYTE v, GridMap &gm);
	 void dilate(BYTE v, int iterations,GridMap &dest);
	 void scale(double s);
	 void subtract(int v);
	 bool isCompatible(GridMap &gm);
	 void plusEquals(GridMap &gm);
	 void maxEquals(GridMap &gm);
	 void fill(int ix, int iy, BYTE v, int tolerance);
	 void fillDown(int ix0, int ix1, int iy, BYTE v, int tolerance);
	 void fillUp(int ix0, int ix1, int iy, BYTE v, int tolerance);

	 void makeGaussianLUT(double scale, double cliffDistMeters, double expDecayMSq, LUT &lut);
	 void makeQuadraticLUT(double scale, double stddev, LUT &lut);
	 void makeLinearReverseLUT(LUT &lut);
	 void makeConstantLUT(int v, double width_meters, LUT &lut);
	 void drawDot(double x, double y, LUT& lut,int lutlength);
	 void drawLine(double x0, double y0, double x1, double y1, LUT& lut);
	 void drawRectangle(double cx, double cy,
                              double x_size, double y_size,
                              double theta,
                              LUT& lut,int lutlength);
	 int clamp(int v, int min, int max);
	 void maxConvolution(int k, GridMap& gm);
	 void maxConvolution(BYTE in[], int in_offset, int width, int k, BYTE out[], int out_offset);
	 void maxConvolution(BYTE in[], int in_offset, int width, int k, BYTE out[], int out_offset, int hist[],int histlength);
	 void decimateMax(int factor,GridMap& gm );
	 void max4(GridMap &gm);
	 int scoreThreshold(vector<Pose> &points,
                              double tx, double ty, double theta,
                              int thresh);
	 double score(vector<Pose> &points,
                     double tx, double ty, double theta, Pose& prior, double **pinv);
	 void scores3D(vector<Pose> &points,
                                  double tx0, int txDim,
                                  double ty0, int tyDim,
                                  double theta0, double thetaStep, int thetaDim,
                                   Pose& priorxyt, double **pinv,vector<IntArray2D*> &vtIntArray2D);
	 void HistogramFilter_scores3D(vector<Pose> &points,
                                  double tx0, 
                                  double ty0, 
                                  double theta0, double thetaStep, int thetaDim,
                                  vector<IntArray2D*> &vtIntArray2D);
	 IntArray2D* scores2D(vector<Pose> &points,
                               double tx0, int txDim,
                               double ty0, int tyDim,
                               double theta,  Pose& priorxyt, double **pinv);
	 IntArray2D* HistogramFilter_scores2D(vector<Pose> &points,
                                                      double tx0, 
                                                      double ty0,
                                                      double theta);

     void pfscores(vector<Pose> &points,vector<particles> &pfswarm);

	 int getConnectedWithin(Pose& xy, int maxCost,BYTE **pRes, int &nResLength);
 	 int getConnectedWithin(int x,int y, int maxCost,BYTE **pRes, int &nResLength);
	 bool getPixelCenter(int indices[],int length,double *pxRes, double *pyRes);
	 bool getPixelCenter(int ix, int iy, double *pxRes, double *pyRes);
	 bool getIndices(double p[],int length, int *pix,int *piy);
	 bool getIndices(double px, double py, int *pix,int *piy);
	 void incrementSaturate(double px, double py);
     void thresholdGreaterThanOrEqual(int thresh, BYTE vtrue, BYTE vfalse);
	void BorderMap(GridMap &bordmap);
	int getConnectedWithin(POINT cur, int maxCost,POINT  next );
	void RotateMap(double theta);

public:
    double x0, y0;        // minimum x, y (lower-left corner of lower-left pixel);
    double metersPerPixel;

    int    width, height; // in pixels. Always a multiple of four.
    BYTE   *data;

    BYTE   defaultFill;


};

