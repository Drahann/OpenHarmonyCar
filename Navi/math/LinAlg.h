#pragma once

#include <math.h>
#include "../type.h"

#include <vector>

using namespace std;


class LinAlg
{
public:
	LinAlg(void);
	~LinAlg(void);

	static double sq(double v);
	static int sq(int v);

	static Pose transform(Pose &xyt, Pose &p);
	static void transform(Pose &T, vector<Pose> &points, vector<Pose> &respoints);
	static void transform(double T[4][4], vector<vector<double> > &points, vector<vector<double> > &newpoints);
	static void  transform(double T[4][4], double p[],int npLength, vector<double> &res);
	static void  transform(double T[4][4], vector<vector<double> > &points, vector<Pose> &newpoints);
	static double DistancePose(Pose a, Pose b);
	static double magnitude(int a[],int size);
	static void resize(double v[], int vLength,int newlength,double *pRes,int *pResLength);
   // static int min(int a, int b);
	static void  quatPosToMatrix(double q[], double pos[],double M[4][4]);

	static Pose xytInvMul31(Pose &a, Pose &b);
   

    /** compute:  X = xytMultiply(xytInverse(a), b) **/
	static Pose xytInvMul31(Pose &a, Pose &b, Pose &res);
	 static Pose xytMultiply(Pose a, Pose b);

	static double manhattenDistance(double* a, int a_len, double* b, int b_len);


	static int squaredDistance(int* a, int* b, int len);
	static double squaredDistance(double* a, double* b, int len);
	static double distance(int* a, int* b, int len );
	static double distance(double* a, double* b, int len );
	static void Arrabs( double** pSrc, double** pDest, int Row, int Col );
	static void Arrabs( double* pSrc, double* pDest, int len );
	static void Arrabs( float* pSrc, float* pDest, int len );
	static double normF(double* a, int len );
	template <typename T>
	static double magnitude(T* a, int len );
	template <typename T>
	static T average(T* a, int len );
	template <typename T>
	static T normL1(T* a, int len );
	template <class T>
	static void scale(T* pSrc, int len, T* pDest, double* scaled);
	static void scale(double* pSrc, int len, double* pDest, double scaled);
	template <class T>
	static void scale(T** pSrc, int Row,int Col, T** pDest, double scaled);
	template <class T>
	static void normalizeL1(T* a, int len, T* pDest );
	template <class T>
	static void normalize(T* a, int len, T* pDest );
	template <typename T>
	static void normalizeEquals(T* a, int len );
	static void add( double* a, double* b, int len, double* pSum );
	static void add( double** a, double** b, int Row, int Col, double** pSum );
	static void subtract( double** a, double** b, int Row, int Col, double** pSubt );
	static void subtract(double* a, double* b, int len, double* pSum);
	template <typename T>
	static void copy( T* pSrc, int len, T* pDest );
	static double dotProduct(double* a,int len, double* b);
	static double GetGuassian();

};

