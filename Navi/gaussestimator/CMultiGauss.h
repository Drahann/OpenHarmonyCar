#pragma once
#include "CMatrix.h"

#define PI 3.1415926

class MultiGaussian
{
public:
	MultiGaussian( double** P, int row, int col );
	MultiGaussian( CMatrix P );
	MultiGaussian( double** P, int row, int col, double* u, int len );
	MultiGaussian( CMatrix* P, double* u, int len );
	~MultiGaussian();
public:
	double* m_u; // mean

	/** The covariance matrix **/
	CMatrix m_P;
	CMatrix m_Pinv;
	double m_Pdet;
	double m_Pscale; // the coefficient out front.
	double m_logPscale; // log of Pscale
	/** right triangular factor from cholesky factorization **/
	CMatrix m_L;
	int m_n; // number of variables

	int getDimension();
	void sample( double* pData );
	double prob(double* v, int len);
	CMatrix getPinv(){ return m_Pinv;};
	double chi2(double* v, int len);
	double logProb(double* v, int len);
	CMatrix getCovariance(){ return m_P;};
	void getCovariance(CMatrix* pMatrix ){ m_P.copy(pMatrix);};
	void getMean( double* p, int len ){ 
		//assert( len==m_n );
		//memcpy_s( p, sizeof(double)*len, m_u, sizeof(double)*len );
		memcpy( p, m_u, sizeof(double)*len );
	};
	double getMahalanobisDistance(double* x, int len);
};
