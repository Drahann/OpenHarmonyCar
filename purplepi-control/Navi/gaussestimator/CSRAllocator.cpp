
#include "CSRAllocator.h"

/*<FUNC+>*******************************************************
 * 函数名称: getBucket
 * 功能描述: 根据capacity和up值获取对应的csrvec
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
vector<CSRVec> CSRAllocator::getBucket (int capacity, int up)
{
	int bucketidx;

	if (capacity < 1024)
		bucketidx = capacity/32;
	else if (capacity < 32768)
		bucketidx = (capacity/1024) + 32;
	else
		bucketidx = capacity/32768 + 64;

	bucketidx += up;

	vector<CSRVec> vec_R;
	while (bucketidx >= (int)m_cache.size())
		m_cache.push_back( vec_R );

	return m_cache.at( bucketidx );
}

/*<FUNC+>*******************************************************
 * 函数名称: get
 * 功能描述: 获取对应容量和长度的csrvec
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
CSRVec CSRAllocator::get (int length, int mincapacity)
{
	vector<CSRVec> bucket = getBucket(mincapacity, 1);
	if (bucket.size() > 0) {
		CSRVec res = bucket.at(bucket.size()-1);
		vector<CSRVec>::iterator pos = bucket.end();
		bucket.erase(pos-1);

		res.m_length = length;
		res.m_nz = 0;
		return res;
	}
	CSRVec r_csvec(length, mincapacity);

	return r_csvec;
}

/*<FUNC+>*******************************************************
 * 函数名称: put
 * 功能描述: 将数据写入公用的Arraylist
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
void CSRAllocator::put (CSRVec v)
{
	vector<CSRVec> bucket = getBucket(v.m_length, 0);
	if (bucket.size() < 10)
		bucket.push_back(v);
}
