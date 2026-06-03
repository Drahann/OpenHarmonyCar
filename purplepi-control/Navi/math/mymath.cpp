
#include <stdio.h>
#include <stdlib.h>
#include "math.h"

#include "mymath.h"

Matrixtt::Matrixtt(void)
{
	rowNum = 1;
	colNum = 1;
	item = new double[1];
}


Matrixtt::Matrixtt(int m,int n)//m*n 的0矩阵
{
	printf("m = %d,n = %d\n",m,n);
	if(m <0 || n<0)
	{
		printf("Matrix error\n");
		return;
	}

	rowNum = m;
	colNum = n;
	item = new double[m*n];
	for(int i = 0;i < m*n; i++)
	{
		item[i] = 0;
	}
}

Matrixtt::Matrixtt(int n)		  //n*n 的单位阵
{
	if(n < 0)
	{
		printf("matrix error\n");
		return;
	}
	rowNum = n;
	colNum = n;
	item = new double[n*n];
	printf("n=%d\n",n);
	for(int i=1;i<=n;i++)
	{
		for(int j=1;j<=n;j++)
		{
			if (i == j)
			{
				printf("i=%d,j=%d\n",i,j);
				set(i,j,1.0);
			}
			else
			{
				set(i,j,0);
			}
		}
	}
}

Matrixtt::Matrixtt(const Matrixtt &M)//拷贝构造函数，深拷贝
{
	colNum = M.getColNum();
	rowNum = M.getRowNum();
	item = new double[rowNum*colNum];
	for(int i=1;i<=rowNum;i++)
	{
		for(int j = 1;j<=colNum;j++)
		{
			set(i,j,M.get(i,j));
		}
	}
}

Matrixtt::Matrixtt(double *items,int m,int n)//数组构建矩阵
{
	rowNum = m;
	colNum = n;
	item = new double[m*n];
	for(int i = 0;i<m*n;i++)
	{
		item[i] = items[i];
	}
}


Matrixtt::~Matrixtt(void)
{
	if(item != NULL)
	{
		delete [] item;
		item = NULL;
	}
}


int Matrixtt::getRowNum() const//返回矩阵的行数
{
	return rowNum;
}


int Matrixtt::getColNum() const//返回矩阵的列数
{
	return colNum;
}

Matrixtt Matrixtt::Trans() const //矩阵转置
{
	Matrixtt _copy = *this;
	//_copy.colNum = this->colNum;
	//_copy.rowNum = this->rowNum;
	for(int i = 1;i <= _copy.getRowNum();i++)
	{
		for(int j = 1;j <= _copy.getColNum();j++)
		{
			_copy.set(i,j,get(j,i));
		}
	}

	return _copy;
}

double Matrixtt::get(int i,int j) const//返回矩阵第i行j列的元素
{
	int r = i -1;
	int c = j -1;
	return item[r*colNum + c];
}

void Matrixtt::set(int i,int j,double val)//设置矩阵第i行j列的元素
{
	int r = i-1;
	int c = j-1;
	
	item[r*colNum + c] = val;
#if 0
	if(i == j)
	{
		printf("r=%d,c=%d, r*g+c = %d\n",r,c,r*colNum + c);
	}
#endif
}

Matrixtt Matrixtt::operator +(const Matrixtt &m)//矩阵相加
{
	if(m.getRowNum() != rowNum || m.getColNum() != colNum)
	{
		printf("can not plus !\n");
		return *this;
	}
	Matrixtt _copy = *this;
	for(int i = 1;i <= rowNum;i++)
	{
		for(int j = 1;j <= colNum;j++)
		{
			_copy.set(i,j,get(i,j)+m.get(i,j));
		}
	}

	return _copy;
}

Matrixtt Matrixtt::operator -(const Matrixtt &m)//矩阵相减
{
	if(m.getRowNum() != rowNum || m.getColNum() != colNum)
	{
		printf("can not subtract !\n");
		return *this;
	}
	Matrixtt _copy = *this;
	for(int i = 1;i <= rowNum;i++)
	{
		for(int j = 1;j <= colNum;j++)
		{
			_copy.set(i,j,get(i,j) - m.get(i,j));
		}
	}

	return _copy;
}

Matrixtt Matrixtt::operator *(const Matrixtt &m)//矩阵相乘
{
	if(colNum != m.getRowNum())
	{
		printf("can not * !\n");
		return *this;
	}
	Matrixtt _copy(rowNum,m.getColNum());
	for(int i = 1;i<=rowNum;i++)
	{
		for(int j = 1;j<=m.getColNum();j++)
		{
			double sum = 0.0;
			for(int k = 1;k <= colNum;k++)
			{
				sum += get(i,k)*m.get(k,j);
			}
			_copy.set(i,j,sum);
		}
	}

	return _copy;
}

Matrixtt Matrixtt::operator *(const double f)//矩阵乘以常数
{
	Matrixtt _copy = *this;
	for(int i = 1;i<=rowNum;i++)
	{
		for(int j = 1;j<=colNum;j++)
		{
			_copy.set(i,j,get(i,j)*f);
		}
	}

	return _copy;
}

Matrixtt Matrixtt::operator =(const Matrixtt &m)//矩阵复制
{
	rowNum = m.getRowNum();
	colNum = m.getColNum();
	if(this == &m)
	{
		return * this;
	}
	if(item != NULL)
	{
		delete [] item;
	}
	item = new double[rowNum*colNum];
	for(int i = 1;i<=rowNum;i++)
	{
		for(int j = 1;j<=colNum;j++)
		{
			set(i,j,m.get(i,j));
		}
	}

	return *this;
}

Matrixtt Matrixtt::addsubmatrix(int m,int n,const Matrixtt &M)
{
	Matrixtt _copy = *this;
	if( (m-1+M.getRowNum())>rowNum || (n-1+M.getColNum())>colNum)
	{
		printf("can not += !\n");
		return *this;
	}

	for(int i = 1;i<=M.getRowNum();i++)
	{
		for(int j = 1;j<=M.getColNum();j++)
		{
			_copy.set(m-1+i,n-1+j,get(m-1+i,n-1+j)+M.get(i,j));
		}
	}

	return _copy;
}



Matrixtt Matrixtt::Inverse(int &a)//矩阵求逆
{
	Matrixtt _copy = *this;
	
	//存放结果
	Matrixtt result(rowNum);
	if(colNum != rowNum)
	{
		printf("can not inverse ,not square!\n");
		a = 0;
		return *this;
	}

	for(int i = 1;i<=colNum;i++)
	{
		int MaxCol = i;
		double max = fabs(_copy.get(i,i));

		for(int j = i;j<=rowNum;j++)
		{
			if(fabs(_copy.get(j,i)) > max)
			{
				max = fabs(_copy.get(j,i));
				MaxCol = j;
			}
		}

		if(MaxCol != i)
		{
			result.RowSwap(i,MaxCol);
			_copy.RowSwap(i,MaxCol);
		}
		if(0 == _copy.get(i,i))
		{
			printf("the matrix dose not has inverse\n");
			a = 0;
			return *this;
		}
		double r = 1.0/_copy.get(i,i);
		_copy.RowSwap(i,-1,r);
		result.RowSwap(i,-1,r);

		for(int j = 1;j<=rowNum;j++)
		{
			if(j == i)
			{
				continue;
			}
			r = -_copy.get(j,i);
			_copy.RowSwap(i,j,r);
			result.RowSwap(i,j,r);
		}
	}
	printf("the matrix have inverse !\n");
	a = 1;

	return result;
}

void Matrixtt::RowSwap(int i,int j,double multiply)
{
	if (-1 == j)
	{
		for(int k = 1;k <=colNum;k++)
		{
			set(i,k,multiply*get(i,k));
		}
	}
	else
	{
		for(int k = 1;k<=colNum;k++)
		{
			set(j,k,multiply*get(i,k)+get(j,k));
		}
	}
}

void Matrixtt::RowSwap(int i,int j)//交换两行
{
	Matrixtt _copy = *this;
	for(int k = 1;k<=colNum;k++)
	{
		double swap = _copy.get(j,k);
		set(j,k,_copy.get(i,k));
		set(i,k,swap);
	}
}


// µÃµœœÇ¶ÈµÄÕýÏÒÖµ£»ÊäÈë·¶Î§(-180~180) œÇ¶È
double getdegreesin(int i)
{
	int degree;
	degree = (int)(Simu_normalize_degree(i));

	return sin(i*PI/180);             ///?????????////ÔõÃŽÓÅ»¯

}

// µÃµœœÇ¶ÈµÄÓàÏÒÖµ£»ÊäÈë·¶Î§(-180~180) œÇ¶È
double getdegreecos(int i)
{
	int degree;
	degree = (int)(Simu_normalize_degree(i));
	return cos(i*PI/180); ///?????????////ÔõÃŽÓÅ»¯

}

//µÃµœœÇ¶ÈµÄÕýÏÒÖµ£»ÊäÈë·¶Î§(-180~180)   »¡¶È
double getradianssin(double x)
{
	int i;
	i = (int) radians_to_degrees(x);
	return getdegreesin(i);
}

//µÃµœœÇ¶ÈµÄÓàÏÒÖµ£»ÊäÈë·¶Î§(-180~180)   »¡¶È
double getradianscos(double x)
{
	int i;
	i = (int) radians_to_degrees(x);
	return getdegreecos(i);
}




//ŽÓ»¡¶È±ä»¯µœœÇ¶È
double radians_to_degrees(double theta)
{
	return (theta * 180.0 / PI);
}

//ŽÓœÇ¶È±ä»¯µœ»¡¶È
double degrees_to_radians(double theta)
{
	return (theta * PI / 180.0);
}

/**************************************************
º¯ÊýÃû£º        clamp
ÃèÊö£º          °ÑYµÄÊýÖµÇ¯ÖÆÔÚ[X,Z]Ö®Œä£»         
**************************************************/
double clamp(double X, double Y, double Z)
{
	if (Y < X)
		return X;
	else if (Y > Z)
		return Z;
	return Y;
}








//œÇ¶È±ê×Œ»¯º¯Êý£¬ÊäÈë²ÎÊý£º·œÏòœÇ£¬ÈÎÒâœÇ¶È£»Êä³ö²ÎÊý£º·œÏòœÇ£¬·¶Î§ÔÚ[-PI ,PI)
double Simu_normalize_theta(double theta)
{
	double multiplier;
	if (theta >= -PI && theta < PI)
        return theta;
	
	multiplier = floor(theta / (2*PI));
	theta = theta - multiplier*2*PI;
	if (theta >= PI)
		theta -= 2*PI;
	if (theta < -PI)
		theta += 2*PI;
	
	return theta;
}

//œÇ¶È±ê×Œ»¯º¯Êý£¬ÊäÈë²ÎÊý£º·œÏòœÇ£¬ÈÎÒâœÇ¶È£»Êä³ö²ÎÊý£º·œÏòœÇ£¬·¶Î§ÔÚ[-180,180)
double Simu_normalize_degree(double theta)
{
	double multiplier;
	if (theta >= -180 && theta < 180)
        return theta;
	
	multiplier = floor(theta / 360);
	theta = theta - multiplier*2*180;
	if (theta >= 180)
		theta -= 2*180;
	if (theta < -180)
		theta += 2*180;
	
	return theta;
}

double Simu_norm(double X, double Y ) 
{
	double Z;
	Z = X*X+Y*Y; 
	Z = sqrt(Z);
	return Z;
}


//µÃµœŽÓÁœÌõÏßÖ®ŒäµÄŒÐœÇ£¬ÊäÈë²ÎÊý£ºÁœÌõÏßµÄ·œÏòœÇ£»Êä³ö²ÎÊý£ºÁœÌõÏßŒÐœÇ£¬·¶Î§ÔÚ(-180,180]
double GetLLAngle(double L1, double L2)
{
	double delta;
	delta = L1-L2;
	delta = Simu_normalize_theta(delta);
	return delta;
}




