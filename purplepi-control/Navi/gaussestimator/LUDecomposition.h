#pragma once

#include "CMatrix.h"

class LUDecomposition
{
public:
	//LUDecomposition();
	LUDecomposition( CMatrix A );
	LUDecomposition( CMatrix* A, bool autoPivot );

	~LUDecomposition();
private:
	double signum( double v );
	bool isSingular();
public:
	CMatrix m_LU;

	int m_pivsign;
	int* m_piv;

	double det();
	double logDet();
	CMatrix solve(CMatrix B);
};
