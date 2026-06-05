
#include "ScanMatcher.h"
#include "../gaussestimator/CMultiGauss.h"
#include "../gaussestimator/MultiGaussionEstimator.h"
#include "../math/MathUtil.h"
#include "../zlib.h"
#include <fstream>

extern lcm_t *lcm;

ScanMatcher::ScanMatcher(void) {
    gmDirty = false;

    metersPerPixel = 0.05;
    useOdometry = true;
    rangeCovariance = 0.1;
    rangeCovariance = 0.0035;
    maxScanHistory = 5;
    decimate = 1; // throw away all but every Nth scan. (1 = keep all).

    search_x_m = 0.2;
    search_y_m = 0.2;
    search_theta_rad =
        MathUtil::toRadians(15); ////////////////////////////////////// 15
    search_theta_res_rad = MathUtil::toRadians(1);

    pose_dist_thresh_m = 0.35;
    pose_theta_thresh_rad = MathUtil::toRadians(30);

    gridmap_size = 50;

    old_scan_decay = 5; // in units of gray-scale values per age of scan kept.

    decimateCounter = 0;

    m_IfVaild_Encoder = false;
    // ArrayList<Scan> scans = new ArrayList<Scan>();
    //  where do we think the robot is now (in global coordinates)?
    //  matcher = new MultiResolutionScanMatcher(config);

    gm.makeMeters(-25, -25, gridmap_size, gridmap_size, metersPerPixel, 0);

    submapscore.clear();

    pthread_mutex_init(&m_csPose_mutex, NULL);
}

ScanMatcher::~ScanMatcher(void) { pthread_mutex_destroy(&m_csPose_mutex); }

//ŽËº¯ÊýÒÀÍÐÓÚcreatMapÖÐµÄm_pscanMatcher
void ScanMatcher::processScan(
    vector<Pose> &doublelaserpoints, vector<Pose> &rpoints,
    vector<double>
        &flag) // rpointsÊÇvector<pose> bodyPoints,»úÆ÷ÈË×ø±êÏµÖÐµÄ×ø±ê
{
    Pose xytCur;

    pthread_mutex_lock(&m_csPose_mutex);
    xytCur = xyt; //»úÆ÷ÈËÔÚÊÀœç×ø±êÏµÖÐµÄÎ»×Ë£¬xyt=0,0,0  µÚÒ»ŽÎ

    // NaviInterface ÀïµÄRevisePoseº¯Êý
    if (m_IfVaild_Encoder) //Èç¹ûÐèÒªŸÀÆ«
    {
        xyt = Encoderpos; //ÂëÅÌÖµž³Öµl
        m_IfVaild_Encoder = false;
    }
    pthread_mutex_unlock(&m_csPose_mutex);

    if (scans.size() == 0) // g,scansÊÇ¿ÉÒÔ±£ÁôÏÂÀŽµÄ
    {
        // our first-ever scan.
        GXYTNode gn;       // GNode gn: pose state; pose init; pose truth
                           // Attributes attributes; //class Attributes:
                           // map<string,vector<pose>> attrs;ÍŒ
        gn.state = xytCur; // xytCur=0,0,0
        gn.init = xytCur;
        gn.truth = xytCur;

        /**********************/
        /**********************/

        //Ã¿žöGNode¶ŒÓÐÒ»žöÍŒ£¬key=points£¬ÄÚÈÝÊÇµ±Ç°»úÆ÷ÈËÎ»×ËÏÂ£¬Œ€¹âµãµÄ»úÆ÷ÈË×ø±êÏµ×ø±ê
        gn.setAttribute("points", rpoints); // map<string,vector<pose>> attrs;
                                            // setAttribute(string key,
                                            // vector<pose> &o)=> attrs[key]=o;
        //°ÑÆðÊŒµãŽæŽ¢£¬»¹ÓÐÔÚµ±Ç°»úÆ÷ÈËÎ»×ËÏÂµÄŒ€¹âµã×ø±ê
        // gÊÇÒªŽ«×ßµÄ£¬Ž«žødoSLAMÓÃ
        g.nodes.push_back(gn); // Graph g;
                               // class Graph:vector<GNode> nodes;

        submapscore.push_back(0);

        Scan scan; // class Scan: pose xyt; »úÆ÷ÈËÎ»×Ë
                   //             vector<pose> gpoints; Œ€¹âµãÊÀœç×ø±ê
                   //             vector<vector<vector<double>>>  gcontours
        scan.xyt = xytCur;

        // change to world
        LinAlg::transform(
            xytCur, rpoints,
            scan.gpoints); //œ«»úÆ÷ÈË×ø±êÏµÖÐµÄŒ€¹âµãÎ»ÖÃrpoints,Íš¹ý»úÆ÷ÈËÎ»×ËxytCur,×ª»¯ÎªÊÀœç×ø±êÏµÖÐµÄÎ»ÖÃ

        //ÔÚÕâÒ»Ö¡µÄËùÓÐµãÖÐ£¬ŽæŽ¢·ûºÏŸàÀëÒªÇóµÄÏàÁÚµãŒ¯vector<vector<double>>,ºÍÀëÉ¢µã
        contourExtractor.getContours(scan.gpoints, scan.gcontours);
        scans.push_back(scan); //ŽæŽ¢µÚÒ»Ö¡ÐÅÏ¢

        vector<vector<Pose>> test_rpoints;
        contourExtractor.getContours_Pose(rpoints, test_rpoints);

        for (int i = 0; i < test_rpoints.size(); i++) {
            vector<Pose> contour_points = test_rpoints.at(i);
            if (contour_points.size() <= 1) {
                continue;
            }

            //婵?鍏夌偣杞笘鐣屽潗鏍?
            //鐢荤嚎锛岄棿闅斿ぇ浜?0.3灏辨柇寮?锛屽惁鍒欒繛绾匡紝缁嗙嚎
            addProbMap(pm, gn.state, contour_points, 5);
        }

        //ÀûÓÃscanµÄÐÅÏ¢£¬»­ÍŒ£¬ŒŽÌî³ädata£¬Ã¿ŽÎ¶ŒÖØÐÂ»­£¬ŒŽÃ¿ŽÎ¶ŒÊÇ×îÐÂµÄÂÖÀªÐÅÏ¢
        //»­Ö®Ç°£¬¶ŒÒªrecenterÒ»ÏÂ£¬Ÿ¡¿ÉÄÜ¶àµÄ±£ÁôÂÖÀªÐÅÏ¢
        // gmµÄŽóÐ¡£¬¿ÉÒÔÔÚ¹¹Ôìº¯ÊýÀï¿Žµœ£¬-25£¬-25£¬50£¬50£¬ÓÐžömakemeters
        drawScan(scan); // gmÀïµÄdata¿ÉÒÔ±£ÁôÏÂÀŽ£¬±£ÁôµÄÊÇ×îÐÂµÄÂÖÀª
        glevel.push_back(5);
        return;
    }

    decimateCounter++;
    if (decimateCounter !=
        decimate) // decimate=1,³éÈ¡±ÈÀý£¬1ÊÇÈ«²¿¶ŒÒª£¬2ÊÇÁœžöÈ¡Ò»žö£¬10ÊÇÊ®žöÈ¡Ò»žö£¬ÀàÍÆ
        return;
    decimateCounter = 0;
    // expend map?
    bool bexpand = expendmap(xytCur);

    // gauss map
    GridMap GaussianMap;
    int ix = (int)((xytCur.x - 25 - pm.x0) / 0.05);
    int iy = (int)((xytCur.y - 25 - pm.y0) / 0.05);
    double x0 = pm.x0 + ix * 0.05;
    double y0 = pm.y0 + iy * 0.05;
    LUT lut;
    GaussianMap.makePixels(x0, y0, 1000, 1000, 0.05, (BYTE)0, false);
    GaussianMap.makeGaussianLUT(1.0, 0, 1.0 / LinAlg::sq(0.06), lut);

    for (int i = 0; i < GaussianMap.width; i++) {
        for (int j = 0; j < GaussianMap.height; j++) {
            if (pm.data[(j + iy) * pm.width + i + ix] >
                0.5) { //仔细看drawDot函数，在画图时，用于比较的点是栅格中点，所以此处传中点，会提高高斯地图的精度
                //如果不传中心点，会使生成的高斯地图，不产生255的点，且会连续出现相邻的127的点，且高斯距离减小，因为127是高斯表的第2个点
                GaussianMap.drawDot(
                    (i + 0.5) * pm.metersPerPixel +
                        x0 //这里给drawDot传的(x0,y0)，正好处于像素点的左下角，可以修改为正中间
                    ,
                    (j + 0.5) * pm.metersPerPixel + y0, lut,
                    lut.length); //如果修改为正中间，理论上，生成的高斯地图在一定程度上会提高定位精确度
            }
        }
    }
    pmatcher.setModel(GaussianMap);

    // map match
    Pose res;
    pmatcher.matchRaw(rpoints, xytCur, NULL, search_x_m, search_y_m,
                      search_theta_rad, search_theta_res_rad, res, flag, 0);

    double a2 = flag.at(1);

    // printf("score = %f\n",a2);
    if (0) {
        matcher.matchRaw(rpoints, xytCur, NULL, search_x_m, search_y_m,
                         search_theta_rad, search_theta_res_rad, res, flag, 0);
    }
    // res.theta = Simu_normalize_theta(res.theta);//角度标准化，范围为[-pi,pi)
    xytCur = res; //µ±Ç°Î»ÖÃ

    pthread_mutex_lock(&m_csPose_mutex);
    xyt = xytCur;
    pthread_mutex_unlock(&m_csPose_mutex);

    // where was our last scan?
    Pose lastxyt = scans.at(scans.size() - 1).xyt; //ÉÏÒ»žöÂÖÀªÖÐ»úÆ÷ÈËµÄÎ»×Ë
    // double ddist = LinAlg.distance(xyt, lastxyt, 2);
    double ddist =
        LinAlg::DistancePose(xytCur, lastxyt); //ÕâŽÎÓëÉÏŽÎ»úÆ÷ÈËÎ»ÖÃŸàÀë
    double dtheta = fabs(MathUtil::mod2pi(
        xytCur.theta - lastxyt.theta)); //ÕâŽÎÓëÉÏŽÎ£¬»úÆ÷ÈË×ªœÇ²îÖµ

    // pose_dist_thresh_m = 0.4; pose_theta_thresh_rad =
    // MathUtil::toRadians(45); ²»Ò»¶šÃ¿žöÖÜÆÚ¶Œ»áÌáÈ¡
    if (ddist >= pose_dist_thresh_m ||
        dtheta >= pose_theta_thresh_rad) //Èç¹ûÓÐÒ»ÏîŽóÓÚÉè¶šãÐÖµ£¬ŸÍ¿ªÊŒÌáÈ¡
    {

        GXYTNode gn;
        gn.state = xytCur;
        gn.init = xytCur;
        gn.truth = xytCur;

        gn.setAttribute("points", rpoints);
        g.nodes.push_back(
            gn); //Ñ¹Õ»£¬±£Žæ»úÆ÷ÈËµ±Ç°Î»×ËºÍ»úÆ÷ÈË×ø±êÏÂµÄŒ€¹âµã×ø±ê
        if (a2 >= 0.9) {
            glevel.push_back(3);
        } else {
            if (a2 >= 0.8) {
                glevel.push_back(4);
            } else {
                glevel.push_back(5);
            }
        }

        submapscore.push_back(flag.at(2));

        Scan scan;
        scan.xyt = xytCur;
        LinAlg::transform(xytCur, rpoints, scan.gpoints);
        contourExtractor.getContours(scan.gpoints, scan.gcontours);
        scans.push_back(scan);

        //Ö»±£Áô×îÐÂµÄ5ŽÎÉšÃèµÄÂÖÀª
        if (scans.size() > maxScanHistory) // maxScanHistory=5
        {
            vector<Scan>::iterator iter = scans.begin();
            scans.erase(iter);
        }

        // drawScan(scan);

        vector<vector<Pose>> test_rpoints;
        contourExtractor.getContours_Pose(rpoints, test_rpoints);

        for (int i = 0; i < test_rpoints.size(); i++) {
            vector<Pose> contour_points = test_rpoints.at(i);
            if (contour_points.size() <= 1) {
                continue;
            }

            if (a2 >= 0.9) {
                addProbMap(pm, gn.state, contour_points, 3);
            } else {
                if (a2 >= 0.8) {
                    addProbMap(pm, gn.state, contour_points, 4);
                } else {
                    addProbMap(pm, gn.state, contour_points, 5);
                }
            }
        }

        // data transmission
        // int subedge = 15;
        // int subw = subedge / 0.05;

        // int subx = (xytCur.x - subedge / 2) / 0.05;
        // int suby = (xytCur.y - subedge / 2) / 0.05;

        // int biasx = (xytCur.x - subedge / 2 - pm.x0) / 0.05;
        // int biasy = (xytCur.y - subedge / 2 - pm.y0) / 0.05;

        // int8_t iparams[subw * subw];

        // int k = 0;
        // for (int j = 0; j < subw; j++) {
        //     for (int i = 0; i < subw; i++) {
        //         if (pm.data[(j + biasy) * pm.width + i + biasx] > 0.5) {
        //             iparams[k] = 1;
        //         } else {
        //             iparams[k] = 0;
        //         }

        //         k++;
        //     }
        // }

        /**********************************************************************************************/
        // char src2[sizeof(iparams) / sizeof(iparams[0])];
        // // char src2[90000];
        // char tmp[10];
        // // memset(src2, 0, sizeof(iparams) / sizeof(iparams[0]));
        // for (int i = 0; i < sizeof(src2); i++) {
        //     src2[i] = 0;
        // }
        // // src2[0] = '\0';
        // for (int i = 0; i < (sizeof(src2)); i++) {
        //     sprintf(tmp, "%d", iparams[i]);
        //     strcat(src2, tmp);
        // }

        // char *src;
        // src = src2;

        // int size_src = strlen(src);
        // char *compressed = NULL;
        // compressed = (char *)malloc(size_src * 2);
        // memset(compressed, 0, size_src * 2);
        // int gzSize = gzCompress(src, size_src, compressed, size_src * 2);
        // if (gzSize <= 0) {
        //     printf("compress error.\n");
        // }
        // grid_map_t grid_data;
        // grid_data.utime = 0;
        // grid_data.encoding = 0;
        // grid_data.x0 = (double)subx;
        // grid_data.y0 = (double)suby;
        // grid_data.meters_per_pixel = 0.05;
        // grid_data.width = subw;
        // grid_data.height = subw;
        // grid_data.src = compressed;
        // grid_data.datalen = gzSize;
        // grid_data.dst = (uint8_t *)compressed;

        // grid_map_t_publish(lcm, "MAPINFO", &grid_data);
    }
}

void ScanMatcher::drawScan(
    Scan &s) // class Scan: pose xyt;
             //             vector<pose> gpoints;   ÊÀœç×ø±êÏµÖÐµÄÎ»ÖÃ
             //             vector<vector<vector<double>>>  gcontours
{
    double minx = Double_MAX_VALUE,
           maxx = -Double_MAX_VALUE; // 999999999999.9999 / -999999999999.9999
    double miny = Double_MAX_VALUE, maxy = -Double_MAX_VALUE;

    // Compute bounds of the scans.
    for (int i = 0; i < s.gpoints.size(); i++) //ÊÀœç×ø±êÏµÖÐµãÑ­»·Ò»±é
    {
        Pose p =
            s.gpoints.at(i); //ÕÒµœÕâÒ»Ö¡µÄËùÓÐµãÖÐ£¬x£¬y×ø±êÖµµÄ×îŽóºÍ×îÐ¡Öµ
        minx = min(minx, p.x);
        maxx = max(maxx, p.x);
        miny = min(miny, p.y);
        maxy = max(maxy, p.y);
    }

    //°ÑÕâÒ»Ö¡ÖÐ×ø±êÖÐµã×÷ÎªµØÍŒÖÐÐÄµã
    //ÕâÀïµÄgm£¬ÊÇScanMatcher.hÀï¶šÒåµÄ£¬Óë×îºóµÄsetModelº¯ÊýÖÐµÄgm²»ÊÇÒ»»ØÊÂ
    gm.recenter((minx + maxx) / 2, (miny + maxy) / 2,
                5); // RridMap gm£ºdouble x0, y0;
                    //             double metersPerPixel;
                    //             int    width, height;
                    //             BYTE   *data;
                    //             BYTE   defaultFill;
    //Ïàµ±ÓÚÈ«²¿ÖÃÁã
    //ÖØÐÂ»­ÍŒ£¬žøÏÂŽÎµÄmatcher.matchRawÓÃ
    gm.subtract(old_scan_decay); // old_scan_decay=5

    LUT lut; // class LUT£ºdouble metersPerPixel;
             //            vector<int> vtlut;
             //            int length;

    //×öÒ»žöžßË¹²éÕÒ±í£¬±ÈÀýÎª1£¬·åÖµµãX×ø±êÎª0£¬rangeCovarianceÊÇsigma^2£¬œ«ÀëÉ¢µÄžßË¹Öµ°ŽË³ÐòŽæŽ¢µœlutÖÐ
    gm.makeGaussianLUT(1.0, 0, 1.0 / rangeCovariance,
                       lut); // rangeCovariance=0.0035

    for (int k = 0; k < s.gcontours.size(); k++) //ÂÖÀªžöÊý
    {
        vector<vector<double>> c =
            s.gcontours.at(k); //°ÑgcontoursÀïÃ¿žöÏàÁÚµãŒ¯ºÏÈ¡³öÀŽ
        for (int i = 0; i + 1 < c.size();
             i++) //Ã¿Ò»žöµãŒ¯ÀïÑ­»·£¬Èç¹ûÊÇµ¥žö¹ÂÁ¢µã£¬ŸÍÅ×Æú
        {
            vector<double> p0 = c.at(i);
            vector<double> p1 = c.at(i + 1);

            // double length = LinAlg.distance(p0, p1);

            double length = sqrt((p0.at(0) - p1.at(0)) * (p0.at(0) - p1.at(0)) +
                                 (p0.at(1) - p1.at(1)) *
                                     (p0.at(1) - p1.at(1))); //ÏàÁÚµãŒäµÄŸàÀë

            //»­ŸØÐÎ£¬Ìî³ädata
            gm.drawRectangle((p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2, length,
                             0, //»­ŸØÐÎ£¬Ž«²Î²»Ò»Ñù£¬ÕâÀïÊÇ»­Ïß¹ŠÄÜ
                             atan2(p1[1] - p0[1], p1[0] - p0[0]), lut,
                             lut.length);
        }
    }

    //œ«gmŽ«žø³ÉÔ±MultiResolutionScanMatcher matcherÖÐµÄdgm
    //ÔÚºóÃæœøÐÐmatcher.matchRawÆ¥ÅäŒì²âÊ±£¬»áÓÃµœ
    // gmÀï°üº¬ÁËŽË×ÓµØÍŒµÄËùÓÐÐÅÏ¢
    matcher.setModel(
        gm); // MultiResolutionScanMatcher matcher
             //ŽËº¯Êý¶šÒåÔÚMultiResolutionScanMatcher.cppÖÐ
             //ÊÇÁíÒ»žöº¯Êý£¬ÀïÃæµÄ±äÁ¿ÊôÓÚÁíÒ»žöÀàÖÐ£¬Óëµ±Ç°ŽúÂëÎ»ÖÃÖÐµÄ±äÁ¿ÎÞ¹Ø
             //ËäÈ»ÓÐ±äÁ¿Ãû×ÖÒ»Ñù£¬²»ÒªÅª»ì£¬ÅªÇåº¯Êý¹ŠÄÜ£¬Œ°ž÷žö±äÁ¿µÄËùÊôÀà
             //ŽËÀàÎÊÌâÔÚÒÔºó¿ŽŽúÂëÖÐÒªÖØÊÓ£¬±ØÒªÊ±ÒªÔÚ³ÌÐòÖÐ×¢Ã÷
}

bool ScanMatcher::addProbMap(ProbMap &probMap, Pose pose, vector<Pose> &points,
                             int level) {
    Pose lastp;

    lastp.x = 0;
    lastp.y = 0;
    lastp.theta = 0;
    double a = (double)level * 0.4;
    double b = (double)1 / a;

    for (int i = 0; i < points.size(); i++) {
        Pose p = points.at(i); //挨个把激光点的机器人坐标系坐标取出来

        p = LinAlg::transform(pose, p); //通过机器人位姿，转换到世界坐标系坐标

        if (LinAlg::DistancePose(p, lastp) > 0.15) //与上一个点距离大于0.3
        {
            probMap.setValue(
                p.x, p.y,
                a); //如果p点没越界，就在gridMap中的data向量的相应点置位255/有符号数-1
            // if(gridMap.getValue(p[0], p[1]) != 255) System.out.println("point
            // value set failure");
        } else //如果不大于0.3
        {
            probMap.drawLine(
                p.x, p.y, lastp.x, lastp.y,
                a); //在两点间画线，填充data，可以理解为画细线，与画矩形那个不一样
        }
        probMap.drawLine(p.x, p.y, pose.x, pose.y, b);
        probMap.setValue(p.x, p.y, a);

        lastp = p;
    }

    return true;
}

void ScanMatcher::initpfswarm() {

    pfswarm.clear();

    particles pf;
    Pose pose;
    pose.x = 0.0;
    pose.y = 0.0;
    pose.theta = 0.0;
    pf.pose = pose;
    pf.weight = 0.0;
    pf.tempweight = 0.0;
    pf.prop = 0.0;
    for (int i = 0; i < 800; i++) {
        pfswarm.push_back(pf);
    }
}

void ScanMatcher::initprobmap() {
    pm.makePixels(-50, -50, 2000, 2000, 0.05, 0.5, false); // 50*50,0.05
    glevel.clear();
    return;
}

bool ScanMatcher::expendmap(Pose &pose) {
    double range = 30;
    double width = pm.width * pm.metersPerPixel; //计算现有地图的长宽
    double height = pm.height * pm.metersPerPixel;
    bool expandX = false, expandY = false; //是否需要扩展
    double x0 = pm.x0;                     //现有地图的左下角坐标
    double y0 = pm.y0;

    //判断是否需要进行地图扩展
    if (pose.x - pm.x0 >
        width - range) { //机器人向X正方向走，离X正方向边界小于30
        width = width + range;
        expandX = true;
    }

    if (pose.y - pm.y0 >
        height - range) { //机器人向Y正方向走，离Y正方向边界小于range
        height = height + range;
        expandY = true;
    }

    if (pose.x < pm.x0 + range) //机器人向X负方向走，离X负方向边界小于range
    {

        x0 -= range;
        width = width + range;
        expandX = true;
    }

    if (pose.y < pm.y0 + range) { //机器人向Y负方向走，离Y负方向边界小于range

        y0 -= range;
        height = height + range;
        expandY = true;
    }

    //需要扩展
    if (expandX || expandY) {
        int wd = (int)(width / 0.05);
        int hi = (int)(height / 0.05);
        pm.makePixels(x0, y0, wd, hi, 0.05, 0.5, false);

        int N = g.nodes.size();
        for (int i = 0; i < N; i++) {
            int level = glevel.at(i);

            GNode gn = g.nodes.at(i); //鎶奼.node鍚戦噺涓殑GNode鎸ㄤ釜鍙栧嚭鏉?
            vector<Pose> tmp_p;

            gn.getAttribute(
                "points",
                tmp_p); //鎶婂綋鍓岹Node涓満鍣ㄤ汉浣嶅Э涓嬬殑婵?鍏夌偣鐨勬満鍣ㄤ汉鍧愭爣绯诲潗鏍囧彇鍑烘潵

            vector<vector<Pose>> test_rpoints;
            contourExtractor.getContours_Pose(tmp_p, test_rpoints);

            for (int i = 0; i < test_rpoints.size(); i++) {
                vector<Pose> contour_points = test_rpoints.at(i);
                if (contour_points.size() <= 1) {
                    continue;
                }

                //婵?鍏夌偣杞笘鐣屽潗鏍?
                //鐢荤嚎锛岄棿闅斿ぇ浜?0.3灏辨柇寮?锛屽惁鍒欒繛绾匡紝缁嗙嚎
                addProbMap(pm, gn.state, contour_points, level);
            }
        }
        return true;
    }
    return false;
}
int gzCompress(const char *src, int srcLen, char *dest, int destLen) {
    z_stream c_stream;
    int err = 0;
    int windowBits = 15;
    int GZIP_ENCODING = 16;

    if (src && srcLen > 0) {
        c_stream.zalloc = (alloc_func)0;
        c_stream.zfree = (free_func)0;
        c_stream.opaque = (voidpf)0;
        if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                         windowBits | GZIP_ENCODING, 8,
                         Z_DEFAULT_STRATEGY) != Z_OK)
            return -1;
        c_stream.next_in = (Bytef *)src;
        c_stream.avail_in = srcLen;
        c_stream.next_out = (Bytef *)dest;
        c_stream.avail_out = destLen;
        while (c_stream.avail_in != 0 && c_stream.total_out < destLen) {
            if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK)
                return -1;
        }
        if (c_stream.avail_in != 0)
            return c_stream.avail_in;
        for (;;) {
            if ((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END)
                break;
            if (err != Z_OK)
                return -1;
        }
        if (deflateEnd(&c_stream) != Z_OK)
            return -1;
        return c_stream.total_out;
    }
    return -1;
}
