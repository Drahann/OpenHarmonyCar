
#include "LinAlg.h"

LinAlg::LinAlg(void) {}

LinAlg::~LinAlg(void) {}
double LinAlg::sq(double v) { return v * v; }

/*
  xyt是机器人的世界坐标系坐标，有x/y/theta，p是激光点在机器人坐标系中的坐标，有x/y，此函数是要实现将点p实现从机器人坐标系转换到世界坐标系，
  theta是带+-的，是机器人坐标系需要旋转的角度，x,y是机器人坐标系坐标，x1,y1是旋转后的世界坐标系坐标，l是点p到原点的距离：
  x=l*cos(a), y=l*sin(a); x1=l*cos(a+t), y1=l*sin(a+t);
  x1=l*(cos(a)cos(t)-sin(a)sin(t)) = l*cos(a)cos(t)-l*sin(a)sin(t) =
  x*cos(t)-y*sin(t); y1=l*(sin(a)cos(t)+cos(a)sin(t)) =
  l*sin(a)cos(t)+l*cos(a)sin(t) = y*cos(t)+x*sin(t);
  再进行坐标系平移,机器人中心点在世界坐标系中的位置为x',y'；
  x1=x1+x';
  y1=y1+y';

*/
Pose LinAlg::transform(Pose &xyt, Pose &p) {
    Pose res;

    double c = cos(xyt.theta), s = sin(xyt.theta);

    res.x = p.x * c - p.y * s + xyt.x;
    res.y = p.x * s + p.y * c + xyt.y;
    res.theta = p.theta;

    return res;
}

/*
    将机器人坐标系中的点集points，根据机器人的坐标T，旋转平移，转化为世界坐标系中的坐标
*/
void LinAlg::transform(Pose &T, vector<Pose> &points, vector<Pose> &respoints) {

    for (int i = 0; i < points.size(); i++) {
        Pose tp;

        tp = LinAlg::transform(T, points.at(i)); //将单个点进行旋转平移变换
        respoints.push_back(tp);
    }
}

//貌似这个没用上
void LinAlg::transform(
    double T[4][4], vector<vector<double>> &points,
    vector<vector<double>> &newpoints) //激光坐标系转化为机器人坐标系中
{

    int num = points.size();
    for (int i = 0; i < num; i++) {
        vector<double> p = points.at(i);
        double pos[3];
        pos[0] = p.at(0);
        pos[1] = p.at(1);
        pos[2] = p.at(2);
        vector<double> res;
        transform(T, pos, 3, res);

        newpoints.push_back(res);
    }
}

void LinAlg::transform(
    double T[4][4], vector<vector<double>> &points,
    vector<Pose> &newpoints) //将激光坐标系坐标中的点，转换到机器人坐标系中
{

    int num = points.size();
    for (int i = 0; i < num; i++) {
        vector<double> p = points.at(i);
        double pos[2];
        pos[0] = p.at(0);
        pos[1] = p.at(1);
        // pos[2] = p.at(2);
        vector<double> res;
        transform(T, pos, 2, res);

        Pose newpt;
        newpt.x = res[0];
        newpt.y = res[1];
        newpt.theta = res[2];

        newpoints.push_back(newpt);
    }
}

void LinAlg::transform(
    double T[4][4], double p[], int npLength,
    vector<double>
        &res) //将激光坐标系中的点，转换到机器人中心坐标系中。即x+=1.5，单点
{
    // if (T.length == 4)
    {
        if (npLength == 2) {
            res.push_back(T[0][0] * p[0] + T[0][1] * p[1] + T[0][3]);
            res.push_back(T[1][0] * p[0] + T[1][1] * p[1] + T[1][3]);
            res.push_back(T[2][0] * p[0] + T[2][1] * p[1] + T[2][3]);
        } else if (npLength == 3) {
            res.push_back(T[0][0] * p[0] + T[0][1] * p[1] + T[0][2] * p[2] +
                          T[0][3]);
            res.push_back(T[1][0] * p[0] + T[1][1] * p[1] + T[1][2] * p[2] +
                          T[1][3]);
            res.push_back(T[2][0] * p[0] + T[2][1] * p[1] + T[2][2] * p[2] +
                          T[2][3]);
        } else if (npLength == 4) {
            res.push_back(T[0][0] * p[0] + T[0][1] * p[1] + T[0][2] * p[2] +
                          T[0][3]);
            res.push_back(T[1][0] * p[0] + T[1][1] * p[1] + T[1][2] * p[2] +
                          T[1][3]);
            res.push_back(T[2][0] * p[0] + T[2][1] * p[1] + T[2][2] * p[2] +
                          T[2][3]);
            res.push_back(1);
        }
    }
    /*	else if (T.length == 3)
            {
        if (p.length==2)
                    {
            return new double[] { T[0][0]*p[0] + T[0][1]*p[1] + T[0][2],
                                  T[1][0]*p[0] + T[1][1]*p[1] + T[1][2],
                                  T[2][0]*p[0] + T[2][1]*p[1] + T[2][2] };
        }
        Assert(false);
    }

    Assert(false);
    return null;*/
}

void LinAlg::quatPosToMatrix(double q[], double pos[], double M[4][4]) {
    // double M[4][4];
    double w = q[0], x = q[1], y = q[2], z = q[3];

    M[0][0] = w * w + x * x - y * y - z * z;
    M[0][1] = 2 * x * y - 2 * w * z;
    M[0][2] = 2 * x * z + 2 * w * y;

    M[1][0] = 2 * x * y + 2 * w * z;
    M[1][1] = w * w - x * x + y * y - z * z;
    M[1][2] = 2 * y * z - 2 * w * x;

    M[2][0] = 2 * x * z - 2 * w * y;
    M[2][1] = 2 * y * z + 2 * w * x;
    M[2][2] = w * w - x * x - y * y + z * z;

    if (pos != NULL) {
        M[0][3] = pos[0];
        M[1][3] = pos[1];
        M[2][3] = pos[2];
    }
    M[3][0] = 0;
    M[3][1] = 0;
    M[3][2] = 0;
    M[3][3] = 1;
}

double LinAlg::DistancePose(Pose a, Pose b) {
    return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}
double LinAlg::magnitude(int a[], int size) {
    double mag = 0;

    for (int i = 0; i < size; i++) {
        mag += a[i] * a[i];
    }

    return sqrt(mag);
}
void LinAlg::resize(double v[], int vLength, int newlength, double *pRes,
                    int *pResLength) {
    pRes = new double[newlength];
    *pResLength = min(newlength, vLength);
    for (int i = 0; i < min(newlength, vLength); i++)
        pRes[i] = v[i];
    return;
}

/* int LinAlg::min(int a, int b)
 {
         if(a<b)
                 return a;
         else
                 return b;


 }*/
//计算位姿a到b的增量，增量为a位姿坐标系下的增量
Pose LinAlg::xytInvMul31(Pose &a, Pose &b) {
    Pose res;
    return xytInvMul31(a, b, res);
}

/** compute:  X = xytMultiply(xytInverse(a), b) **/
Pose LinAlg::xytInvMul31(Pose &a, Pose &b, Pose &res) {

    double theta = a.theta;
    double ca = cos(theta), sa = sin(theta);
    double dx = b.x - a.x;
    double dy = b.y - a.y;

    res.x = ca * dx + sa * dy;
    res.y = -sa * dx + ca * dy;
    res.theta = b.theta - a.theta;

    return res;
}

//通过位姿和位姿增量，求出增加后的位姿
Pose LinAlg::xytMultiply(Pose a, Pose b) {

    Pose r;
    double s = sin(a.theta), c = cos(a.theta);

    r.x = c * b.x - s * b.y + a.x;
    r.y = s * b.x + c * b.y + a.y;
    r.theta = a.theta + b.theta;

    return r;
}

/*<FUNC+>*******************************************************
 * 函数名称: manhattenDistance
 * 功能描述: 数组中所有差值的�?
 * 输入参数: double* a：数�? a，int a_len:a的长�?
 *			double* b：数�? b，int b_len：b的长�?
 * 输出参数: double dist:a数组与b数组对应下标的差值绝对�?�的�?
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double LinAlg::manhattenDistance(double *a, int a_len, double *b, int b_len) {
    if (a_len != b_len) {
        return 0;
    }

    double dist = 0;
    for (int i = 0; i < a_len; i++)
        dist += abs(b[i] - a[i]);

    return dist;
}

int LinAlg::sq(int v) {
    int sqv = v * v;

    return sqv;
}

/*<FUNC+>*******************************************************
 * 函数名称: squaredDistance
 * 功能描述: 获取等长数组的差的平方和
 * 输入参数: int* a,int* b
 *
 * 输出参数: int mag：两列数组的差的平方�?
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
int LinAlg::squaredDistance(int *a, int *b, int len) {
    int mag = 0;

    for (int i = 0; i < len; i++)
        mag += sq(b[i] - a[i]);

    return mag;
}

double LinAlg::squaredDistance(double *a, double *b, int len) {
    double mag = 0;

    for (int i = 0; i < len; i++)
        mag += sq(b[i] - a[i]);

    return mag;
}

/*<FUNC+>*******************************************************
 * 函数名称: distance
 * 功能描述:
 * 输入参数:
 *
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double LinAlg::distance(int *a, int *b, int len) {
    int sqd = squaredDistance(a, b, len);
    return sqrt((double)sqd);
}

double LinAlg::distance(double *a, double *b, int len) {
    double sqd = squaredDistance(a, b, len);
    return sqrt(sqd);
}

/*<FUNC+>*******************************************************
 * 函数名称: abs
 * 功能描述: 将一维�?�二维数组的数�?�取绝对值后输出
 * 输入参数: psrc：源数组 pDest：处理后的目标数�?
 *			int Row:二维数组行数，int Col：二维数组列�?
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void LinAlg::Arrabs(double **pSrc, double **pDest, int Row, int Col) {
    if (Row == 0)
        return;

    for (int i = 0; i < Row; i++) {
        for (int j = 0; j < Col; j++) {
            pDest[i][j] = abs(pSrc[i][j]);
        }
    }
}
void LinAlg::Arrabs(double *pSrc, double *pDest, int len) {
    for (int i = 0; i < len; i++) {
        pDest[i] = abs(pSrc[i]);
    }
}

void LinAlg::Arrabs(float *pSrc, float *pDest, int len) {
    for (int i = 0; i < len; i++) {
        pDest[i] = abs(pSrc[i]);
    }
}

/*<FUNC+>*******************************************************
 * 函数名称: normF
 * 功能描述: 数组的平方和
 * 输入参数: double* a:数组，int len：数组长�?
 *
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double LinAlg::normF(double *a, int len) {
    double mag = 0;

    for (int i = 0; i < len; i++)
        mag += sq(a[i]);

    return mag;
}

/*<FUNC+>*******************************************************
 * 函数名称: magnitude
 * 功能描述: 求数组的平方�?+�?根号
 * 输入参数: int* a：数组首指针 int len：数组长�? a[] = {a1,a2,...,an}
 *  mag = (a1*a1+a2*a2+...+an*an)的½次�?
 * 输出参数: 数组的平方和+�?根号
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
template <typename T> double LinAlg::magnitude(T *a, int len) {
    double mag = 0;

    for (int i = 0; i < len; i++)
        mag += sq(a[i]);

    return sqrt(mag);
}

/*<FUNC+>*******************************************************
 * 函数名称: average
 * 功能描述: 求数组平均�??
 * 输入参数: T* p:数组首指针，int len：数组长�?
 *
 * 输出参数: T sum / len：平均�??
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
template <typename T> T LinAlg::average(T *a, int len) {
    T sum = 0;
    for (int i = 0; i < len; i++)
        sum += a[i];
    return sum / len;
}

/*<FUNC+>*******************************************************
 * 函数名称: normL1
 * 功能描述: 求数组绝对�?�的�?
 * 输入参数: T* p:数组首指针，int len：数组长�?
 *
 * 输出参数: T mag：数组绝对�?�的�?
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
template <typename T> T LinAlg::normL1(T *a, int len) {
    T mag = 0;
    for (int i = 0; i < len; i++)
        mag += abs(a[i]);
    return mag;
}

/*<FUNC+>*******************************************************
 * 函数名称: scale
 * 功能描述: 将数组（1�?2维）倍乘scaled后赋值给新的数组
 * 输入参数:
 *
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
template <class T>
void LinAlg::scale(T *pSrc, int len, T *pDest, double *scaled) {
    for (int i = 0; i < len; i++)
        pDest[i] = pSrc[i] * scaled[i];
}

void LinAlg::scale(double *pSrc, int len, double *pDest, double scaled) {
    for (int i = 0; i < len; i++)
        pDest[i] = pSrc[i] * scaled;
}

template <class T>
void LinAlg::scale(T **pSrc, int Row, int Col, T **pDest, double scaled) {
    for (int i = 0; i < Row; i++)
        for (int j = 0; j < Col; j++)
            pDest[i][j] = pSrc[i][j] * scaled;
}

/*<FUNC+>*******************************************************
 * 函数名称: normalizeL1
 * 功能描述: 获取每个数据占整体绝对�?�和的比�? a[]={a1,a2,...an}
 * pDest[] = { a1/l, a2/l,....,an/l } l = |a1|+|a2|+...+|an|
 *
 *
 * 输入参数:
 *
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
template <class T> void LinAlg::normalizeL1(T *a, int len, T *pDest) {
    scale(a, len, pDest, 1.0 / normL1(a, len));
}

/*<FUNC+>*******************************************************
 * 函数名称: normalize
 * 功能描述: a[]={a1,a2,...,an} pDest[]={ a1/mag }mag见magnitude
 * 输入参数:
 *
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
template <typename T> void LinAlg::normalize(T *a, int len, T *pDest) {
    T mag = magnitude(a);

    for (int i = 0; i < len; i++)
        pDest[i] = a[i] / mag;
}

/*<FUNC+>*******************************************************
 * 函数名称: normalizeEquals
 * 功能描述: a[]={a1,a2,...,an} a[]={ a1/mag }mag见magnitude
 * 输入参数:
 *
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
template <typename T> void LinAlg::normalizeEquals(T *a, int len) {
    T mag = magnitude(a);

    for (int i = 0; i < len; i++)
        a[i] = a[i] / mag;
}

/*<FUNC+>*******************************************************
 * 函数名称: add
 * 功能描述: 将数组的每个元素相加
 * 输入参数:
 *
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void LinAlg::add(double *a, double *b, int len, double *pSum) {
    for (int i = 0; i < len; i++) {
        pSum[i] = a[i] + b[i];
    }
}

void LinAlg::add(double **a, double **b, int Row, int Col, double **pSum) {
    for (int i = 0; i < Row; i++) {
        for (int j = 0; j < Col; j++) {
            pSum[i][j] = a[i][j] + b[i][j];
        }
    }
}

/*<FUNC+>*******************************************************
 * 函数名称: subtract
 * 功能描述: 二维数组相减 a-b
 * 输入参数:
 *
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
void LinAlg::subtract(double **a, double **b, int Row, int Col,
                      double **pSubt) {
    for (int i = 0; i < Row; i++) {
        for (int j = 0; j < Col; j++) {
            pSubt[i][j] = a[i][j] - b[i][j];
        }
    }
}

void LinAlg::subtract(double *a, double *b, int len, double *pSum) {
    for (int i = 0; i < len; i++)
        pSum[i] = a[i] - b[i];
}

/*<FUNC+>*******************************************************
 * 函数名称:
 * 功能描述:
 * 输入参数:
 *
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
template <typename T> void LinAlg::copy(T *pSrc, int len, T *pDest) {
    for (int i = 0; i < len; i++) {
        pDest[i] = pSrc[i];
    }
}

/*<FUNC+>*******************************************************
 * 函数名称: dotProduct
 * 功能描述: 数组的点乘和
 * 输入参数:
 *
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/10/30        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double LinAlg::dotProduct(double *a, int len, double *b) {
    double mag = 0;
    for (int i = 0; i < len; i++)
        mag += a[i] * b[i];

    return mag;
}

/*<FUNC+>*******************************************************
 * 函数名称: GetGuassian
 * 功能描述: 获取高斯随机�?
 * 输入参数:
 *
 * 输出参数:
 *
 * �? �? �?:
 * 操作流程:
 * 其它说明: �?
 * 修改记录:
 * -------------------------------------------------------------
 *    2014/11/06        1.0           王磊          创建函数
 *<FUNC->*******************************************************
 */
double LinAlg::GetGuassian() {
    static double V1, V2, S;
    static int phase = 0;
    double X;

    if (phase == 0) {
        do {
            double U1 = (double)rand() / RAND_MAX;
            double U2 = (double)rand() / RAND_MAX;

            V1 = 2 * U1 - 1;
            V2 = 2 * U2 - 1;
            S = V1 * V1 + V2 * V2;
        } while (S >= 1 || S == 0);

        X = V1 * sqrt(-2 * log(S) / S);
    } else {
        X = V2 * sqrt(-2 * log(S) / S);
    }

    phase = 1 - phase;

    return X;
}
