#include "MultiResolutionScanMatcher.h"

#include <fstream>
bool compareweight(const particles &a, const particles &b) {
    return a.weight > b.weight;
}

MultiResolutionScanMatcher::MultiResolutionScanMatcher(void) {
    decimate = 1;
    debug = false;

    refine_minimum_stepsize[0] = 0.001;
    refine_minimum_stepsize[1] = 0.001;
    refine_minimum_stepsize[2] = MathUtil::toRadians(0.05);
    refine_shrink_ratio = 0.7;
    for (int i = 0; i < 81; i++) {
        VdHistogramFilterWeight[i] = (1.0 / 81.0);
        VdHistogramFilterTempWeight[i] = (0.0);
        ViHistogramFilterPoseTheta[i] = (0);
    }

    ratio = 0;
    m_ifrefine = 1;
}

MultiResolutionScanMatcher::~MultiResolutionScanMatcher(void) {}
void MultiResolutionScanMatcher::config() {
    refine_minimum_stepsize[0] = 0.001;
    refine_minimum_stepsize[1] = 0.001;
    refine_minimum_stepsize[2] = MathUtil::toRadians(0.05);
    refine_shrink_ratio = 0.7;
}
int MultiResolutionScanMatcher::bestIndices(vector<IntArray2D *> &vtScores,
                                            int *bestidx) {
    int bestscore = -9999999; // double.max
    int bestnum = 0;
    vector<int> tmpscore;
    for (int tidx = 0; tidx < vtScores.size(); tidx++) {
        for (int yidx = 0; yidx < vtScores.at(tidx)->m_dim1; yidx++) {
            for (int xidx = 0; xidx < vtScores.at(tidx)->m_dim2; xidx++) {
                int score = vtScores.at(tidx)->get(yidx, xidx);
                tmpscore.push_back(score);
                if (score > bestscore) {
                    bestscore = score;
                    bestidx[0] = xidx;
                    bestidx[1] = yidx;
                    bestidx[2] = tidx;
                    bestnum = vtScores.at(tidx)->getvd(yidx, xidx);
                }
            }
        }
    }
    return bestnum;
}

int MultiResolutionScanMatcher::bestIndices(vector<IntArray2D *> &vtScores,
                                            int *bestidx, int &bestscore,
                                            int &similar_num, int &bestnum) {
    vector<int> tmpscore;
    bestscore = -9999999; // double.max
    bestnum = 0;
    //����˵�ĵ�ǰ�㣬��ָ�����˵�λ�ˣ�ֻ������������չ�����е����λ�ñ�ʾ��
    for (
        int tidx = 0; tidx < vtScores.size();
        tidx++) //ȡ��ÿһ��Ԫ�أ�ÿ��Ԫ�������������������洢��һ�����󣬴�СΪ�ֵĽǶȵĸ���
    {
        for (int yidx = 0; yidx < vtScores.at(tidx)->m_dim1;
             yidx++) //��ǰ�������
        {
            for (int xidx = 0; xidx < vtScores.at(tidx)->m_dim2;
                 xidx++) //��ǰ�������
            {
                int score =
                    vtScores.at(tidx)->get(yidx, xidx); //ȡ����ǰ����ĵ�ǰ���vs�е�ֵ
                tmpscore.push_back(score); //��ֵѹջ��ÿ���Ƕ��µ�ÿ�����vsֵ
                if (score > bestscore) {
                    bestscore = score;
                    bestidx[0] = xidx; //���ֵ�ĵ�����xֵ
                    bestidx[1] = yidx; //���ֵ�ĵ�����yֵ
                    bestidx[2] = tidx; //�������ڵڼ�������������Ƕȼ����й�ϵ
                    bestnum = vtScores.at(tidx)->getvd(
                        yidx, xidx); //���ֵ���ĵ㣬ƥ�����˶��ٸ������
                }
            }
        }
    }
    int j = 0;

    //��������ֵ�ӽ�95%���ϵĵ�ĸ���������ǶȲ�һ��һ������Ϊ�����нǶ��µ����е����ɸѡ
    for (int i = 0; i < tmpscore.size(); i++) {
        if ((double)tmpscore[i] > (bestscore * 0.95)) {
            j++;
        }
    }
    similar_num = j;
    return 1;
}

/**********************************************************************
* Function: 	 HistogramFilterBestIndices
* Description:  	robot location
* Table Accessed:
* Table Updated:
* Input: 	 vector<IntArray2D*> &vtScores,
* Output: 	 int *bestidx,
                 int &bestscore,
                 int &similar_num,
                 int &bestnum
* Return: 	0/1
* Others:
* Modify Date    Version    Author	      Modification
* -----------------------------------------------
* 2018/07/17	   V1.0	     WangZT	       new add
**********************************************************************/
int MultiResolutionScanMatcher::HistogramFilterBestIndices(
    vector<IntArray2D *> &vtScores, int *bestidx, int &bestscore,
    int &similar_num, int &bestnum) {
    vector<int> tmpscore;
    bestscore = -9999999; // double.max
    bestnum = 0;
    /*
vector<double> VdHistogramFilterWeight;
vector<double> VdHistogramFilterTempWeight;
    vector<int> ViHistogramFilterPoseTheta;
    */

    //����˵�ĵ�ǰ�㣬��ָ�����˵�λ�ˣ�ֻ������������չ�����е����λ�ñ�ʾ��
    for (
        int tidx = 0; tidx < vtScores.size();
        tidx++) //ȡ��ÿһ��Ԫ�أ�ÿ��Ԫ�������������������洢��һ�����󣬴�СΪ�ֵĽǶȵĸ���
    {
        for (int yidx = 0; yidx < vtScores.at(tidx)->m_dim1;
             yidx++) //��ǰ�������
        {
            for (int xidx = 0; xidx < vtScores.at(tidx)->m_dim2;
                 xidx++) //��ǰ�������
            {
                int score =
                    vtScores.at(tidx)->get(yidx, xidx); //ȡ����ǰ����ĵ�ǰ���vs�е�ֵ
                if (score > VdHistogramFilterTempWeight
                                [yidx * (vtScores.at(tidx)->m_dim2) + xidx]) {
                    VdHistogramFilterTempWeight
                        [yidx * (vtScores.at(tidx)->m_dim2) + xidx] =
                            score; //��������TempWeight
                    ViHistogramFilterPoseTheta[yidx *
                                                   (vtScores.at(tidx)->m_dim2) +
                                               xidx] =
                        tidx; //��������weight��Ӧ�ĽǶ�
                }

                tmpscore.push_back(score); //��ֵѹջ��ÿ���Ƕ��µ�ÿ�����vsֵ
                if (score > bestscore) {
                    bestscore = score;
                    bestnum = vtScores.at(tidx)->getvd(
                        yidx, xidx); //���ֵ���ĵ㣬ƥ�����˶��ٸ������
                }
            }
        }
    }

    if (0 == bestnum) {
        for (int yidx = 0; yidx < vtScores.at(0)->m_dim2; yidx++) {
            for (int xidx = 0; xidx < vtScores.at(0)->m_dim1; xidx++) {
                VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                        xidx] = 0.9;
            }

            VdHistogramFilterWeight[40] = 1.0;
        }
        bestidx[0] = 4;
        bestidx[1] = 4; // fixme:����������£��Ƕ���ô��
        bestidx[2] = 15;

        m_ifrefine = 0;

        for (int i = 0; i < 81; i++) {
            VdHistogramFilterTempWeight[i] = (0.0);
            ViHistogramFilterPoseTheta[i] = (0);
        }

        int j = 0;

        //��������ֵ�ӽ�95%���ϵĵ�ĸ���������ǶȲ�һ��һ������Ϊ�����нǶ��µ����е����ɸѡ
        for (int i = 0; i < tmpscore.size(); i++) {
            if ((double)tmpscore[i] > (bestscore * 0.95)) {
                j++;
            }
        }
        similar_num = j;
        return 0;
    }
    int totalscores = 0;
    double scores = 0.0;

    for (int yidx = 0; yidx < vtScores.at(0)->m_dim2; yidx++) {
        for (int xidx = 0; xidx < vtScores.at(0)->m_dim1; xidx++) {

            VdHistogramFilterTempWeight[yidx * (vtScores.at(0)->m_dim2) +
                                        xidx] *=
                VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) + xidx];
            totalscores +=
                VdHistogramFilterTempWeight[yidx * (vtScores.at(0)->m_dim2) +
                                            xidx];
        }
    }

    //��һ��
    int zeronum = 0;
    for (int yidx = 0; yidx < vtScores.at(0)->m_dim2; yidx++) {
        for (int xidx = 0; xidx < vtScores.at(0)->m_dim1; xidx++) {
            VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) + xidx] =
                VdHistogramFilterTempWeight[yidx * (vtScores.at(0)->m_dim2) +
                                            xidx] /
                totalscores;
            if (VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                        xidx] > scores) {
                scores =
                    VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                            xidx];
                bestidx[0] = xidx; //��λ���꣬��theta��
                bestidx[1] = yidx;
            }
            if (0.0 == VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                               xidx]) {
                // zeronum ++;
            }
        }
    }

    bestidx[2] =
        ViHistogramFilterPoseTheta[bestidx[1] * (vtScores.at(0)->m_dim2) +
                                   bestidx[0]]; //����theta

    //�жϻ����ĺû�
    if (100 < bestnum) //���������ߵ���
    {
        for (int yidx = 0; yidx < vtScores.at(0)->m_dim2; yidx++) {
            for (int xidx = 0; xidx < vtScores.at(0)->m_dim1; xidx++) {
                if (VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                            xidx] < 0.7 * scores) {
                    VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                            xidx] = 0.8 * scores;
                }
                if (VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                            xidx] < 0.8 * scores) {
                    VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                            xidx] = 0.85 * scores;
                }
#if 1
                if (VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                            xidx] < 0.85 * scores) {
                    VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                            xidx] = 0.90 * scores;
                }
#endif
#if 1
                if (VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                            xidx] < 0.90 * scores) {
                    VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                            xidx] = 0.95 * scores;
                }
#endif
            }
        }
        if (bestidx[0] != 4 || bestidx[1] != 4) // swap
        {
            int x = bestidx[0];
            int y = bestidx[1];
            VdHistogramFilterWeight[y * (vtScores.at(0)->m_dim2) + x] =
                VdHistogramFilterWeight[4 * (vtScores.at(0)->m_dim2) + 4];
            VdHistogramFilterWeight[4 * (vtScores.at(0)->m_dim2) + 4] = scores;
        }
        m_ifrefine = 1;
    } else //����ƥ��Ĳ��ã�������
    {
        for (int yidx = 0; yidx < vtScores.at(0)->m_dim2; yidx++) {
            for (int xidx = 0; xidx < vtScores.at(0)->m_dim1; xidx++) {
                VdHistogramFilterWeight[yidx * (vtScores.at(0)->m_dim2) +
                                        xidx] = 0.99;
            }

            VdHistogramFilterWeight[40] = 1.0;
        }
        bestidx[0] = 4;
        bestidx[1] = 4; // fixme:����������£��Ƕ���ô��
        bestidx[2] = 15;
        printf("mode = encode\n");

        m_ifrefine = 0;
    }

    ratio++;

#if 1
    if (1 == ratio) {
        if (200 > bestnum) {
            printf("bestnum = %d\n", bestnum);
        }

        if (4 != bestidx[0] || 4 != bestidx[1]) {
            printf("best = %d , %d\n", bestidx[0], bestidx[1]);
        }

        ratio = 0;
    }
#endif
    for (int i = 0; i < 81; i++) {
        VdHistogramFilterTempWeight[i] = (0.0);
        ViHistogramFilterPoseTheta[i] = (0);
    }

    int j = 0;

    //��������ֵ�ӽ�95%���ϵĵ�ĸ���������ǶȲ�һ��һ������Ϊ�����нǶ��µ����е����ɸѡ
    for (int i = 0; i < tmpscore.size(); i++) {
        if ((double)tmpscore[i] > (bestscore * 0.95)) {
            j++;
        }
    }
    similar_num = j;
    return 1;
}

int MultiResolutionScanMatcher::bestIndices(IntArray2D *pScores, int *bestidx) {
    double bestscore = -999999999999.0; // double.max

    for (int yidx = 0; yidx < pScores->m_dim1; yidx++) {
        for (int xidx = 0; xidx < pScores->m_dim2; xidx++) {
            int score = pScores->get(yidx, xidx);
            if (score > bestscore) {
                bestscore = score;
                bestidx[0] = xidx;
                bestidx[1] = yidx;
            }
        }
    }

    return 1;
}

bool saveMap1(GridMap &gm) {

    ofstream outFile;

    outFile.open("haha2.txt", ios::out);

    if (!outFile) {
        return false;
    }

    outFile << 15;
    outFile << ' ';
    outFile << gm.metersPerPixel;
    outFile << ' ';
    outFile << gm.height;
    outFile << ' ';
    outFile << gm.width;
    outFile << ' ';
    outFile << gm.metersPerPixel;
    outFile << ' ';
    outFile << gm.x0;
    outFile << ' ';
    outFile << gm.y0;
    outFile << endl;

    for (int j = 0; j < gm.height; j++) {
        for (int i = 0; i < gm.width; i++) {
            int status = 0;
            status = gm.data[j * gm.width + i];
            { outFile << status; }
            outFile << ' ';
        }
        outFile << endl;
    }

    outFile.close();
    return true;
}
void MultiResolutionScanMatcher::setModel(
    GridMap &tmp) // GridMap:double x0,y0;
                  //         double metersPerPixel
                  //         int    width,height
                  //         BYTE   *data
                  //         BYTE   defaultFill
{
    //�����gm�Ƕ����ڴ�ͷ�ļ��еģ���MultiResolutionScanMatcher.h�У�һ���봫������GridMap�Ƕ����ڲ�ͬ��ͷ�ļ��У�����һ����
    gm = tmp;
    if (decimate <= 1) { // decimate=1
        dgm = gm;
    } else {
        gm.decimateMax(decimate, dgm);
        dgm.max4(dgm); // necessary to avoid quantization problems.
    }
}
void MultiResolutionScanMatcher::refineFunc(vector<Pose> &points, double x,
                                            double y, double t,
                                            Pose &posepriorxyt, double **pinv,
                                            Pose &poseRes, int mode) {
    double score = gm.score(points, x, y, t, posepriorxyt, pinv);
    double stepsize[3] = {0};
    int refine_max_iterations = 1000; //��֤ѭ�������㹻�ã�ʵ��ѭ��������ô���
    double newxyts[6][3];

    int iterations = 0;
    /*********/
    double x0 = x; //�������ʼ��λ��
    double y0 = y;
    double t0 = t;
    /******/
    stepsize[0] = 0.1;
    stepsize[1] = 0.1;
    stepsize[2] = MathUtil::toRadians(2);

    for (iterations = 0; iterations < refine_max_iterations; iterations++) {

        newxyts[0][0] = x + stepsize[0];
        newxyts[0][1] = y;
        newxyts[0][2] = t;

        newxyts[1][0] = x - stepsize[0];
        newxyts[1][1] = y;
        newxyts[1][2] = t;

        newxyts[2][0] = x;
        newxyts[2][1] = y + stepsize[1];
        newxyts[2][2] = t;

        newxyts[3][0] = x;
        newxyts[3][1] = y - stepsize[1];
        newxyts[3][2] = t;

        newxyts[4][0] = x;
        newxyts[4][1] = y;
        newxyts[4][2] = t + stepsize[2];

        newxyts[5][0] = x;
        newxyts[5][1] = y;
        newxyts[5][2] = t - stepsize[2];

        // move in the best direction. (Roughly a local gradient search.)
        bool stepped = false;
        for (int i = 0; i < 6; i++) {

            if (1 == mode) {
                double distance;
                distance = sqrt((newxyts[i][0] - x0) * (newxyts[i][0] - x0) +
                                (newxyts[i][1] - y0) * (newxyts[i][1] - y0));
                if (0.10 < distance)
                    continue;
            }
            double newscore = gm.score(points, newxyts[i][0], newxyts[i][1],
                                       newxyts[i][2], posepriorxyt, pinv);

            //�ҵ�����ƥ���λ��
            if (newscore > score) {
                stepped = true;
                score = newscore;
                x = newxyts[i][0];
                y = newxyts[i][1];
                t = newxyts[i][2];
            }
        }

        // That step was good. Keep going at the same step size.
        if (stepped)
            continue;

        // We've rejected that step. Reduce our step size.
        bool searchMore = false;
        for (int i = 0; i < 3; i++) {

            if (stepsize[i] > refine_minimum_stepsize[i]) {

                searchMore =
                    true; //�������һ���������ڶ������С����������С��������������

                // refine_minimum_stepsize[0]/[1]=0.001
                // refine_minimum_stepsize[2]=0.05
                stepsize[i] =
                    max(refine_minimum_stepsize[i],
                        stepsize[i] *
                            refine_shrink_ratio); // refine_shrink_ratio=0.7
            }
        }

        if (!searchMore) //������ٽ������������˳�ѭ��
            break;
    }

    //ƥ����õ�λ��
    poseRes.x = x;
    poseRes.y = y;
    poseRes.theta = t;

    return;
    // new double[] {x, y, t};
}

void MultiResolutionScanMatcher::HistogramRefineFunc(
    vector<Pose> &points, double x, double y, double t, Pose &posepriorxyt,
    double **pinv, Pose &poseRes, int mode) {
    double score = gm.score(points, x, y, t, posepriorxyt, pinv);
    double stepsize[3] = {0};
    int refine_max_iterations = 1000; //��֤ѭ�������㹻�ã�ʵ��ѭ��������ô���
    double newxyts[6][3];

    int iterations = 0;
    /*********/
    double x0 = x; //�������ʼ��λ��
    double y0 = y;
    double t0 = t;
    /******/
    stepsize[0] = 0.1;
    stepsize[1] = 0.1;
    stepsize[2] = MathUtil::toRadians(2);

    //        double weights[] = computeWeights(points);

    for (iterations = 0; iterations < refine_max_iterations; iterations++) {

        newxyts[0][0] = x + stepsize[0];
        newxyts[0][1] = y;
        newxyts[0][2] = t;

        newxyts[1][0] = x - stepsize[0];
        newxyts[1][1] = y;
        newxyts[1][2] = t;

        newxyts[2][0] = x;
        newxyts[2][1] = y + stepsize[1];
        newxyts[2][2] = t;

        newxyts[3][0] = x;
        newxyts[3][1] = y - stepsize[1];
        newxyts[3][2] = t;

        newxyts[4][0] = x;
        newxyts[4][1] = y;
        newxyts[4][2] = t + stepsize[2];

        newxyts[5][0] = x;
        newxyts[5][1] = y;
        newxyts[5][2] = t - stepsize[2];

        // move in the best direction. (Roughly a local gradient search.)
        bool stepped = false;

        for (int i = 0; i < 6; i++) {

            if (1 == mode) {
                double distance;
                distance = sqrt((newxyts[i][0] - x0) * (newxyts[i][0] - x0) +
                                (newxyts[i][1] - y0) * (newxyts[i][1] - y0));
                if (0.05 < distance)
                    continue;
            }
            if (2 == mode) {
                double distance;
                distance = sqrt((newxyts[i][0] - x0) * (newxyts[i][0] - x0) +
                                (newxyts[i][1] - y0) * (newxyts[i][1] - y0));
                if (0.1 < distance)
                    continue;
            }
            if (3 == mode) {
                double distance;
                distance = sqrt((newxyts[i][0] - x0) * (newxyts[i][0] - x0) +
                                (newxyts[i][1] - y0) * (newxyts[i][1] - y0));
                if (0.2 < distance)
                    continue;
            }

            double newscore = gm.score(points, newxyts[i][0], newxyts[i][1],
                                       newxyts[i][2], posepriorxyt, pinv);

            //�ҵ�����ƥ���λ��
            if (newscore > score) {
                stepped = true;
                score = newscore;
                x = newxyts[i][0];
                y = newxyts[i][1];
                t = newxyts[i][2];
            }
        }

        // That step was good. Keep going at the same step size.
        if (stepped)
            continue;

        // We've rejected that step. Reduce our step size.
        bool searchMore = false;

        for (int i = 0; i < 3; i++) {

            if (stepsize[i] > refine_minimum_stepsize[i]) {

                searchMore =
                    true; //�������һ���������ڶ������С����������С��������������

                // refine_minimum_stepsize[0]/[1]=0.001
                // refine_minimum_stepsize[2]=0.05
                stepsize[i] =
                    max(refine_minimum_stepsize[i],
                        stepsize[i] *
                            refine_shrink_ratio); // refine_shrink_ratio=0.7
            }
        }

        if (!searchMore) //������ٽ������������˳�ѭ��
            break;
    }

    //ƥ����õ�λ��
    poseRes.x = x;
    poseRes.y = y;
    poseRes.theta = t;

    return;
    // new double[] {x, y, t};
}

void MultiResolutionScanMatcher::matchRaw(vector<Pose> &points,
                                          Pose &posepriorxyt, double **pinv,
                                          double xrange, double yrange,
                                          double thetaRange,
                                          double thetaResolution, Pose &resPose,
                                          vector<double> &data_flag, int mode) {

    int max_search_iterations = 100;
    int decimate = 1;
    double metersPerPixel =
        dgm.metersPerPixel; // dgm����Ϣ����proccessScan���drawScan���ͨ��matcher.setModel(gm)����gm��������
    double lowResXYT[3];
    double priorxyt[3];
    int pointsize = points.size();

    //��һ���ڣ������˵�λ��
    priorxyt[0] = posepriorxyt.x;
    priorxyt[1] = posepriorxyt.y;
    priorxyt[2] = posepriorxyt.theta;

    //��������ʼλ��
    lowResXYT[0] = priorxyt[0] - xrange;
    lowResXYT[1] = priorxyt[1] - yrange;
    lowResXYT[2] = priorxyt[2] - thetaRange;

    //����������ÿһ��Ԫ�������������飬��һ�������ʾ�ڵ�ǰ�Ƕ��£�ÿ����Ϊ������ԭ��ʱ��ƥ���ϵĵ��ֵ�ĺ�
    //�ڶ��������ʾ�ڵ�ǰ�Ƕ��£�ÿ����Ϊ������ԭ��ʱ��ƥ���ϵĵ�ĸ���
    vector<IntArray2D *>
        vtlowResScores; // class IntArray2D: int *vs; //vs = new
                        // int[m_dim1*dim2];
                        //                   int *vd; //vd = new
                        //                   int[m_dim1*dim2]; int
                        //                   m_dim1,m_dim2;

    dgm.scores3D(points, lowResXYT[0],
                 (int)(2 * xrange / metersPerPixel + 1), //��չ����Ĵ�С
                 lowResXYT[1], (int)(2 * yrange / metersPerPixel + 1),
                 lowResXYT[2], thetaResolution,
                 max(1, (int)(2 * thetaRange / thetaResolution)), posepriorxyt,
                 pinv, vtlowResScores); // vtlowResScores������

    CMultiGaussionEstimator mge(3);
    /////////////////////////////////////////////////////////////////
    // Step 1. Compute the covariance by fitting a Gauassian to the
    // low-resolution samples.
    // MultiGaussianEstimator mge = new MultiGaussianEstimator(3);

    /////////////////////////////////////////////////////////////////
    // If no decimation has been requested, return a result now.
    if (decimate <= 1) {
        int bestidx0[3];
        double match_accuracy = 0.0, score_accuracy;
        int similar_num = 0;
        int bestscore = 0;
        int bestnum = 0;
        double vaildlaser_accuracy = 0.0;

        bestIndices(vtlowResScores, bestidx0, bestscore, similar_num, bestnum);

        match_accuracy =
            (double)bestnum /
            pointsize; //��ȷ�ȣ�λ��ƥ����õĵ㣬ƥ���ϵĸ��������ϼ�������Ч����
        vaildlaser_accuracy = (double)pointsize /
                              1081.0; //��Ч���⾫ȷ�ȣ���������Ч�����������ܸ���1081

        // double������
        data_flag.clear();
        // data_flag.push_back(vaildlaser_accuracy); //��Ч���⾫ȷ��
        data_flag.push_back((double)similar_num);
        data_flag.push_back(match_accuracy); //ƥ�侫ȷ��
        data_flag.push_back((double)pointsize);
        // data_flag.push_back((double)similar_num); //���ֵ����λ�˸���

        double xyt0score = vtlowResScores.at(bestidx0[2])
                               ->get(bestidx0[1], bestidx0[0]); //ȡbestscore

        //ƥ����õĻ�����λ��
        double u[3];
        u[0] =
            lowResXYT[0] +
            bestidx0[0] *
                metersPerPixel; //������դ�񳤶�Ϊ��λ���еĶ�λ�����ܻ���ƫ�������о���λ
        u[1] = lowResXYT[1] + bestidx0[1] * metersPerPixel;
        u[2] = lowResXYT[2] + bestidx0[2] * thetaResolution;

        if (false) {
            double xyt[3];

            for (int tidx = 0; tidx < vtlowResScores.size(); tidx++) {
                xyt[2] = lowResXYT[2] + tidx * thetaResolution;

                for (int yidx = 0; yidx < vtlowResScores.at(tidx)->m_dim1;
                     yidx++) {
                    xyt[1] = lowResXYT[1] + yidx * dgm.metersPerPixel;

                    for (int xidx = 0; xidx < vtlowResScores.at(tidx)->m_dim2;
                         xidx++) {

                        xyt[0] = lowResXYT[0] + xidx * dgm.metersPerPixel;
                        // int v = lowResScores[tidx].get(yidx, xidx);
                        int v = vtlowResScores.at(tidx)->get(yidx, xidx);
                        mge.observeWeighted(xyt, 3, MathUtil::exp(-256.0 + v));
                    }
                }
            }
        }

        //�����ˣ��ͷ�ƥ���λ�˽��
        if (vtlowResScores.size() > 0) {
            for (int i = 0; i < vtlowResScores.size(); i++) {
                IntArray2D *pArray = vtlowResScores.at(i);
                if (pArray != NULL) {
                    delete pArray;
                }
            }
        }

        Pose xytRes;

        refineFunc(points, u[0], u[1], u[2], posepriorxyt, NULL, xytRes, mode);
        resPose.x = xytRes.x;
        resPose.y = xytRes.y;

        resPose.theta = xytRes.theta;

        return;
        // Heuristic: posterior's mean is the MLE, posterior's
        // covariance is the fit covariance.
        // MultiGaussian mg = mge.getEstimate();
        // return new MultiGaussian(mg.getCovariance(), u);
    }

    /////////////////////////////////////////////////////////////////
    // Step 3. Search promising low-resolution voxels at high resolution.
    // Consider the peaks at low resolution in decreasing order. For each,
    // compute the corresponding cell at high resolution.
    int bestLowResIdx[3];
    int bestHighResScore = -1;
    double bestHighResXYT[3];

    for (int iters = 0; true; iters++) {
        // Find the next best score that's less than maxscore
        // This implementation just researches the entire low
        // resolution volume. We could do better with a MaxHeap
        // type data structure, but emperically, this does not
        // appear to take a significant amount of time.
        // Find the next best score that's less than maxscore
        // This implementation just researches the entire low
        // resolution volume. We could do better with a MaxHeap
        // type data structure, but emperically, this does not
        // appear to take a significant amount of time.
        int thisBestLowResScore = -1;

        for (int tidx = 0; tidx < vtlowResScores.size(); tidx++) {
            for (int yidx = 0; yidx < vtlowResScores.at(tidx)->m_dim1; yidx++) {
                for (int xidx = 0; xidx < vtlowResScores.at(tidx)->m_dim2;
                     xidx++) {
                    int score = vtlowResScores.at(tidx)->get(yidx, xidx);
                    if (score > thisBestLowResScore) {
                        thisBestLowResScore = score;
                        bestLowResIdx[0] = xidx;
                        bestLowResIdx[1] = yidx;
                        bestLowResIdx[2] = tidx;
                    }
                }
            }
        }

        if (iters > max_search_iterations ||
            bestHighResScore >= thisBestLowResScore) {
            // we're done: we have not found another low
            // resolution voxel that needs to be searched. Thus,
            // we return a result now.
            //               MultiGaussian mg = mge.getEstimate();
            //               return new MultiGaussian(mg.getCovariance(),
            //               bestHighResXYT);

            resPose.x = bestHighResXYT[0];
            resPose.y = bestHighResXYT[1];
            resPose.theta = bestHighResXYT[2];

            if (vtlowResScores.size() > 0) {
                for (int i = 0; i < vtlowResScores.size(); i++) {
                    IntArray2D *pArray = vtlowResScores.at(i);
                    if (pArray != NULL) {
                        delete pArray;
                    }
                }
            }

            double *pmgMeanN = new double[3];
            CMatrix *P_Cov2N = new CMatrix();
            MultiGaussian *pmgN;
            pmgN = mge.getEstimate();
            pmgN->getCovariance(P_Cov2N);

            MultiGaussian mg2N(P_Cov2N, bestHighResXYT, 3);

            mg2N.getMean(pmgMeanN, 3);

            delete pmgMeanN;
            delete P_Cov2N;
            delete pmgN;

            resPose.x = pmgMeanN[0];
            resPose.y = pmgMeanN[1];
            resPose.theta = pmgMeanN[2];
        }

        vtlowResScores.at(bestLowResIdx[2])
            ->set(bestLowResIdx[1], bestLowResIdx[0], -thisBestLowResScore);

        // evaluate this grid at high resolution.
        // xyt1 is the lower-left corner of the cell that contained the maximum.
        double xyt1[3];
        xyt1[0] = lowResXYT[0] + bestLowResIdx[0] * metersPerPixel;
        xyt1[1] = lowResXYT[1] + bestLowResIdx[1] * metersPerPixel;
        xyt1[2] = lowResXYT[2] + bestLowResIdx[2] * thetaResolution;

        IntArray2D *highResscores = NULL;
        highResscores = dgm.scores2D(points, xyt1[0], decimate, xyt1[1],
                                     decimate, xyt1[2], posepriorxyt, pinv);

        int thisBestHighResIdx[2];
        bestIndices(highResscores, thisBestHighResIdx);
        int thisBestHighResScore =
            highResscores->get(thisBestHighResIdx[1], thisBestHighResIdx[0]);

        if (thisBestHighResScore > bestHighResScore) {
            bestHighResScore = thisBestHighResScore;
            bestHighResXYT[0] =
                xyt1[0] + thisBestHighResIdx[0] * metersPerPixel;
            bestHighResXYT[1] =
                xyt1[1] + thisBestHighResIdx[1] * metersPerPixel;
            bestHighResXYT[2] = xyt1[2];
        }
        if (highResscores != NULL) {
            delete[] highResscores;
        }

        /*  if (thisBestHighResScore > thisBestLowResScore && debug) {
              // TODO: Investigate cases where this
              // happens. Numerical precision problems?
              System.out.printf("DEBUG: MultiResolutionScanMatcher %10d %10d
        %10d [%5d %5d %5d] [%5d %5d]\n", thisBestLowResScore,
        thisBestHighResScore, thisBestHighResScore - thisBestLowResScore,
                                bestLowResIdx[0], bestLowResIdx[1],
        bestLowResIdx[2], thisBestHighResIdx[0], thisBestHighResIdx[1]);
        }  */
    }

    if (vtlowResScores.size() > 0) {
        for (int i = 0; i < vtlowResScores.size(); i++) {
            IntArray2D *pArray = vtlowResScores.at(i);
            if (pArray != NULL) {
                delete pArray;
            }
        }
    }
}

void MultiResolutionScanMatcher::HistogramFilter_matchRaw(
    vector<Pose> &points, Pose &posepriorxyt, Pose &resPose,
    vector<double> &data_flag, int mode, vector<int> &id, int &navimode) {

    int decimate = 1;
    double metersPerPixel = dgm.metersPerPixel; // dgm����Ϣ������load map�����õ�
    double lowResXYT[3] = {0};
    double priorxyt[3] = {0};
    int pointsize = points.size();

    //��һ���ڣ������˵�λ��
    priorxyt[0] = posepriorxyt.x;
    priorxyt[1] = posepriorxyt.y;
    priorxyt[2] = posepriorxyt.theta;

    double HisFilter_thetaResolution = degrees_to_radians(1.0);
    double HisFilter_thetaRange = degrees_to_radians(15.0);

    //��������ʼλ��
    lowResXYT[0] = priorxyt[0] - 4 * metersPerPixel;
    lowResXYT[1] = priorxyt[1] - 4 * metersPerPixel;
    lowResXYT[2] = priorxyt[2] - HisFilter_thetaRange;

    //����������ÿһ��Ԫ�������������飬��һ�������ʾ�ڵ�ǰ�Ƕ��£�ÿ����Ϊ������ԭ��ʱ��ƥ���ϵĵ��ֵ�ĺ�
    //�ڶ��������ʾ�ڵ�ǰ�Ƕ��£�ÿ����Ϊ������ԭ��ʱ��ƥ���ϵĵ�ĸ���
    vector<IntArray2D *>
        vtlowResScores; // class IntArray2D: int *vs; //vs = new
                        // int[m_dim1*dim2];
                        //                   int *vd; //vd = new
                        //                   int[m_dim1*dim2]; int
                        //                   m_dim1,m_dim2;

    //��ǰһ���ڵĻ��������������ϣ���search_x_m��search_y_m,��0.2����չ��һ�����Σ����������ڵĵ㣬�ֱ���
    //�����ڵĵ㵱�������˵��������꣬��ɨ�赽�ĵ㣬��������ϵ����תƽ��ת�������ת����ĵ��Ƿ���Ѵ��ڵĵ�
    //ƥ���ϣ�ƥ�����ˣ��ͽ�vtlowResScores�е�vs�������Ӧλ�õ�ֵ����data�е�ֵ��vd�����е���Ӧλ��ֵ��1��
    //ÿһ���Ƕ��µ�ƥ�����ֱ�ѹջ����ʾ�ڴ˽Ƕ��£������ڸ�����ƥ��̶�
    dgm.HistogramFilter_scores3D(
        points, lowResXYT[0], lowResXYT[1], lowResXYT[2],
        HisFilter_thetaResolution,
        max(1, (int)(2 * HisFilter_thetaRange / HisFilter_thetaResolution)),
        vtlowResScores); // vtlowResScores������

    CMultiGaussionEstimator mge(3);
    /////////////////////////////////////////////////////////////////
    // Step 1. Compute the covariance by fitting a Gauassian to the
    // low-resolution samples.
    // MultiGaussianEstimator mge = new MultiGaussianEstimator(3);

    /////////////////////////////////////////////////////////////////
    // If no decimation has been requested, return a result now.
    if (1 >= decimate) {
        int bestidx0[3] = {0};
        double match_accuracy = 0.0, score_accuracy;
        int similar_num = 0;
        int bestscore = 0;
        int bestnum = 0;
        double vaildlaser_accuracy = 0.0;

        // ret = bestIndices(vtlowResScores,bestidx0);
        //��ƥ�������룬���м�⣬�����еĽǶ��µ����л�����λ���£������ƥ������
        // bestidx0    �洢ƥ���vsֵ���ĵ�����x���꣬���y���꣬��������������
        // bestscore   �洢ƥ���vs�����ֵ
        // similar_num �洢�����vsֵ����95%���ϵĵ�ĸ���
        // bestnum     �洢���vsֵ��λ�ˣ���Ӧ��ƥ���ϵļ����ĸ���
        HistogramFilterBestIndices(vtlowResScores, bestidx0, bestscore,
                                   similar_num, bestnum);

        match_accuracy =
            (double)bestnum /
            pointsize; //��ȷ�ȣ�λ��ƥ����õĵ㣬ƥ���ϵĸ��������ϼ�������Ч����
        vaildlaser_accuracy = (double)pointsize /
                              1081.0; //��Ч���⾫ȷ�ȣ���������Ч�����������ܸ���1081

        // double������
        data_flag.clear();
        data_flag.push_back((double)similar_num);
        // data_flag.push_back(vaildlaser_accuracy); //��Ч���⾫ȷ��
        data_flag.push_back(match_accuracy); //ƥ�侫ȷ��
        data_flag.push_back((double)pointsize);
        // data_flag.push_back((double)similar_num); //���ֵ����λ�˸���

        // double xyt0score =
        // vtlowResScores.at(bestidx0[2])->get(bestidx0[1],bestidx0[0]);//ȡbestscore

        //ƥ����õĻ�����λ��
        double u[3] = {0};
        u[0] =
            lowResXYT[0] +
            bestidx0[0] *
                metersPerPixel; //������դ�񳤶�Ϊ��λ���еĶ�λ�����ܻ���ƫ�������о���λ
        u[1] = lowResXYT[1] + bestidx0[1] * metersPerPixel;
        u[2] = lowResXYT[2] + bestidx0[2] * HisFilter_thetaResolution;

        id.push_back(bestidx0[0]);
        id.push_back(bestidx0[1]);

        if (false) {
            double xyt[3] = {0};

            for (int tidx = 0; tidx < vtlowResScores.size(); tidx++) {
                xyt[2] = lowResXYT[2] + tidx * HisFilter_thetaResolution;

                for (int yidx = 0; yidx < vtlowResScores.at(tidx)->m_dim1;
                     yidx++) {
                    xyt[1] = lowResXYT[1] + yidx * dgm.metersPerPixel;

                    for (int xidx = 0; xidx < vtlowResScores.at(tidx)->m_dim2;
                         xidx++) {

                        xyt[0] = lowResXYT[0] + xidx * dgm.metersPerPixel;
                        // int v = lowResScores[tidx].get(yidx, xidx);
                        int v = vtlowResScores.at(tidx)->get(yidx, xidx);
                        mge.observeWeighted(xyt, 3, MathUtil::exp(-256.0 + v));
                    }
                }
            }
        }

        //�����ˣ��ͷ�ƥ���λ�˽��
        if (0 < vtlowResScores.size()) {
            for (int i = 0; i < vtlowResScores.size(); i++) {
                IntArray2D *pArray = vtlowResScores.at(i);
                if (pArray != NULL) {
                    delete pArray;
                }
            }
        }

        Pose xytRes;

        //�Ի����˵�λ�˽��о���λ��֮ǰ����һ��դ��ľ���Ϊ��λ������������С���Դﵽ0.001�׾��Ƚ��ж�λ
        if (1 == m_ifrefine) {

            int rmode;
            if (match_accuracy >= 0.9) {
                rmode = 3;
            } else {

                if (match_accuracy >= 0.8) {
                    rmode = 2;
                } else {
                    rmode = mode;
                }
            }

            HistogramRefineFunc(points, u[0], u[1], u[2], posepriorxyt, NULL,
                                xytRes, rmode);
            resPose.x = xytRes.x;
            resPose.y = xytRes.y;

            resPose.theta = xytRes.theta;
        } else {
            resPose.x = u[0];
            resPose.y = u[1];

            resPose.theta = u[2];
        }

        navimode = m_ifrefine;

        return;
    }

    return;
}

double MultiResolutionScanMatcher::rand_back(double i, double j) {
    double u1;
    double u2;
    double r;
    u1 = double(rand() % 1000) / 1000;
    u2 = double(rand() % 1000) / 1000;
    static unsigned int seed = 0;
    r = i + sqrt(j) * sqrt(-2.0 * (log(u1) / log(exp(1.0)))) * cos(2 * PI * u2);
    return r;
}

void MultiResolutionScanMatcher::initWeight() {

    for (int i = 0; i < 81; i++) {
        VdHistogramFilterWeight[i] = (1.0 / 81.0);
    }
}
