#pragma once
#include "Vec.h"
#include "define.h"


class CMatrix
{
public:
	CMatrix();
	CMatrix( int Row, int Col, int Option=0 );
	CMatrix( double** pV, int Row, int Col );
	~CMatrix();

public:
	static void GetRandArray( double* pArray, int len );
public:
	const static int DENSE = 0;
	const static int SPARSE = 1;

	Vec* m_pVec;
	CDenseVec* m_pVec_Dense;
	CSRVec* m_pVec_CSR;
	int m_Row;
	int m_Col;
	int m_Option;
	bool m_bMode_DE;

	Vec makeVec(int length);
	CDenseVec makeVec( double* p, int length );
	CSRVec makeVec_CSR( int length );
	CDenseVec makeVec_Dense( int length );
	static CMatrix diag(double* pV, int length);
	static CMatrix outerProduct(double* x, int len_x, double* y, int len_y);
	static void outerProduct(double* x, int len_x, double* y, int len_y, CMatrix* pMatrix);
	CMatrix copy();
	void copy( CMatrix* );
	void copyArray( map<int,  double>* pData );
	static CMatrix identity(int m, int n);
	static CMatrix random(int m, int n);
	int getRowDimension();
	int getColumnDimension();
	int getOptions();
	double get( int pos );
	double get(int row, int col);
	void set( int pos, double v );
	void set( int row, int col, double v);
	void set( int row, int col, double** pv, int v_row, int v_col );
	Vec getColumn(int col);
	void getColumn_Dense( int col, CDenseVec* pVec );
	CSRVec getColumn_CSR( int col);
	Vec getRow(int row);
	void getRow_Dense( int row, CDenseVec* pVec );
	CSRVec getRow_CSR( int row);
	void setRow(int row, Vec v);
	void setRow_dense( int row, CDenseVec* v );
	bool isSparse();
	int getNz();
	double getNzFrac();
	void clear(){
		for (int i = 0; i < m_Row; i++)
		{
			Vec* p = &(m_pVec[i]);
			p->clear();
		}
	};
	bool equals(CMatrix M);
	bool equals(CMatrix M, double eps );
	CMatrix times(double v);
	void times( double v, CMatrix* pMatrix );
	CMatrix times(CMatrix B);
	void times(double* B, int len, double* pDest);
	CMatrix plus(CMatrix B);
	void plus( CMatrix* B, CMatrix* pDestMT );
	void plusEquals(int i, int j, double v);
	void plusEquals(double** pV, int row, int col);
	void plusEquals(int i0, int j0, double** pV, int row, int col);
	void plusEquals(CMatrix M);
	void plusEquals(int i0, int j0, CMatrix M);
	CMatrix copyPermuteRowsAndColumns(int* perm, int len);
	void inversePermuteRows(int* pivot, int len);
	void permuteRows(int* perm, int len);
	CMatrix transpose();
	CMatrix upperRightTranspose();
	void upperRightTranspose( CMatrix* pMatrix );
	//CMatrix upperRight();
	void upperRight(CMatrix* pMatrix);
	//CMatrix inverse();
	void inverse(CMatrix* pMatrix );
	void swapRows(int a, int b);
	void timesEquals(double v);
	void timesEquals(int i, int j, double v);
	CMatrix solve(CMatrix B);
	double det();
	double logDet();
};
