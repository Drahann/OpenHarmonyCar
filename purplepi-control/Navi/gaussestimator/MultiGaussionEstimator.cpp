
#include "MultiGaussionEstimator.h"
#include "../math/LinAlg.h"

CMultiGaussionEstimator::CMultiGaussionEstimator(int n) : m_obsCount(0) {
    int i = 0;

    int len = sizeof(double) * n;
    m_num = n;
    m_pP = new double *[n];
    for (i = 0; i < n; i++) {
        m_pP[i] = new double[n];
        memset(m_pP[i], 0, len);
    }
    m_pU = new double[n];
    memset(m_pU, 0, len);
}

CMultiGaussionEstimator::~CMultiGaussionEstimator() {
    int i = 0;

    delete[] m_pU;
    for (i = 0; i < m_num; i++) {
        delete[] m_pP[i];
        m_pP[i] = 0;
    }
    delete[] m_pP;
    m_pP = NULL;
}

/*<FUNC+>*******************************************************
 * ��������: observe
 * ��������: �۲�
 * �������: double* pV ��int len��������ָ��ͳ���
 *
 * �������:
 *
 * �� �� ֵ:
 * ��������:
 * ����˵��: ��
 * �޸ļ�¼:
 * -------------------------------------------------------------
 *    2014/10/27        1.0           ����          ��������
 *<FUNC->*******************************************************
 */
void CMultiGaussionEstimator::observe(double *pV, int len) {
    observeWeighted(pV, len, 1.0);
}

/*<FUNC+>*******************************************************
 * ��������: observeWeighted
 * ��������: For estimating the covariance by integrating numerically.
 *			Note: PROB only need be accurate up to a constant
 *			multiplicative factor. You cannot use an unbiased
 *estimate �������:
 *
 * �������:
 *
 * �� �� ֵ:
 * ��������:
 * ����˵��: ��
 * �޸ļ�¼:
 * -------------------------------------------------------------
 *    2014/10/27        1.0           ����          ��������
 *<FUNC->*******************************************************
 */
void CMultiGaussionEstimator::observeWeighted(double *v, int len, double prob) {
    // assert( len==m_num );

    int i = 0;
    int j = 0;
    for (i = 0; i < m_num; i++) {
        for (j = 0; j < m_num; j++) {
            m_pP[i][j] += v[i] * v[j] * prob;
        }
    }

    for (i = 0; i < m_num; i++) {
        m_pU[i] += v[i] * prob;
    }

    m_obsCount += prob;
}

/*<FUNC+>*******************************************************
 * ��������: getEstimate
 * ��������: ��ȡ��˹���Ƶ�ָ��
 * �������:
 *
 * �������:
 *
 * �� �� ֵ:
 * ��������:
 * ����˵��: ��
 * �޸ļ�¼:
 * -------------------------------------------------------------
 *    2014/11/13        1.0           ����          ��������
 *<FUNC->*******************************************************
 */
MultiGaussian *
CMultiGaussionEstimator::getEstimate(/*MultiGaussian* pG,*/ bool unbiased) {
    double normalization;

    if (unbiased) {
        normalization = 1.0 / (m_obsCount - 1);
    } else {
        normalization = 1.0 / m_obsCount;
    }

    double *tu = new double[m_num];
    memset(tu, 0, sizeof(double) * m_num);
    LinAlg::scale(m_pU, m_num, tu, normalization);

    CMatrix *tP = new CMatrix(m_pP, m_num, m_num);
    tP->times(normalization, tP);

    for (int i = 0; i < m_num; i++) {
        for (int j = 0; j < m_num; j++) {
            tP->plusEquals(i, j, -tu[i] * tu[j]);
        }
    }
    MultiGaussian *pG = new MultiGaussian(tP, tu, m_num);
    delete[] tu;
    tu = NULL;
    delete tP;
    tP = NULL;

    return pG;
}
