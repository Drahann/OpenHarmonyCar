
#include "Vec.h"
#include <math.h>
#include "CMatrix.h"

CSRVec::CSRVec( int length, int capacity )
	: m_lastGetIdx(-1)
	, m_lastSetIdx(-1)
{
	if( capacity<CSRVMIN_SIZE )
	{
		capacity = CSRVMIN_SIZE;
	}
	m_length = length;
	m_capacity = capacity;
	m_pIndices = new int[capacity];
	m_pValue = new double[capacity];
}

CSRVec::~CSRVec()
{
	if( m_pIndices!=NULL ){
		delete []m_pIndices;
		m_pIndices = NULL;
	}
	if( m_pValue!=NULL ){
		delete []m_pValue;
		m_pValue = NULL;
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: copy
 * 功能描述: 重载基类的Vec拷贝函数
 * 输入参数: NULL
 * 
 * 输出参数: Vec X:拷贝后的Vec
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
Vec CSRVec::copy()
{
	int len = sizeof(int)*m_capacity;
	CSRVec X( m_length, m_capacity );
	memcpy( X.m_pIndices,m_pIndices, len );
	len = sizeof(double)*m_capacity;
	memcpy( X.m_pValue, m_pValue, len );
	X.m_nz = m_nz;

	return X;
}

void CSRVec::copy ( CSRVec* pVec )
{
	pVec->m_capacity = m_capacity;
	pVec->m_lastGetIdx = m_lastGetIdx;
	pVec->m_lastSetIdx = m_lastSetIdx;
	pVec->m_length = m_length;
	pVec->m_nz = m_nz;
	pVec->m_pValue = new double[m_capacity];
	pVec->m_pIndices = new int[m_capacity];

	int len = sizeof(double)*m_capacity;
	memcpy( pVec->m_pValue,  m_pValue, len );
	len = sizeof(int)*m_capacity;
	memcpy( pVec->m_pIndices, m_pIndices, len );
}

/*<FUNC+>*******************************************************
 * 函数名称: copyArray
 * 功能描述: 获取有效数据值（非零）的拷贝
 * 输入参数: CMap<int, int&, double, double&>& pData：需要拷贝数据的map
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::copyArray ( map<int, double>& pData )
{
	getDoubles( pData );
}

/*<FUNC+>*******************************************************
 * 函数名称: getDoubles
 * 功能描述: 获取有效数据值（非零）的拷贝
 * 输入参数: CMap<int, int&, double, double&>& pData：需要拷贝数据的map
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::getDoubles (map<int, double>& pData )
{
	for (int i = 0; i < m_nz; i++)
	{
		pData.at( m_pIndices[i]) = m_pValue[i] ;
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: filterZeros
 * 功能描述: 将数值为0的数据过滤掉
 * 输入参数: NULL
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::filterZeros()
{
	int outpos = 0;

	for (int inpos = 0; inpos < m_nz; inpos++) {
		if (m_pValue[inpos]!=0) {
			m_pIndices[outpos] = m_pIndices[inpos];
			m_pValue[outpos] = m_pValue[inpos];
			outpos++;
		}else{
			m_capacity--;
		}
	}

	m_nz = outpos;
}

/*<FUNC+>*******************************************************
 * 函数名称: filterZeros
 * 功能描述: 将数值低于或等于某个值的数据过滤掉
 * 输入参数: double eps：屏蔽的界限
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::filterZeros ( double eps )
{
	int outpos = 0;

	for (int inpos = 0; inpos < m_nz; inpos++) {
		if (m_pValue[inpos]>eps) {
			m_pIndices[outpos] = m_pIndices[inpos];
			m_pValue[outpos] = m_pValue[inpos];
			outpos++;
		}else{
			m_capacity--;
		}
	}

	m_nz = outpos;
}

/*<FUNC+>*******************************************************
 * 函数名称: insert
 * 功能描述: make the 'ith' element correspond to the (idx,v) tuple, moving
 *			anything after it as necessary. Position 'i' must be the
 *			correct position for idx. 'idx' must not already be in the vec.
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::insert (int i, int idx, double v)
{
	if (v==0)
		return;

	if (m_nz == m_capacity)
		grow();

	memcpy( &m_pIndices[i+1],  &m_pIndices[i], (m_nz-i)*sizeof(int) );
	memcpy( &m_pValue[i+1],  &m_pValue[i], (m_nz-i)*sizeof(double) );

	m_pIndices[i] = idx;
	m_pValue[i] = v;
	m_nz++;
}

/*<FUNC+>*******************************************************
 * 函数名称: grow 
 * 功能描述: 增加Vec的大小
 * 输入参数: int count：需要增加的个数
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::grow ( int mincapacity )
{
	int* pTmpIndices = new int[m_capacity];
	double* pTmpValue	 = new double[m_capacity];
	int len1 = sizeof(int)*m_capacity;
	memcpy( pTmpIndices, m_pIndices, len1 );
	int len2 = sizeof(double)*m_capacity;
	memcpy( pTmpValue,  m_pValue, len2 );

	int newcapacity = m_capacity *2;
	while (newcapacity < mincapacity)
		newcapacity *= 2;

	delete []m_pIndices;
	m_pIndices = NULL;
	delete []m_pValue;
	m_pValue = NULL;

	m_capacity = newcapacity;
	m_pIndices = new int[m_capacity];
	m_pValue = new double[m_capacity];

	memcpy( m_pIndices,  pTmpIndices, len1 );
	memcpy( m_pValue,  pTmpValue, len2 );

	delete []pTmpValue;
	pTmpValue = NULL;
	delete []pTmpIndices;
	pTmpIndices = NULL;
}

void CSRVec::grow()
{
	grow(0);
}

/*<FUNC+>*******************************************************
 * 函数名称: resize
 * 功能描述: 重新设置Vec的大小
 * 输入参数: int newlength:新的逻辑大小
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::resize (int newlength)
{
	if (m_length <= newlength) 
	{
		m_length = newlength;
		return;
	}

	while (m_nz > 0 && m_pIndices[m_nz-1] >= newlength) 
	{
		m_pIndices[m_nz-1] = 0;
		m_pValue[m_nz-1] = 0.0;
		m_nz--;
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: sort
 * 功能描述: Vec排序 this is fairly inefficient if there are many changes (but
 *			pretty reasonable if there's just one change, as in the
 *			common case of an insert.
 * 输入参数: NULL
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::sort()
{
	for (int i = 1; i < m_nz; i++) {
		if (m_pIndices[i] < m_pIndices[i-1]) {

			// do an insertion of i back somewhere.
			int itmp = m_pIndices[i];
			double vtmp = m_pValue[i];

			// find the insertion point
			int ipt = i;
			while (ipt >=1 && itmp<m_pIndices[ipt-1]) {
				m_pIndices[ipt]=m_pIndices[ipt-1];
				m_pValue[ipt]=m_pValue[ipt-1];
				ipt--;
			}

			m_pIndices[ipt] = itmp;
			m_pValue[ipt] = vtmp;
		}
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: ensureCapacity
 * 功能描述: 确认容器的容量大小
 * 输入参数: int mincapacity:最小容量
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::ensureCapacity (int mincapacity)
{
	if (mincapacity > m_capacity)
		grow(mincapacity);
}

/*<FUNC+>*******************************************************
 * 函数名称: copy
 * 功能描述: 拷贝某一范围的数据至新的Vec
 * 输入参数: i0：起始键值，i1：终了键值
 * 
 * 输出参数: CSRVec X：拷贝后的Vec
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
Vec CSRVec::copy (int i0, int i1)
{
	CSRVec X(i1 - i0 + 1);

	int low = 0;
	while (low < m_nz && m_pIndices[low] < i0)
		low++;

	// XXX could be done with arraycopy by finding high.
	for (int i = low; i < m_nz && m_pIndices[i] <= i1; i++) {

		if (X.m_nz == X.m_capacity)
			X.grow();

		X.m_pIndices[X.m_nz] = m_pIndices[i] - i0;
		X.m_pValue[X.m_nz] = m_pValue[i];
		X.m_nz++;
	}

	return X;
}

/*<FUNC+>*******************************************************
 * 函数名称: copyPart
 * 功能描述: 拷贝某一范围的数据至新的Vec,别的部分不做改变
 * 输入参数: i0：起始键值，i1：终了键值
 * 
 * 输出参数: CSRVec X：拷贝后的Vec
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CSRVec CSRVec::copyPart1 (int i0, int i1)
{

	CSRVec X(m_length);

	int low = 0;
	while (low < m_nz && m_pIndices[low] < i0)
		low++;

	// XXX could be done with arraycopy by finding high.
	for (int i = low; i < m_nz && m_pIndices[i] <= i1; i++) {

		if (X.m_nz == X.m_capacity)
			X.grow();

		X.m_pIndices[X.m_nz] = m_pIndices[i];
		X.m_pValue[X.m_nz] = m_pValue[i];
		X.m_nz++;
	}

	return X;
}


void CSRVec::copyPart1 (int i0, int i1, CSRVec* pVec)
{

}

/*<FUNC+>*******************************************************
 * 函数名称: size
 * 功能描述: 获取容器的逻辑大小
 * 输入参数: NULL
 * 
 * 输出参数: int Size：容器的逻辑大小
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
int CSRVec::size ()
{
	return m_length;
}

/*<FUNC+>*******************************************************
 * 函数名称: getNz
 * 功能描述: 获取有效数据的长度
 * 输入参数: NULL
 * 
 * 输出参数: 有效数据的长度
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
int CSRVec::getNz ()
{
	return m_nz;
}

/*<FUNC+>*******************************************************
 * 函数名称: get
 * 功能描述: 根据键值获取数据,not thread safe. Maintain a cursor to
 *			help consecutive accesses 
 * 输入参数: int idx：键值
 * 
 * 输出参数: double data:获得的数据
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double CSRVec::get ( int idx )
{
	if (m_nz==0)
		return 0;

	if (m_lastGetIdx >= m_nz || m_lastGetIdx<0)
		m_lastGetIdx = m_nz/2;

	if (m_pIndices[m_lastGetIdx]<idx) {
		// search up
		while (m_lastGetIdx+1<m_nz && m_pIndices[m_lastGetIdx+1]<=idx)
			m_lastGetIdx++;

		if (m_pIndices[m_lastGetIdx]==idx)
			return m_pValue[m_lastGetIdx];

		return 0;
	} else {
		// search down
		while (m_lastGetIdx-1>=0 && m_pIndices[m_lastGetIdx-1]>=idx)
			m_lastGetIdx--;

		if (m_pIndices[m_lastGetIdx]==idx)
			return m_pValue[m_lastGetIdx];

		return 0;
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: getRef
 * 功能描述: 以正常校对方式根据键值获取数值
 * 输入参数: int idx：键值
 * 
 * 输出参数: double data：键值对应的数据
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double CSRVec::getRef (int idx)
{
	for (int i = 0; i < m_nz && m_pIndices[i] <= idx; i++)
	{
		if (m_pIndices[i]==idx)
		{
			return m_pValue[i];
		}
	}

	return 0;
}

/*<FUNC+>*******************************************************
 * 函数名称: set
 * 功能描述: 设置键值和相应的数据
 *		reference version of "get", without the cursor caching.
 * 输入参数: int idx：键值，double v:数据
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::set (int idx, double v)
{
	if (m_lastSetIdx < 0 || m_lastSetIdx >= m_nz)
	{
		m_lastSetIdx = m_nz/2;
	}

	if (m_nz==0) 
	{
		if (v==0)
		{
			return;
		}
		m_pIndices[0]=idx;
		m_pValue[0]=v;
		m_nz=1;
		return;
	}

	if (m_pIndices[m_lastSetIdx]==idx) {
		m_pValue[m_lastSetIdx]=v;
		return;
	}

	// search.
	if (m_pIndices[m_lastSetIdx]<idx) {
		// search up
		while (m_lastSetIdx+1<m_nz && m_pIndices[m_lastSetIdx+1]<=idx)
			m_lastSetIdx++;

		if (m_pIndices[m_lastSetIdx]==idx) {
			m_pValue[m_lastSetIdx]=v;
			return;
		}
		insert(m_lastSetIdx+1, idx, v);

	} else {
		// search down
		while (m_lastSetIdx-1>=0 && m_pIndices[m_lastSetIdx-1]>=idx)
			m_lastSetIdx--;

		if (m_pIndices[m_lastSetIdx]==idx) {
			m_pValue[m_lastSetIdx]=v;
			return;
		}

		insert(m_lastSetIdx, idx, v);
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: setRef
 * 功能描述: 设置键值和值，
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/28        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::setRef ( int idx, double v )
{
	for (int i = 0; i < m_nz && m_pIndices[i] <= idx; i++) {
		if (m_pIndices[i]==idx) {
			m_pValue[i] = v;
			return;
		}
	}

	if (v==0)
		return;

	if (m_nz == m_capacity)
		grow();

	m_pIndices[m_nz] = idx;
	m_pValue[m_nz] = v;
	m_nz++;
	sort();
}

/*<FUNC+>*******************************************************
 * 函数名称: dotProduct
 * 功能描述: Vec内数据点乘和功能添加
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double CSRVec::dotProduct (CSRVec r)
{
	double acc = 0;
	int aidx = 0;
	int bidx = 0;

	while (aidx < m_nz && bidx < r.m_nz) {
		int ai = m_pIndices[aidx], bi = r.m_pIndices[bidx];

		if (ai == bi) {
			acc += m_pValue[aidx]*r.m_pValue[bidx];
			aidx++;
			bidx++;
			continue;
		}

		if (ai < bi)
			aidx++;
		else
			bidx++;
	}

	return acc;
}

double CSRVec::dotProduct ( CSRVec r, int i0, int i1 )
{
	double acc = 0;
	int aidx = 0, bidx = 0;

	while (aidx < m_nz && m_pIndices[aidx] < i0)
		aidx++;
	while (bidx < r.m_nz && r.m_pIndices[bidx] < i0)
		bidx++;

	while (aidx < m_nz && bidx < r.m_nz) {
		int ai = m_pIndices[aidx], bi = r.m_pIndices[bidx];

		if (ai > i1 || bi > i1)
			break;

		if (ai == bi) {
			acc += m_pValue[aidx]*r.m_pValue[bidx];
			aidx++;
			bidx++;
			continue;
		}

		if (ai < bi)
			aidx++;
		else
			bidx++;
	}

	return acc;
}

/*<FUNC+>*******************************************************
 * 函数名称: timesEquals
 * 功能描述: Vec里数据倍乘一个数据
 * 输入参数: double scale：所乘的倍数
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::timesEquals (double scale)
{
	for (int i = 0; i < m_nz; i++)
		m_pValue[i] *= scale;
}

void CSRVec::timesEquals (double scale, int i0, int i1)
{
	int low = 0;
	while (low < m_nz && m_pIndices[low] < i0)
		low++;

	for (int i = low; i < m_nz && m_pIndices[i] <= i1; i++)
		m_pValue[i] *= scale;
}

/*<FUNC+>*******************************************************
 * 函数名称: transposeAsColumn
 * 功能描述: 将Vec输出为矩阵的一列
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::transposeAsColumn (/*CMatrix*/LPVOID A, int col)
{
	for (int i = 0; i < m_nz; i++)
		((CMatrix*)A)->set(m_pIndices[i], col, m_pValue[i]);
}

void CSRVec::transposeAsColumn(/*CMatrix*/LPVOID A, int col, int i0, int i1)
{
	int low = 0;
	while (low < m_nz && m_pIndices[low] < i0)
		low++;

	for (int i = low; i < m_nz && m_pIndices[i] <= i1; i++)
		((CMatrix*)A)->set(m_pIndices[i], col, m_pValue[i]);
}

/*<FUNC+>*******************************************************
 * 函数名称: add
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
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::add (CSRVec csra, double ascale, CSRVec csrb, int i0, int i1, CSRVec& res)
{
	//assert(csra.m_length == csrb.m_length);

	int apos = 0, bpos = 0;

	while (apos < csra.m_nz || bpos < csrb.m_nz) {
		int thisidx = 65535;
		if (apos < csra.m_nz)
			thisidx = csra.m_pIndices[apos];
		if (bpos < csrb.m_nz)
			thisidx = thisidx<csrb.m_pIndices[bpos]?thisidx:csrb.m_pIndices[bpos];
			//thisidx = Math.min(thisidx, csrb.indices[bpos]);

		double vala = 0;
		double valb = 0;

		if (apos < csra.m_nz && csra.m_pIndices[apos] == thisidx) {
			vala = csra.m_pValue[apos];
			apos++;
		}
		if (bpos < csrb.m_nz && csrb.m_pIndices[bpos] == thisidx) {
			valb = csrb.m_pValue[bpos];
			bpos++;
		}

		if (thisidx < i0 || thisidx > i1)
			continue;

		double thisvalue = ascale*vala + valb;
		res.m_pIndices[res.m_nz] = thisidx;
		res.m_pValue[res.m_nz] = thisvalue;
		res.m_nz++;
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: addTo
 * 功能描述: 当前数据乘以scale倍数后加到vec r中
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::addTo (CSRVec& r, double scale)
{
	int ridx = 0;

	for (int i = 0; i < m_nz; i++) {
		while (ridx < r.m_nz && r.m_pIndices[ridx] < m_pIndices[i])
			ridx++;

		if (ridx < r.m_nz && r.m_pIndices[ridx]==m_pIndices[i]) {
			r.m_pValue[ridx] += m_pValue[i]*scale;
		} else {
			r.insert(ridx, m_pIndices[i], m_pValue[i] * scale);
		}
	}
}

void CSRVec::addTo (CSRVec& r, double scale, int i0, int i1)
{
	int low = 0;
	while (low < m_nz && m_pIndices[low] < i0)
		low++;

	int ridx = 0;

	for (int i = low; i < m_nz && m_pIndices[i] <= i1; i++) {
		while (ridx < r.m_nz && r.m_pIndices[ridx] < m_pIndices[i])
			ridx++;

		if (ridx < r.m_nz && r.m_pIndices[ridx]==m_pIndices[i]) {
			r.m_pValue[ridx] += m_pValue[i]*scale;
		} else {
			r.insert(ridx, m_pIndices[i], m_pValue[i]*scale);
		}
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: plusEquals
 * 功能描述: 在键值idx处增加数值v，如果无idx键值，则创建该键值，并赋值为v
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CSRVec::plusEquals (int idx, double v)
{
	for (int i = 0; i < m_nz; i++) {
		if (m_pIndices[i] == idx) {
			m_pValue[i] += v;
			return;
		}
	}

	// it's a new element
	set(idx, v);
}

/*<FUNC+>*******************************************************
 * 函数名称: normF
 * 功能描述: 求队列中数据的平方和
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double CSRVec::normF ()
{
	double acc = 0;

	for (int i = 0; i < m_nz; i++)
		acc += m_pValue[i]*m_pValue[i];

	return acc;
}

/*<FUNC+>*******************************************************
 * 函数名称: copyPermuteColumns
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
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CSRVec CSRVec::copyPermuteColumns1 (Permutation& p)
{
	CSRVec X(m_length, m_capacity);

	for (int i = 0; i < m_nz; i++) {
		X.m_pIndices[i] = p.m_invperm[m_pIndices[i]];
		X.m_pValue[i]  = m_pValue[i];
	}
	X.m_nz = m_nz;
	X.sort();

	return X;
}

/*<FUNC+>*******************************************************
 * 函数名称: DeneseVec 
 * 功能描述: 构造函数和析构函数
 * 输入参数: int length:vec的大小
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CDenseVec::CDenseVec( int length )
{
	if( length < 0 )
	{
		m_pValue = NULL;
		m_capacity = 0;
		return;
	}
	m_capacity = length;
	m_pValue = new double[length];
	memset( m_pValue, 0, sizeof(double)*length );
}

CDenseVec::CDenseVec( double* pValue, int length )
{
	m_pValue = new double[length];
	m_capacity = length;

	memcpy( m_pValue,  pValue, sizeof(double)*length );
}

CDenseVec::~CDenseVec()
{
	if( m_pValue!=NULL )
	{
		delete []m_pValue;
		m_pValue = 0;
	}
	m_capacity = 0;
}

/*<FUNC+>*******************************************************
 * 函数名称: copy
 * 功能描述: 复制当前Vec并输出
 * 输入参数: NULL
 * 
 * 输出参数: Vec X：复制完成的结构体
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
Vec CDenseVec::copy ()
{
	CDenseVec X(m_capacity);
	for (int i = 0; i < m_capacity; i++)
		X.m_pValue[i] = m_pValue[i];

	return X;
}

Vec CDenseVec::copy (int i0, int i1)
{
	CDenseVec X(i1 - i0 + 1);
	for (int i = 0; i < X.m_capacity; i++)
		X.m_pValue[i] = m_pValue[i0+i];

	return X;
}

/* 拷贝大小相同，数据仅为下标i0到i1之间的数据 */
CDenseVec CDenseVec::copyPart1 (int i0, int i1)
{
	CDenseVec X(m_capacity);
	for (int i = i0; i <= i1; i++)
		X.m_pValue[i] = m_pValue[i];

	return X;
}

void CDenseVec::copyPart1 (int i0, int i1, CDenseVec* pVec)
{
	int len = sizeof(double)*(i1+1-i0);
	memcpy( &pVec->m_pValue[i0],  &m_pValue[i0], len );
}

void CDenseVec::copy ( CDenseVec* pVec )
{

	//pVec->m_pValue = new double[m_capacity];
	pVec->resize( m_capacity );
	memcpy( pVec->m_pValue,  m_pValue, sizeof(double)*m_capacity );
	pVec->m_capacity = m_capacity;
}

/*<FUNC+>*******************************************************
 * 函数名称: copyArray
 * 功能描述: 将i和value值一起写入map中
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */

void CDenseVec::copyArray( map<int, double>& pData )
{
	for( int i=0; i<m_capacity; i++ )
		pData.at(i) = m_pValue[i] ;
}

/*<FUNC+>*******************************************************
 * 函数名称: resize
 * 功能描述: 重新设置容器大小
 * 输入参数: int newlength：新容器的大小
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CDenseVec::resize (int newlength)
{
	double* pTmpData = new double[m_capacity];
	int len = sizeof(double)*m_capacity;
	memcpy( pTmpData, m_pValue, len );

	delete []m_pValue;
	m_pValue = NULL;
	m_pValue = new double[newlength];

	memset( m_pValue, 0, sizeof(double)*newlength );

	if( newlength<m_capacity )
	{
		len = sizeof(double)*newlength;
	}

	memcpy( m_pValue,  pTmpData, len );

	delete []pTmpData;
	pTmpData = NULL;

	m_capacity = newlength;
}

/*<FUNC+>*******************************************************
 * 函数名称: getDoubles
 * 功能描述: 获取Vec中的double数组
 * 输入参数: int length:pValue的长度输入
 *			pValue：存储double数据的数组首指针
 * 输出参数: BOOL Res: TRUE--成功，FALSE--失败
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
bool CDenseVec::getDoubles ( int length, double* pValue )
{
	if( length<m_capacity ){
		return FALSE;
	}else{
		memcpy( pValue,  m_pValue, sizeof(double)*m_capacity );
		return TRUE;
	}
}

/*<FUNC+>*******************************************************
 * 函数名称: size、getnz 
 * 功能描述: 获取Vec的长度
 * 输入参数: NULL
 * 
 * 输出参数: int len：Vec长度
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
int CDenseVec::size ()
{
	return m_capacity;
}

int CDenseVec::getNz ()
{
	return m_capacity;
}

/*<FUNC+>*******************************************************
 * 函数名称: get
 * 功能描述: 获取Vec中的idx下标的数值
 * 输入参数: int idx：Vec中的下标
 * 
 * 输出参数: double data：取出的数据
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/29        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double CDenseVec::get ( int idx )
{
	return m_pValue[idx];
}

/*<FUNC+>*******************************************************
 * 函数名称: set
 * 功能描述: 设置idx值对应的value
 * 输入参数: int idx:下标 double v:数值
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CDenseVec::set (int idx, double v)
{
	m_pValue[idx] = v;
}

/*<FUNC+>*******************************************************
 * 函数名称: dotProduct
 * 功能描述: 点乘Vec r的处理,以其中数据较少的为基准
 * 输入参数: Vec r:需要点乘的数据
 * 
 * 输出参数: double acc：点乘后的和
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double CDenseVec::dotProduct (CDenseVec* r)
{
	int length = r->getNz();
	if (r->getNz() > getNz())
		length = getNz();

	double acc = 0;
	for (int i = 0; i < length; i++) {
		acc += m_pValue[i]*r->get(i);
	}

	return acc;
}

double CDenseVec::dotProduct (CDenseVec* r, int i0, int i1)
{
	int length = ((CDenseVec*)r)->getNz();
	if (r->getNz() > getNz())
		length = getNz();

	double acc = 0;
	if( length>i1 )
		length = i1;

	if(length<i0 )
		return 0;

	for (int i = i0; i <= length; i++) {
		acc += m_pValue[i]*r->get(i);
	}

	return acc;
}

/*<FUNC+>*******************************************************
 * 函数名称: timesEquals
 * 功能描述: Vec中的数据倍乘scale
 * 输入参数: double scale：倍数
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CDenseVec::timesEquals (double scale)
{
	for (int i = 0; i < m_capacity; i++)
		m_pValue[i] *= scale;
}

void CDenseVec::timesEquals (double scale, int i0, int i1)
{
	for (int i = i0; i <= i1; i++)
		m_pValue[i] *= scale;
}

/*<FUNC+>*******************************************************
 * 函数名称: transposeAsColumn
 * 功能描述: 将Vec转变为矩阵的一列
 * 输入参数: Matrix A:目标矩阵，int col:列号
 * 
 * 输出参数: NULL
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CDenseVec::transposeAsColumn (/*CMatrix*/LPVOID A, int col)
{
	for (int i = 0; i < m_capacity; i++)
		((CMatrix*)A)->set(i,col, m_pValue[i]);
}

void CDenseVec::transposeAsColumn (/*CMatrix*/LPVOID A, int col, int i0, int i1)
{
	for (int i = i0; i <= i1; i++)
		((CMatrix*)A)->set(i,col, m_pValue[i]);
}

/*<FUNC+>*******************************************************
 * 函数名称: addTo
 * 功能描述: 将Vec数据倍乘后加入Vec r中
 * 输入参数: 
 * 
 * 输出参数: 
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void CDenseVec::addTo (CDenseVec& r, double scale)
{
	for (int i = 0; i < m_capacity; i++)
		r.set(i, r.get(i) + m_pValue[i]*scale);
}

void CDenseVec::addTo (Vec& r, double scale, int i0, int i1)
{
	for (int i = i0; i <= i1; i++)
		r.set(i, r.get(i) + m_pValue[i]*scale);
}

/*<FUNC+>*******************************************************
 * 函数名称: normF
 * 功能描述: 获取平方和
 * 输入参数: NULL
 * 
 * 输出参数: double：平方和
 *	
 * 返 回 值: 
 * 操作流程: 
 * 其它说明: 无
 * 修改记录: 
 * ------------------------------------------------------------- 
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double CDenseVec::normF ()
{
	double acc = 0;
	for (int i = 0; i < m_capacity; i++)
		acc += m_pValue[i]*m_pValue[i];

	return acc;
}

/*<FUNC+>*******************************************************
 * 函数名称: copyPermuteColumns
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
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
CDenseVec CDenseVec::copyPermuteColumns1 (Permutation& p)
{
	CDenseVec X(m_capacity);

	for (int i = 0; i < m_capacity; i++) {
		X.m_pValue[i] = p.m_invperm[i];
	}

	return X;
}
