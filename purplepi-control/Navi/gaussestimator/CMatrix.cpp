#include "define.h"
#include "CMatrix.h"
#include "Vec.h"
#include  <sys/time.h>
#include "LUDecomposition.h"

/*<FUNC+>*******************************************************
 * 函数名称: CMatrix
 * 功能描述: 构造和析构函数
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CMatrix::CMatrix()
	: m_Col(0)
	, m_Row(0)
	, m_pVec(NULL)
	, m_pVec_CSR(NULL)
	, m_pVec_Dense(NULL)
	, m_Option(0)
{

}

CMatrix::~CMatrix()
{
	if( m_bMode_DE ){
		for( int i=0; i<m_Row; i++ )
		{
			if( m_pVec_Dense[i].m_pValue!=NULL )
			{
				delete m_pVec_Dense[i].m_pValue;
				m_pVec_Dense[i].m_pValue = NULL;
			}
		}
		delete []m_pVec_Dense;
		m_pVec_Dense = NULL;
	}else{
		delete []m_pVec_CSR;
		m_pVec_CSR = NULL;
	}
}

CMatrix::CMatrix( int Row, int Col, int Option/* =0 */ )
{
	m_Option = Option;
	m_Row = Row;
	m_Col = Col;

	if( (m_Option&SPARSE)==0 ){
		m_bMode_DE = TRUE;
	}else{
		m_bMode_DE = FALSE;
	}

	if( m_bMode_DE ){
		m_pVec_Dense = new CDenseVec[Row];
		for( int i=0; i<Row; i++ )
		{
			//m_pVec_Dense[i] = makeVec_Dense(Col);
			m_pVec_Dense[i].resize(Col);
			m_pVec_Dense[i].clear();
		}
	}else{
		m_pVec_CSR = new CSRVec[Row];
		for( int i=0; i<Row; i++ )
		{
			m_pVec_CSR[i] = makeVec_CSR(Col);
		}
	}
}

CMatrix::CMatrix ( double** pV, int Row, int Col )
{
	m_Row = Row;
	m_Col = Col;

	//m_pVec = new Vec[Row];
	m_pVec_Dense = new CDenseVec[Row];
	for( int i=0; i<Row; i++ )
	{
		//m_pVec_Dense[i] = makeVec( pV[i], Col );
		m_pVec_Dense[i].resize( Col );
		for( int j=0; j<Col; j++ ){
			m_pVec_Dense[i].set(j,pV[i][j]);
		}

	}
	if(m_bMode_DE==FALSE){
		m_bMode_DE = TRUE;
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: makeVec
 * 功能描述: 创建Vec
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
Vec CMatrix::makeVec (int length)
{
	CDenseVec DenseVec(length);
	CSRVec SRVec(length);
	if ((m_Option&SPARSE)==0)
		return DenseVec;

	return SRVec;
}

CDenseVec CMatrix::makeVec ( double* p, int length )
{
	CDenseVec DenseVec( p, length );
	
	return DenseVec;
}

CSRVec CMatrix::makeVec_CSR ( int length )
{
	CSRVec SRVec(length);

	return SRVec;
}

CDenseVec CMatrix::makeVec_Dense ( int length )
{
	CDenseVec DenseVec(length);
	
	return DenseVec;
}
/*<FUNC+>*******************************************************
 * 函数名称: diag
 * 功能描述: 对角线元素已经确定的Matrix生成
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CMatrix CMatrix::diag (double* pV, int length)
{
	CMatrix X( length, length );

	for (int i = 0; i < length; i++)
		X.set(i,i, pV[i]);

	return X;
}

/*<FUNC+>*******************************************************
 * 函数名称: outerProduct
 * 功能描述: x[] = {x1,x2,...,xm} y[]={ y1,y2,....,yn}
 * CMatrix = { x1*y1, x1*y2,.....,x1*yn
 *			   x2*y1, x2*y2,.....,x2*yn
 *			   ........................
 *			   xm*y1, xm*y2,.....,xm*yn }
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CMatrix CMatrix::outerProduct (double* x, int len_x, double* y, int len_y)
{
	CMatrix X(len_x, len_y);
	for (int i = 0; i < len_x; i++) {
		for (int j = 0; j < len_y; j++) {
			X.set(i,j, x[i]*y[j]);
		}
	}

	return X;
}

void CMatrix::outerProduct (double* x, int len_x, double* y, int len_y, CMatrix* pMatrix)
{
	CMatrix X(len_x, len_y);
	for (int i = 0; i < len_x; i++) {
		for (int j = 0; j < len_y; j++) {
			X.set(i,j, x[i]*y[j]);
		}
	}

	X.copy(pMatrix);
}

/*<FUNC+>*******************************************************
 * 函数名称: copy
 * 功能描述: 将当前矩阵复制给别的矩阵
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CMatrix CMatrix::copy()
{
	CMatrix X( m_Row, m_Col, m_Option );

	for( int i = 0; i<m_Row; i++ )
	{
		if( m_bMode_DE ){
			CDenseVec* p = &m_pVec_Dense[i];
			X.m_pVec_Dense[i] = *p;
		}else{
			CSRVec* p = &m_pVec_CSR[i];
			X.m_pVec_CSR[i] = *p;
		}
	}

	return X;
}

void CMatrix::copy( CMatrix* pU )
{
	if( m_bMode_DE )
	{
		pU->m_bMode_DE = TRUE;
		pU->m_Row = m_Row;
		pU->m_Col = m_Col;
		pU->m_pVec_Dense = new CDenseVec[m_Row];
		for( int i=0; i<m_Row; i++ )
		{
			m_pVec_Dense[i].copy( &(pU->m_pVec_Dense[i]) );
		}
	}
	else
	{
		pU->m_bMode_DE = FALSE;
		pU->m_Row = m_Row;
		pU->m_Col = m_Col;
		pU->m_pVec_CSR = new CSRVec[m_Row];
		for( int i=0; i<m_Row; i++ )
		{
			m_pVec_CSR[i].copy( &(pU->m_pVec_CSR[i]) );
		}
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: copyArray
 * 功能描述: 将矩阵里的数据获取至二维数组中
 * 输入参数: map数组
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CMatrix::copyArray ( map<int,double>* pData )
{
	for (int i = 0; i < m_Row; i++)
	{
		if( m_bMode_DE ){
			Vec* p = &m_pVec_Dense[i];
			p->copyArray( pData[i] );
		}else{
			Vec* p = &m_pVec_CSR[i];
			p->copyArray( pData[i] );
		}
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: identity
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
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CMatrix CMatrix::identity (int m, int n)
{
	CMatrix M(m, n);
	int ntmp = 0;
	
	if( m>n ){
		ntmp = n;
	}else{
		ntmp = m;
	}
	for (int i = 0; i < ntmp; i++)
		M.set(i,i, 1);

	return M;
}

/*<FUNC+>*******************************************************
 * 函数名称: random
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
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CMatrix::GetRandArray ( double* pArray, int len )
{
	double j=0;
	double k=0;
	srand((unsigned int)time(NULL));
	for(int i=0;i<len;++i)
	{
		j=rand();
		k=rand();
		pArray[i] = j+k/32768;
	}
}

CMatrix CMatrix::random (int m, int n)
{
	CMatrix M(m,n);
	double* pRandArry = NULL;

	pRandArry = new double[m*n];
	CMatrix::GetRandArray( pRandArry, m*n );

	for (int i = 0; i < m; i++)
	{
		for (int j = 0; j < n; j++) 
		{
			M.set(i,j, pRandArry[i*n+j]);
		}
	}

	return M;
}

/*<FUNC+>*******************************************************
 * 函数名称: getRowDimension
 * 功能描述: 获取矩阵的行数
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
int CMatrix::getRowDimension ()
{
	return m_Row;
}

/*<FUNC+>*******************************************************
 * 函数名称: getColumnDimension
 * 功能描述: 获取矩阵的列数
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
int CMatrix::getColumnDimension ()
{
	return m_Col;
}

/*<FUNC+>*******************************************************
 * 函数名称: getOptions
 * 功能描述: 获取属性值
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
int CMatrix::getOptions ()
{
	return m_Option;
}

/*<FUNC+>*******************************************************
 * 函数名称: get
 * 功能描述: 获取矩阵中某个位置的数值
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/31        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double CMatrix::get ( int pos )
{
	int Row = pos/m_Col;
	int Col = pos - m_Col*Row;

	return get( Row, Col );
}

double CMatrix::get (int row, int col)
{
	//Vec r = m_pVec[row];
	Vec* r;
	if(m_bMode_DE){
		r = &m_pVec_Dense[row];
	}else{
		r = &m_pVec_CSR[row];
	}

	return r->get(col);
}

/*<FUNC+>*******************************************************
 * 函数名称: set
 * 功能描述: 在pos或col-row位置设置数据v
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CMatrix::set( int pos, double v )
{
	int row = pos/m_Col;
	int Col = pos - m_Col*row;

	set( row, Col, v );
}

void CMatrix::set (int row, int col, double v)
{
	//CDenseVec* r = (CDenseVec*)(void*)&m_pVec[row];
	////((CDenseVec*)r)->set(col, v);
	//r->set( col, v );
	Vec* p;
	if(m_bMode_DE){
		p = &m_pVec_Dense[row];
	}else{
		p = &m_pVec_CSR[row];
	}

	p->set( col, v );
}

void CMatrix::set ( int row, int col, double** pv, int v_row, int v_col )
{
	for( int dr = 0; dr < v_row; dr++ )
	{
		for( int dc = 0; dc < v_col; dc++ )
		{
			set( row+dr, col+dc, pv[dr][dc] );
		}
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: getColumn
 * 功能描述: 获得某一列的数据，存入Vec
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
Vec CMatrix::getColumn(int col)
{
	Vec res = makeVec(m_Row);

	for (int i = 0; i < m_Row; i++) {
		Vec r = m_pVec[i];
		res.set(i, r.get(col));
	}

	return res;
}

void CMatrix::getColumn_Dense ( int col, CDenseVec* pVec )
{
	Vec* p;

	for( int i=0; i<m_Row; i++ )
	{
		p = &m_pVec_Dense[i];
		pVec->set(i, p->get(col));
	}
}

CSRVec CMatrix::getColumn_CSR (int col)
{
	CSRVec ResVec = makeVec_CSR(m_Row);
	Vec* p;

	for( int i=0; i<m_Row; i++ )
	{
		p = &m_pVec_CSR[i];
		ResVec.set(i, p->get(col));
	}

	return ResVec;
}
/*<FUNC+>*******************************************************
 * 函数名称: getRows
 * 功能描述: 获得矩阵某一行的数据
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
Vec CMatrix::getRow(int row)
{
	return m_pVec[row];
}

void CMatrix::getRow_Dense(int row, CDenseVec* pVec)
{
	//return m_pVec_Dense[row];
	Vec* p = &m_pVec_Dense[row];

	for( int i=0; i<m_Col; i++ )
	{
		pVec->set(i, p->get(i));
	}
}

CSRVec CMatrix::getRow_CSR ( int row)
{
	return m_pVec_CSR[row];
}

/*<FUNC+>*******************************************************
 * 函数名称: setRow
 * 功能描述: 设置某一行的数据
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CMatrix::setRow (int row, Vec v)
{
	if( v.size()!=m_Col )
	{
		return;
	}

	for( int i=0; i<m_Col; i++ )
	{
		Vec* p;
		if( m_bMode_DE ){
			p = &m_pVec_Dense[row];
		}else{
			p = &m_pVec_CSR[row];
		}

		p->set( i, v.get(i) );
	}
}

void CMatrix::setRow_dense ( int row, CDenseVec* v )
{
	if( v->size() != m_Col )
		return;

	for( int i=0; i<m_Col; i++ ){
		m_pVec_Dense[row].set( i, v->get(i) );
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: isSparse
 * 功能描述: 查看矩阵的属性
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
bool CMatrix::isSparse ()
{
	return (m_Option&CMatrix::SPARSE)!=0;
}

/*<FUNC+>*******************************************************
 * 函数名称: getNz
 * 功能描述: 获取矩阵中的有效数据个数
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
int CMatrix::getNz ()
{
	int nz = 0;
	for (int i = 0; i < m_Row; i++) {
		Vec* p;
		if( m_bMode_DE ){
			p = &m_pVec_Dense[i];
		}else{
			p = &m_pVec_CSR[i];
		}
		nz += p->getNz();
	}
	return nz;
}

/*<FUNC+>*******************************************************
 * 函数名称: getNzFrac
 * 功能描述: 获取有效数据占总数据量的比例
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double CMatrix::getNzFrac ()
{
	double mn = ((double) m_Row)*((double) m_Col);
	return ((double) getNz()) / mn;
}

/*<FUNC+>*******************************************************
 * 函数名称: equals
 * 功能描述: 判定矩阵是否等同于另外一个矩阵
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
bool CMatrix::equals (CMatrix M)
{
	return equals(M, 0.000001);
}

bool CMatrix::equals(CMatrix M, double eps )
{
	double tmpEps;
	if (m_Row != M.m_Row || m_Col != M.m_Col)
		return FALSE;

	for (int i = 0; i < m_Row; i++)
		for (int j = 0; j < m_Col; j++)
			tmpEps = M.get(i,j)-get(i,j);
			if ( tmpEps > eps||tmpEps<-eps )
				return FALSE;

	return TRUE;
}

/*<FUNC+>*******************************************************
 * 函数名称: times
 * 功能描述: 矩阵倍乘一个数或矩阵
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CMatrix CMatrix::times (double v)
{
	CMatrix X = copy();
	for (int i = 0; i < m_Row; i++)
	{
		Vec* p;
		if( m_bMode_DE ){
			p = &(X.m_pVec_Dense[i]);
		}else{
			p = &(X.m_pVec_CSR[i]);
		}
		p->timesEquals(v);
	}
	return X;
}

void CMatrix::times ( double v, CMatrix* pMatrix )
{
	for( int i=0; i<m_Row; i++ )
	{
		if( m_bMode_DE )
		{
			pMatrix->m_pVec_Dense[i].timesEquals(v);
		}
		else
		{
			pMatrix->m_pVec_CSR[i].timesEquals(v);
		}
	}
}

CMatrix CMatrix::times (CMatrix B)
{
	if (m_Col != B.m_Row)
		return B;
		//throw new IllegalArgumentException("Matrix inner dimensions must agree");

	int xm = m_Row;
	int xn = B.m_Col;
	int Option = 0;

	if (isSparse() || B.isSparse())
		Option = SPARSE;
	else
		Option = DENSE;

	CMatrix X(xm, xn, Option);

	for (int col = 0; col < xn; col++) {
		if( B.m_bMode_DE ){
			CDenseVec pbcol;
			B.getColumn_Dense(col,&pbcol);
			for (int row = 0; row < xm; row++) {
				Vec arow = getRow(row);
				CDenseVec arow_dense;
				getRow_Dense( row, &arow_dense );
				X.set( row, col, arow_dense.dotProduct(&pbcol));
			}
		}else{
			CSRVec pbcol;
			pbcol =B.getColumn_CSR(col);
			for (int row = 0; row < xm; row++) {
				Vec arow = getRow(row);
				CSRVec arow_csr = getRow_CSR(row);
				X.set( row, col, arow_csr.dotProduct(pbcol));
			}
		}

	}

	return X;
}

void CMatrix::times (double* B, int len, double* pDest)
{
	//assert(m_Col==len);
	
	CDenseVec* col = new CDenseVec(B, len);

	for (int i = 0; i < m_Row; i++)
	{
		Vec* p;
		if( m_bMode_DE ){
			p = &m_pVec_Dense[i];
			pDest[i] = m_pVec_Dense[i].dotProduct(col);
		}else{
			p = &m_pVec_CSR[i];
			//m_pVec_CSR[i].dotProduct(*col);  //wang.lei 后续使用时添加或修改函数
		}
		//pDest[i] = p->dotProduct(*col);
	}

	delete col;
	col = NULL;
}

/*<FUNC+>*******************************************************
 * 函数名称: plus
 * 功能描述: 矩阵相加
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CMatrix CMatrix::plus (CMatrix B)
{
	CMatrix X = copy();
	Vec* pTmp;
	for (int i = 0; i < m_Row; i++)
	{
		//pTmp = &(B.m_pVec[i]);
		//pTmp->addTo(X.m_pVec[i], 1);
		if( m_bMode_DE ){
			if( B.m_bMode_DE ){
				pTmp = &(B.m_pVec_Dense[i]);
				pTmp->addTo( X.m_pVec_Dense[i], 1);
			}else{
				pTmp = &(B.m_pVec_CSR[i]);
				pTmp->addTo( X.m_pVec_CSR[i], 1 );
			}
		}
	}
	return X;
}

void CMatrix::plus (CMatrix* B, CMatrix* pMatrix)
{
	Vec* pTmp;

	for (int i = 0; i < m_Row; i++)
	{
		//pTmp = &(B.m_pVec[i]);
		//pTmp->addTo(X.m_pVec[i], 1);
		if( m_bMode_DE ){
			if( B->m_bMode_DE ){
				pTmp = &(B->m_pVec_Dense[i]);
				B->m_pVec_Dense[i].addTo( pMatrix->m_pVec_Dense[i], 1);
			}else{
				//pTmp = &(B.m_pVec_CSR[i]);
				//pTmp->addTo( X.m_pVec_CSR[i], 1 ); //wang.lei del ,add when csrvec is used
			}
		}
	}
	

}

/*<FUNC+>*******************************************************
 * 函数名称: plusEquals
 * 功能描述: 矩阵中数据加某个数值
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CMatrix::plusEquals (int i, int j, double v)
{
	Vec* p;
	if( m_bMode_DE ){
		p = &m_pVec_Dense[i];
	}else{
		p = &m_pVec_CSR[i];
	}

	p->plusEquals(j, v);
}

void CMatrix::plusEquals (double** pV, int row, int col)
{
	plusEquals( 0, 0, pV, row, col );
}

void CMatrix::plusEquals (int i0, int j0, double** pV, int row, int col)
{
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			Vec* p;
			if( m_bMode_DE ){
				p = &m_pVec_Dense[i0+i];
			}else{
				p = &m_pVec_CSR[i0+i];
			}
			p->plusEquals(j0+j, pV[i][j]);
		}
	}
}

void CMatrix::plusEquals (CMatrix M)
{
	plusEquals(0,0, M);
}

void CMatrix::plusEquals (int i0, int j0, CMatrix M)
{
	for (int i = 0; i < M.m_Row; i++) {
		for (int j = 0; j < M.m_Col; j++) {
			Vec* p;
			if( m_bMode_DE ){
				p = &m_pVec_Dense[i0+i];
			}else{
				p = &m_pVec_CSR[i0+i];
			}
			p->plusEquals(j0+j, M.get(i,j));
		}
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: copyPermuteRowsAndColumns
 * 功能描述: 获取
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/11/03        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CMatrix CMatrix::copyPermuteRowsAndColumns (int* perm, int len)
{
	Permutation* p = new Permutation(perm, len);

	CMatrix X = copy();
	//X.m_Row = m_Row;
	//X.m_Col = m_Col;
	//X.m_Option = m_Option;
	////X.m_pVec = new Vec[m_Row];
	//X.m_pVec = new CDenseVec[m_Row];

	for (int i = 0; i < len; i++)
	{
		//X.m_pVec[i] = getRow(perm[i]).copyPermuteColumns(*p);
		if( m_bMode_DE )
		{
			//X.m_pVec_Dense[i] = getRow_Dense(perm[i]).copyPermuteColumns1(*p);暂时注释，需要修改
		}
	}

	delete p;
	p = NULL;

	return X;
}

/*<FUNC+>*******************************************************
 * 函数名称: inversePermuteRows
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
void CMatrix::inversePermuteRows (int* pivot, int len)
{
	//Vec* newrows = new Vec[m_Row];
	//for (int i = 0; i < len; i++)
	//	newrows[pivot[i]] = m_pVec[i];

	//for( int i=0; i<m_Row; i++ )
	//{
	//	m_pVec[i] = newrows[i].copy();
	//}
	CDenseVec* newrows_dense;
	CSRVec* newrows_csr;

	if( m_bMode_DE )
	{
		newrows_dense = new CDenseVec[m_Row];
		for( int i = 0; i<len; i++ )
		{
			m_pVec_Dense[i].copy( &newrows_dense[pivot[i]] );
			//newrows_dense[pivot[i]] = m_pVec_Dense[i];
		}
		for( int i=0; i<m_Row; i++ )
		{
			//m_pVec_Dense[i] = newrows_dense[i];
			newrows_dense[i].copy( &m_pVec_Dense[i] );
		}
	}
	else
	{
		newrows_csr = new CSRVec[m_Row];
		for( int i = 0; i<len; i++ )
		{
			newrows_csr[pivot[i]] = m_pVec_CSR[i];
		}
		for( int i=0; i<m_Row; i++ )
		{
			m_pVec_CSR[i] = newrows_csr[i];
		}
	}
}

void CMatrix::permuteRows (int* perm, int len)
{
	//Vec* newrows = new Vec[m_Row];
	//for (int i = 0; i < len; i++)
	//{
	//	Vec* p = &m_pVec[perm[i]];		
	//	newrows[i] = p->copy();
	//}

	//for( int i=0; i<m_Row; i++ )
	//{
	//	Vec* p = &newrows[i];
	//	m_pVec[i] = p->copy();
	//}
	CDenseVec* newrows_dense;
	CSRVec* newrows_csr;

	if( m_bMode_DE )
	{
		newrows_dense = new CDenseVec[m_Row];
		for( int i = 0; i<len; i++ )
		{
			//newrows_dense[i] = m_pVec_Dense[perm[i]];
			m_pVec_Dense[perm[i]].copy( &newrows_dense[i] );
		}
		for( int i=0; i<m_Row; i++ )
		{
			//m_pVec_Dense[i] = newrows_dense[i];
			newrows_dense[i].copy( &m_pVec_Dense[i] );
		}
	}
	else
	{
		newrows_csr = new CSRVec[m_Row];
		for( int i = 0; i<len; i++ )
		{
			newrows_csr[i] = m_pVec_CSR[perm[i]];
		}
		for( int i=0; i<m_Row; i++ )
		{
			m_pVec_CSR[i] = newrows_csr[i];
		}
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: transpose
 * 功能描述: 转化成对称矩阵
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
CMatrix CMatrix::transpose ()
{
	CMatrix X(m_Col, m_Row, m_Option);
	for (int i = 0; i < m_Row; i++) {
		Vec* prow;
		//Vec row = m_pVec[i];
		//row.transposeAsColumn(&X, i);
		if( m_bMode_DE )
		{
			prow = &m_pVec_Dense[i];
		}
		else
		{
			prow = &m_pVec_CSR[i];
		}

		prow->transposeAsColumn(&X,i);
	}

	return X;
}

/*<FUNC+>*******************************************************
 * 函数名称: upperRightTranspose
 * 功能描述: 转化成对称的方阵
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
CMatrix CMatrix::upperRightTranspose ()
{
	//assert(m_Row==m_Col);

	CMatrix X(m_Col, m_Col, m_Option);
	for (int i = 0; i < m_Col; i++) {
		Vec row = getRow(i);
		row.transposeAsColumn(&X, i, i, m_Col-1);
	}

	return X;
}

void CMatrix::upperRightTranspose ( CMatrix* pMatrix )
{
	//assert(m_Row==m_Col);

	copy(pMatrix);
	for( int i=0; i<m_Row; i++ ){
		pMatrix->m_pVec_Dense[i].clear();
	}

	for (int i = 0; i < m_Col; i++) {
		CDenseVec row(m_Col);
		getRow_Dense(i,&row);
		row.transposeAsColumn(pMatrix, i, i, m_Col-1);
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: upperRight
 * 功能描述: 对称方阵
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
//CMatrix CMatrix::upperRight ()
void CMatrix::upperRight (CMatrix* pMatrix)
{
	//assert(m_Row==m_Col);
	//CMatrix X = copy();
	copy(pMatrix);

	for (int i = 0; i < m_Row; i++)
	{
		//Vec* p = &m_pVec[i];
		//X.m_pVec[i] = p->copyPart(i, m_Col-1);
		Vec* p;

		if( m_bMode_DE )
		{
			pMatrix->m_pVec_Dense[i].clear();
			//p = &m_pVec_Dense[i];
			//pMatrix->m_pVec_Dense[i] = m_pVec_Dense[i].copyPart1( i, m_Col-1 );
			m_pVec_Dense[i].copyPart1( i, m_Col-1, &pMatrix->m_pVec_Dense[i] );
		}
		else
		{
			p = &m_pVec_CSR[i];
			pMatrix->m_pVec_CSR[i] = m_pVec_CSR[i].copyPart1( i, m_Col-1 );
		}

	}
	//return X;
}

/*<FUNC+>*******************************************************
 * 函数名称: inverse
 * 功能描述: 求矩阵的逆矩阵
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
//CMatrix CMatrix::inverse ()
void CMatrix::inverse (CMatrix* pMatrix )
{
	if (m_Row==2 && m_Col==2) {
		double a = get(0,0), b = get(0,1);
		double c = get(1,0), d = get(1,1);

		double det = 1/(a*d-b*c);

		CMatrix X(2,2);
		X.set(0,0, det*d);
		X.set(0,1, -det*b);
		X.set(1,0, -det*c);
		X.set(1,1, det*a);

		//return X;
		X.copy(pMatrix);
	}

	if (m_Row==3 && m_Col==3) {
		double a = get(0,0), b = get(0,1), c = get(0,2);
		double d = get(1,0), e = get(1,1), f = get(1,2);
		double g = get(2,0), h = get(2,1), i = get(2,2);

		double det = 1/(a*e*i-a*f*h-d*b*i+d*c*h+g*b*f-g*c*e);

		CMatrix X(3,3);

		X.set(0,0, det*(e*i-f*h));
		X.set(0,1, det*(-b*i+c*h));
		X.set(0,2, det*(b*f-c*e));
		X.set(1,0, det*(-d*i+f*g));
		X.set(1,1, det*(a*i-c*g));
		X.set(1,2, det*(-a*f+c*d));
		X.set(2,0, det*(d*h-e*g));
		X.set(2,1, det*(-a*h+b*g));
		X.set(2,2, det*(a*e-b*d));

		//return X;
		X.copy(pMatrix);
	}

	//CString TmpStr;
	//TmpStr.Format( _T("inverser:%.4f,%.4f,%.4f,%.4f"), pMatrix->get(0,0),pMatrix->get(0,1), pMatrix->get(1,0), pMatrix->get(1,1) );
	//AfxMessageBox(TmpStr);
	//return solve(identity(m_Row,m_Row));如果需要使用，进行后续修改wang.lei
}

/*<FUNC+>*******************************************************
 * 函数名称: swapRows
 * 功能描述: 替换两行数据
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
void CMatrix::swapRows (int a, int b)
{
	//Vec t = m_pVec[a];
	//m_pVec[a] = m_pVec[b];
	//m_pVec[b] = t;

	if( m_bMode_DE )
	{
		CDenseVec t = m_pVec_Dense[a];
		m_pVec_Dense[a] = m_pVec_Dense[b];
		m_pVec_Dense[b] = t;
	}
	else
	{
		CSRVec t = m_pVec_CSR[a];
		m_pVec_CSR[a] = m_pVec_CSR[b];
		m_pVec_CSR[b] = t;
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: timesEquals
 * 功能描述: 矩阵自乘运算
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
void CMatrix::timesEquals(double v)
{
	for( int i = 0; i<m_Row; i++ )
	{
		//Vec* p = &m_pVec[i];
		//p->timesEquals(v);
		Vec* p;

		if( m_bMode_DE )
		{
			p = &m_pVec_Dense[i];
		}
		else
		{
			p = &m_pVec_CSR[i];
		}

		p->timesEquals(v);
	}
}

void CMatrix::timesEquals(int i, int j, double v)
{
	//Vec* p = &m_pVec[i];
	//p->timesEquals(v, j, j);
	Vec* p;

	if( m_bMode_DE )
	{
		p = &m_pVec_Dense[i];
	}
	else
	{
		p = &m_pVec_CSR[i];
	}

	p->timesEquals(v, j, j);
}

/*<FUNC+>*******************************************************
 * 函数名称: det.logdet,详见：LUDecomposition中的det和logdet
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
double CMatrix::det ()
{
	CMatrix* pTmpMatrix = new CMatrix();
	copy( pTmpMatrix );
	LUDecomposition* pLUD = new LUDecomposition(pTmpMatrix, TRUE);
	double d_det = pLUD->det();
	delete pTmpMatrix;
	pTmpMatrix = NULL;
	delete pLUD;
	pLUD = NULL;

	return d_det;
}

double CMatrix::logDet ()
{
	LUDecomposition LUD(copy());
	return LUD.logDet();
}

CMatrix CMatrix::solve (CMatrix B)
{
	LUDecomposition LUD(copy());

	return LUD.solve(B);
}
