
#include "LUDecomposition.h"
#include "../math/LinAlg.h"
#include <math.h>

//LUDecomposition::LUDecomposition()
//	:m_piv(NULL)
//	,m_pivsign(0)
//{
//
//}

LUDecomposition::LUDecomposition( CMatrix A )
{
	LUDecomposition(&A,TRUE);
}

LUDecomposition::LUDecomposition( CMatrix* A, bool autoPivot )
{
	int i = 0;
	int j = 0;

	//m_LU = A.copy();
	A->copy( &m_LU );
	int m = m_LU.m_Row, n = m_LU.m_Col;

	m_pivsign = 1;
	m_piv = new int[m];

	for( i=0; i<m; i++ )
	{
		m_piv[i] = i;
	}

	for( j=0; j<n; j++ )
	{
		//Vec LUcolj = m_LU.getColumn(j);
		Vec* pLUcolj;
		CDenseVec dsvec(m_LU.m_Row);
		CSRVec csrvec;
		if( m_LU.m_bMode_DE ){
			m_LU.getColumn_Dense(j,&dsvec);
			pLUcolj = &dsvec;
		}else{
			csrvec = m_LU.getColumn_CSR(j);
			pLUcolj = &csrvec;
		}


		// Apply previous transformations.
		for (int i = 0; i < m; i++) {
			//Vec LUrowi = m_LU.getRow(i);
			Vec* pLUrowi;
			CDenseVec dsvec2(m_LU.m_Col);
			CSRVec csrvec2;
			if( m_LU.m_bMode_DE ){
				m_LU.getRow_Dense(i, &dsvec2);
				pLUrowi = &dsvec2;
			}else{
				csrvec2 = m_LU.getRow_CSR(i);
				pLUrowi = &csrvec2;
			}

			// Most of the time is spent in the following dot product.

			int kmax = min(i,j);
			//double s = LUrowi.dotProduct(LUcolj, 0, kmax-1);

			//LUcolj.plusEquals(i, -s);
			//LUrowi.plusEquals(j, -s);
			double s = dsvec2.dotProduct(&dsvec, 0, kmax-1);
			dsvec.plusEquals(i, -s);
			dsvec2.plusEquals(j, -s);
			m_LU.setRow_dense( i, &dsvec2 );
		}

		int p = j;
		if (autoPivot) {
			for (int i = j+1; i < m; i++) {
				double x1 = abs((double)(pLUcolj->get(i)));
				double x2 = abs((double)(pLUcolj->get(p)));
				if (abs((double)(pLUcolj->get(i))) > abs((double)(pLUcolj->get(p)))) {
					p = i;
				}
			}
		}

		if (p != j) {
			m_LU.swapRows(p, j);
			int k = m_piv[p]; m_piv[p] = m_piv[j]; m_piv[j] = k;
			m_pivsign = -m_pivsign;
		}

		// Compute multipliers.
		if (j < n && j < m && m_LU.get(j,j) != 0.0) {
			double LUjj = m_LU.get(j,j);
			for (int i = j+1; i < m; i++) {
				m_LU.timesEquals(i,j, 1.0/LUjj);
			}
		}
	}
	//CString TmpStr;
	//TmpStr.Format( _T("m_LU:%.4f,%.4f,%.4f,%.4f"), m_LU.get(0,0),m_LU.get(0,1), m_LU.get(1,0), m_LU.get(1,1) );
	//AfxMessageBox(TmpStr);
}

LUDecomposition::~LUDecomposition()
{
	if( m_piv!=NULL )
	{
		delete m_piv;
		m_piv = NULL;
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: det logdet
 * 功能描述: 对角线数据相乘
 * 输入参数: 
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
double LUDecomposition::det ()
{
	//assert( m_LU.m_Row == m_LU.m_Col );

	double d = (double) m_pivsign;
	for (int j = 0; j < m_LU.m_Col; j++)
		d *= m_LU.get(j,j);

	return d;
}

double LUDecomposition::logDet ()
{
	//assert(m_LU.m_Col == m_LU.m_Row);

	double sign = (double) m_pivsign;
	double logD = 0;
	for (int j = 0; j < m_LU.m_Col; j++) {
		double v = m_LU.get(j,j);
		sign *= signum(v);
		logD += log(abs(v));
	}

	//assert( sign>0 );

	return logD;
}

double LUDecomposition::signum( double v )
{
	if( v>-0.00000001&&v<0.00000001 )
	{
		return v;
	}
	else if( v>0 )
	{
		return 1.0;
	}
	else if ( v<0 )
	{
		return -1.0;
	}else{
		return 0;
	}


}

/*<FUNC+>*******************************************************
 * 函数名称: solve
 * 功能描述: 
 * 输入参数: 
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
CMatrix LUDecomposition::solve (CMatrix B)
{
	int m = m_LU.m_Row, n = m_LU.m_Col;
	//assert( B.m_Row==m_LU.m_Row );
	//assert( !isSingular() );

	// Copy right hand side with pivoting
        CMatrix X;
		B.copy( &X );
        X.permuteRows(m_piv, m_LU.m_Row);
        int nx = X.m_Col;

        // Solve L*Y = B(piv,:)
        for (int k = 0; k < n; k++) {
            Vec Xrowk = X.getRow(k);

            for (int i = k+1; i < n; i++) {
                Vec Xrowi = X.getRow(i);
                Xrowk.addTo(Xrowi, -m_LU.get(i,k));
            }
        }

        // Solve U*X = Y;
        for (int k = n-1; k >= 0; k--) {
            Vec Xrowk = X.getRow(k);
            Xrowk.timesEquals(1.0 / m_LU.get(k,k));

            for (int i = 0; i < k; i++) {
                Vec Xrowi = X.getRow(i);
                Xrowk.addTo(Xrowi, -m_LU.get(i,k));
            }
        }

        return X;
}

bool LUDecomposition::isSingular ()
{
	int m = m_LU.m_Row, n = m_LU.m_Col;
	for (int j = 0; j < n; j++) {
		if (m_LU.get(j,j)==0)
			return true;
	}

	return false;
}
