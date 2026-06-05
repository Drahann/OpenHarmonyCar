
#include "CMultiGauss.h"
#include "CholeskyDecomposition.h"
#include "LinAlg.h"

/*<FUNC+>*******************************************************
 * 函数名称: MultiGaussian
 * 功能描述: 构造函数和析构函数
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
MultiGaussian::MultiGaussian ( double** p, int row, int col )
{
	CMatrix* pTmpMatrix = new CMatrix( (double**)p, row, col );

	MultiGaussian( pTmpMatrix->copy() );
	delete pTmpMatrix;
	pTmpMatrix = NULL;
}

MultiGaussian::MultiGaussian ( CMatrix P )
{
	int len = P.getRowDimension();
	double* pTmpD = new double[len];
	MultiGaussian( &P, pTmpD, len );
	delete []pTmpD;
	pTmpD = NULL;
}

MultiGaussian::MultiGaussian ( double** P, int row, int col, double* u, int len )
{
	int Srclen =sizeof(double)*len;
	CMatrix pTmpMatrix( P, row, col );

	MultiGaussian( &pTmpMatrix, u, len );
}

MultiGaussian::MultiGaussian ( CMatrix* P, double* u, int len )
{
	int MemLen = sizeof(double)*len;
	CMatrix* pTmpMatrix;
	P->copy( &m_P );
	m_u = new double[len];
	//memcpy_s( m_u, MemLen, u, MemLen );

	memcpy( m_u,  u, MemLen );
	m_n = m_P.getColumnDimension();

	m_Pdet = m_P.det();
	if (m_Pdet==0) // if they pass in a zero covariance matrix, produce exact answers.
	{
		pTmpMatrix = new CMatrix(m_n, m_n);
		m_Pinv = pTmpMatrix->copy();
		m_L = pTmpMatrix->copy();
		m_Pscale = 0;
		delete pTmpMatrix;
		pTmpMatrix = NULL;
	}
	else
	{
		//m_Pinv = m_P.inverse();
		m_P.inverse( &m_Pinv );
		
		CholeskyDecomposition* cd = new CholeskyDecomposition( &m_P, m_P.m_bMode_DE, FALSE );
		if (false && !cd->isSPD()) // XXX returning wrong answer sometimes?
		{
			//System.out.println("MULTIGAUSSIAN: P is not semi-definite positive! det="+P.det());

			//P.print();
		}

		//m_L = cd->getL();
		cd->getL( &m_L );
		
		m_Pscale = 1.0/pow(2*PI, m_n/2.0)/sqrt(m_Pdet);

		delete cd;
		cd = NULL;
	}

	m_logPscale = log(m_Pscale);
}

MultiGaussian::~MultiGaussian ()
{
	delete []m_u;
	m_u = NULL;

	
}

/*<FUNC+>*******************************************************
 * 函数名称: getDimension
 * 功能描述: 获取m_P矩阵的容量(方阵的列数)
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/06        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
int MultiGaussian::getDimension ()
{
	return m_P.getColumnDimension();
}

/*<FUNC+>*******************************************************
 * 函数名称: sample
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
 *    2014/11/06        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void MultiGaussian::sample ( double* pData )
{
	for (int i = 0; i < m_n; i++)
		pData[i] = LinAlg::GetGuassian();

	m_L.times( pData, m_n, pData );
	LinAlg::add( pData, m_u, m_n, pData );
}

/*<FUNC+>*******************************************************
 * 函数名称: prob
 * 功能描述: 概率计算
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/06        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double MultiGaussian::prob (double* v, int len)
{
	//assert( len==m_n );
	double tmpd[2];
	LinAlg::subtract( v, m_u, len, tmpd );

	double* pTmpD = new double[len];

	double d1 = m_Pinv.get(0,0);
	double d2 = m_Pinv.get(0,1);
	double d3 = m_Pinv.get(1,0);
	double d4 = m_Pinv.get(1,1);
	m_Pinv.times( tmpd, len, pTmpD );
	d1 = m_Pinv.get(0,0);
	d2 = m_Pinv.get(0,1);
	d3 = m_Pinv.get(1,0);
	d4 = m_Pinv.get(1,1);
	double e = LinAlg::dotProduct(tmpd, len, pTmpD);
	double p = exp(-e/2.0);

	delete pTmpD;
	pTmpD = NULL;
	return m_Pscale*p;
}

/*<FUNC+>*******************************************************
 * 函数名称: chi2
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
 *    2014/11/06        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */

double MultiGaussian::chi2 (double* v, int len)
{
	// special case 3x3 for performance
	if (len == 3)
	{
		double a = v[0] - m_u[0];
		double b = v[1] - m_u[1];
		double c = v[2] - m_u[2];

		// assume symmetric Pinv (of course!)
		double d = m_Pinv.get(0,0), e = m_Pinv.get(0,1), f = m_Pinv.get(0,2);
		double g = m_Pinv.get(1,1), h = m_Pinv.get(1,2);
		double i = m_Pinv.get(2,2);

		return a*a*d + b*b*g + c*c*i + 2*(a*b*e + a*c*f + b*c*h);
	}

	LinAlg::subtract(v, m_u, len, v);
	double* pTmpD = new double[len];
	m_Pinv.times( v, len, pTmpD );
	double e = LinAlg::dotProduct(v, len, pTmpD);
	delete []pTmpD;
	pTmpD = NULL;

	return e;
}

/*<FUNC+>*******************************************************
 * 函数名称: logProb
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
 *    2014/11/06        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double MultiGaussian::logProb (double* v, int len)
{
	//	return Math.log(prob(v));

	return m_logPscale-chi2(v, len)/2;
}

/*<FUNC+>*******************************************************
 * 函数名称: getMahalanobisDistance
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
 *    2014/11/06        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double MultiGaussian::getMahalanobisDistance (double* x, int len)
{
	double* dx = new double[len];
	LinAlg::subtract(x, m_u, len, dx);
	double* Tdx = new double[len];
	m_Pinv.times( dx,len,Tdx );

	double ResD = sqrt(LinAlg::dotProduct(dx, len, Tdx));
	delete []Tdx;
	delete []dx;
	Tdx = NULL;
	dx = NULL;

	return ResD;
}
