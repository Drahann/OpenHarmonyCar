
#include "CholeskyDecomposition.h"
#include <math.h>

/*<FUNC+>*******************************************************
 * 函数名称: BOOL RowStyle:A中的每一行是否属于CSRVec
 * 功能描述: 
 * 输入参数: RowStyle:TRUE--矩阵A中的每一行为CSRVec队列
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/04        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CholeskyDecomposition::CholeskyDecomposition (CMatrix A, bool RowStyle)
{
	CholeskyDecomposition( &A, RowStyle, false );
}

CholeskyDecomposition::CholeskyDecomposition (CMatrix* A, bool RowStyle, bool bBose )
{
	int m = A->m_Row, n = A->m_Col;
	m_isVerbose = bBose;

	CSRAllocator csralloc;

	CMatrix U;
	A->upperRight(&U);
	m_isSpd = (m == n);
	for (int i = 0; i < n; i++) {

		//if (m_isVerbose && n > 1000 && i%50==0)
			//System.out.printf("%d / %d\r", i, n);
		//Vec Urowi = U.getRow(i);
		//double d = sqrt(Urowi.get(i));
		//m_isSpd &= (d>0);
		//Urowi.timesEquals(1.0/d, i, n-1); // or the whole row  //wang.lei addto normal branch,but not csrvec branch
		if (!RowStyle) {
			// special case sparse row to avoid having to
			// iterate over the whole row. (big win!)
			//CSRVec* uvv = (CSRVec*)(void*) &Urowi;

			//for (int pos = 0; pos < uvv->m_nz; pos++) {
			//	int j = uvv->m_pIndices[pos];
			//	if (j >= i+1) {
			//		double s = uvv->m_pValue[pos];

			//		CSRVec* Urowj = (CSRVec*)(void*) &(U.getRow(j));

			//		Urowi.addTo(*Urowj, -s, j, n-1); // or the whole row
			//	}
			//}
		} else {
			// default case.
			CDenseVec Urowi(U.m_Col);
			U.getRow_Dense(i, &Urowi);
			double d = sqrt(Urowi.get(i));
			m_isSpd &= (d>0);
			Urowi.timesEquals(1.0/d, i, n-1); // or the whole row
			U.setRow_dense( i, &Urowi );
			for (int j = i+1; j < n; j++) {
				double s = Urowi.get(j);
				if (s==0)
					continue;
				CDenseVec Urowj(U.m_Col);
				U.getRow_Dense(j, &Urowj);
				Urowi.addTo(Urowj, -s, j, n-1); // or the whole row
				U.setRow_dense( j, &Urowj );
			}
		}
	}

	//m_L = U.upperRightTranspose();
	U.upperRightTranspose(&m_L);

	if (m_isVerbose && n > 1000) {
		double nzfrac = ((double) m_L.getNz()) / pow((double)(m_L.getRowDimension()),2);
		//System.out.println("L size: "+L.getRowDimension()+" nz: "+L.getNz()+" (%): "+nzfrac*100);
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: getL
 * 功能描述: 获取m_L矩阵
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/05        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CMatrix CholeskyDecomposition::getL ()
{
	return m_L;
}

void CholeskyDecomposition::getL( CMatrix* pMatrix )
{
	m_L.copy( pMatrix );
}

/*<FUNC+>*******************************************************
 * 函数名称:
 * 功能描述: Computes the determinant by squaring the product of the diagonals of L
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/05        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double CholeskyDecomposition::det ()
{
	if (!m_isSpd)
		return 0;

	double det = 1.0;
	for (int i = 0; i < m_L.m_Row; i++)
		det *= m_L.get(i,i);

	return det*det;
}

/*<FUNC+>*******************************************************
 * 函数名称: solve
 * 功能描述: 处理过程添加
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/05        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CMatrix CholeskyDecomposition::solve (CMatrix B)
{
	int m = m_L.m_Row, n = m_L.m_Col;
	//if (B.m_Row != m)
	//	throw new IllegalArgumentException("Matrix row dimensions must agree.");
	//assert( B.m_Row==m );

	//if (!isSpd)
	//{
	//	throw new RuntimeException("Matrix is not SPD");
	//}
	//assert( m_isSpd );

	//	erp.util.Tic tic = new erp.util.Tic();

	// Solve L*Y = B
	CMatrix* YT = new CMatrix(B.getColumnDimension(), B.getRowDimension());

	// most of the time, B will have one column.
	for (int cidx = 0; cidx < B.getColumnDimension(); cidx++) {

		for (int ridx = 0; ridx < B.getRowDimension(); ridx++) {
			double dot = m_L.getRow(ridx).dotProduct(YT->getRow(cidx));
			double err = (B.get(ridx,cidx) - dot) / m_L.get(ridx,ridx);

			YT->set(cidx,ridx, err);
		}
	}

	CMatrix* XT = new CMatrix(B.getColumnDimension(), B.getRowDimension());
	CMatrix LT = m_L.transpose();

	for (int cidx = 0; cidx < B.getColumnDimension(); cidx++) {

		for (int ridx = B.getRowDimension()-1; ridx >= 0; ridx--) {
			double dot = LT.getRow(ridx).dotProduct(XT->getRow(cidx));
			double err = (YT->get(cidx,ridx) - dot) / m_L.get(ridx,ridx);

			XT->set(cidx,ridx, err);
		}
	}
	
	CMatrix ResX = XT->transpose();

	delete YT;
	delete XT;
	return ResX;
}
