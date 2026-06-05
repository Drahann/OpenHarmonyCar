
#pragma once

//#include "Trace.h"
//#include "TraceDlg.h"
#include <stdio.h>
#include <stdlib.h>
#include "math.h"
#include "../type.h"


class Matrixtt
{
public:
	Matrixtt();
	Matrixtt(int m,int n);//m*n 的0矩阵
	Matrixtt(int n);		  //n*n 的单位阵
	Matrixtt(const Matrixtt &);//拷贝构造函数，深拷贝
	Matrixtt(double *items,int m,int n);//数组构建矩阵
	~Matrixtt();

	int getRowNum() const;//返回矩阵的行数
	int getColNum() const;//返回矩阵的列数

	Matrixtt Trans() const; //矩阵转置

	double get(int i,int j) const;//返回矩阵第i行j列的元素
	void set(int i,int j,double val);//设置矩阵第i行j列的元素

	Matrixtt operator +(const Matrixtt &m);//矩阵相加
	Matrixtt operator -(const Matrixtt &m);//矩阵相减
	Matrixtt operator *(const Matrixtt &m);//矩阵相乘
	Matrixtt operator *(const double f);//矩阵乘以常数
	Matrixtt operator =(const Matrixtt &m);//矩阵复制

	Matrixtt addsubmatrix(int m,int n,const Matrixtt &M);//在大矩阵中特定位置加上子矩阵
	Matrixtt Inverse(int &a);//矩阵求逆

private:
	double *item;//矩阵元素
	int rowNum; //行数
	int colNum; //列数

private:

	//矩阵初等行变换
	//如果j= -1，对i行扩大multiply倍
	//如果j在取值范围内，将第i行扩大multiply倍加到j行
	void RowSwap(int i,int j,double multiply);
	void RowSwap(int i,int j);//交换两行
	//void FlowOver();
	
};




// µÃµœœÇ¶ÈµÄÕýÏÒÖµ£»ÊäÈë·¶Î§(-180~180) œÇ¶È
double getdegreesin(int i);

// µÃµœœÇ¶ÈµÄÓàÏÒÖµ£»ÊäÈë·¶Î§(-180~180) œÇ¶È
double getdegreecos(int i);
//µÃµœœÇ¶ÈµÄÕýÏÒÖµ£»ÊäÈë·¶Î§(-180~180)   »¡¶È
double getradianssin(double x);
//µÃµœœÇ¶ÈµÄÓàÏÒÖµ£»ÊäÈë·¶Î§(-180~180)   »¡¶È
double getradianscos(double x);

//ŽÓ»¡¶È±ä»¯µœœÇ¶È
double radians_to_degrees(double theta);
//ŽÓœÇ¶È±ä»¯µœ»¡¶È
double degrees_to_radians(double theta);

/**************************************************
º¯ÊýÃû£º        clamp
ÃèÊö£º          °ÑYµÄÊýÖµÇ¯ÖÆÔÚ[X,Z]Ö®Œä£»         
**************************************************/
double clamp(double X, double Y, double Z);

      
/************************************************************************
º¯Êý¹ŠÄÜ£º°ÑŸÖ²¿×ø±êÏµÏÂµÄµã×ª»»ÎªÈ«ŸÖ×ø±êÏµÏÂµÄµã£»
ÊäÈë²ÎÊý£ºInputPoint.xºÍInputPoint.yÎªžÃµãÔÚŸÖ²¿µØÍŒÏÂµÄ×ø±ê£¬ÓÐŸÖ²¿µØÍŒ·¶Î§ÏÞÖÆ£¬¿ÉÒÔÎªžºÊý£»
Êä³ö²ÎÊý£ºžÃµãÔÚÈ«ŸÖ×ø±êÏµÏÂµÄµãµÄ×ø±ê£¬ÓÐÈ«ŸÖµØÍŒ·¶Î§ÏÞÖÆ£»
±ž×¢£º//ÕâÀïÐèÒªÒ»žöº¯Êý£¬ÅÐ¶ÏžÃµãÊÇ·ñÔÚŸÖ²¿µØÍŒµÄ·¶Î§Ö®ÄÚ£¬×¢ÒâÊäÈëµÄInputPoint.xºÍInputPoint.y¿ÉÒÔÎªžºÊý
		×¢Òâ£¬²»ÒªÉèŒÆµœLMAP£¬ÄãÔÚÕâÀï»áºÜÈÝÒ×Åª»ìÏýµÄ£¬ÏµÍ³ÕûÀíLMapÓëGMap¡£
		×¢Òâ£º¶ÔÓÚÒ»žö×ª»»ŸÍÄÜ¹»ÊµÏÖµÄ¹ŠÄÜ£¬ÐèÒªÊäÈëºÍÊä³ö¶ŒœøÐÐãÐÖµÅÐ¶Ï;
		1.	(lx,ly)->(gx,gy)
************************************************************************/




//œÇ¶È±ê×Œ»¯º¯Êý£¬ÊäÈë²ÎÊý£º·œÏòœÇ£¬ÈÎÒâœÇ¶È£»Êä³ö²ÎÊý£º·œÏòœÇ£¬·¶Î§ÔÚ[-PI ,PI)
double Simu_normalize_theta(double theta);

//œÇ¶È±ê×Œ»¯º¯Êý£¬ÊäÈë²ÎÊý£º·œÏòœÇ£¬ÈÎÒâœÇ¶È£»Êä³ö²ÎÊý£º·œÏòœÇ£¬·¶Î§ÔÚ[-180,180)
double Simu_normalize_degree(double theta);
double Simu_norm(double X, double Y ) ;

//µÃµœŽÓÁœÌõÏßÖ®ŒäµÄŒÐœÇ£¬ÊäÈë²ÎÊý£ºÁœÌõÏßµÄ·œÏòœÇ£»Êä³ö²ÎÊý£ºÁœÌõÏßŒÐœÇ£¬·¶Î§ÔÚ(-180,180]
double GetLLAngle(double L1, double L2);
//µÃµœÁœžöÎ»ÖÃÖ®ŒäµÄŸàÀë;

float DisPtToLine(long x0, long y0,float k, float b);

