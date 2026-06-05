#pragma once
#include "CMultiGauss.h"

class CMultiGaussionEstimator
{
public:
	CMultiGaussionEstimator( int n );
	~CMultiGaussionEstimator();

public:
	void observeWeighted(double* v, int len, double prob);

public:
	int m_num;
	double* m_pU;
	double** m_pP;
	double m_obsCount;
public:
	void observe( double* v, int len );
	MultiGaussian* getEstimate( /*MultiGaussian* pG, */bool unbiased=FALSE );


};
