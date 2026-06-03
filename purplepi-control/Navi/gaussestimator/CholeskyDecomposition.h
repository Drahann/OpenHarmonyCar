#pragma once
#include "CSRAllocator.h"
#include "CMatrix.h"

class CholeskyDecomposition
{
public:
	CholeskyDecomposition(CMatrix A, bool RowStyle);
	CholeskyDecomposition(CMatrix* A, bool RowStyle, bool bBose);

public:
	CMatrix m_L;
	CMatrix m_U;/* contains L' and garbage in the lower diagonal */

	bool m_isSpd;/* 是否是方阵的标志 */
	bool m_isVerbose;

	CMatrix getL();
	void getL( CMatrix* pMatrix );
	bool isSPD(){ return m_isSpd;};
	double det();
	CMatrix solve(CMatrix B);
};
