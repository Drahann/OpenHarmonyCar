#include <signal.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <vector>

#include "laser_t.h"
#include "sl_lidar.h"
#include "sl_lidar_driver.h"
#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef _WIN32
#include <Windows.h>
#define delay(x) ::Sleep(x)
#else
#include <unistd.h>
// #include "sl_lidar_driver_impl.h"

#include "lcm.h"
lcm_t *lcm = NULL;

#define Circle_span_rad 300
#define LIDAR_SCAN_BUFFER_HINT 1024
#define LIDAR_MAX_SCAN_POINTS 8192
#define LIDAR_STATS_INTERVAL_SEC 1
#define LIDAR_STATS_INTERVAL_SCANS 10

static inline void delay(sl_word_size_t ms) {
    while (ms >= 1000) {
        usleep(1000 * 1000);
        ms -= 1000;
    };
    if (ms != 0)
        usleep(ms * 1000);
}
#endif

using namespace sl;

struct LidarLogStats {
    time_t windowStart;
    int publishedScans;
    int skippedScans;
    int grabFailures;
    int totalPoints;
    int validPoints;
    int minPoints;
    int maxPoints;
    float minDistanceMm;
    float maxDistanceMm;
    double sumDistanceMm;
    double sumRadstep;
};

static void lidarLog(const char *level, const char *fmt, ...) {
    FILE *stream = (strcmp(level, "ERROR") == 0 || strcmp(level, "WARN") == 0)
                       ? stderr
                       : stdout;
    fprintf(stream, "ts=%lld level=%s ", (long long)time(NULL), level);

    va_list args;
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);

    fprintf(stream, "\n");
    fflush(stream);
}

static void resetStats(LidarLogStats &stats) {
    stats.windowStart = time(NULL);
    stats.publishedScans = 0;
    stats.skippedScans = 0;
    stats.grabFailures = 0;
    stats.totalPoints = 0;
    stats.validPoints = 0;
    stats.minPoints = 0;
    stats.maxPoints = 0;
    stats.minDistanceMm = FLT_MAX;
    stats.maxDistanceMm = 0.0f;
    stats.sumDistanceMm = 0.0;
    stats.sumRadstep = 0.0;
}

static void addScanStats(LidarLogStats &stats, int points, int validPoints,
                         float minDistanceMm, float maxDistanceMm,
                         double sumDistanceMm, float radstep) {
    stats.publishedScans++;
    stats.totalPoints += points;
    stats.validPoints += validPoints;
    stats.sumDistanceMm += sumDistanceMm;
    stats.sumRadstep += radstep;

    if (stats.minPoints == 0 || points < stats.minPoints)
        stats.minPoints = points;
    if (points > stats.maxPoints)
        stats.maxPoints = points;

    if (validPoints > 0) {
        if (minDistanceMm < stats.minDistanceMm)
            stats.minDistanceMm = minDistanceMm;
        if (maxDistanceMm > stats.maxDistanceMm)
            stats.maxDistanceMm = maxDistanceMm;
    }
}

static void maybeLogStats(LidarLogStats &stats, bool force) {
    time_t now = time(NULL);
    int elapsed = (int)(now - stats.windowStart);
    bool shouldLog = force ||
                     elapsed >= LIDAR_STATS_INTERVAL_SEC ||
                     stats.publishedScans >= LIDAR_STATS_INTERVAL_SCANS;

    if (!shouldLog)
        return;

    if (stats.publishedScans == 0 && stats.skippedScans == 0 &&
        stats.grabFailures == 0) {
        return;
    }

    double avgPoints = stats.publishedScans > 0
                           ? (double)stats.totalPoints / stats.publishedScans
                           : 0.0;
    double avgRadstep = stats.publishedScans > 0
                            ? stats.sumRadstep / stats.publishedScans
                            : 0.0;
    double avgDistanceMm = stats.validPoints > 0
                               ? stats.sumDistanceMm / stats.validPoints
                               : 0.0;
    double hz = elapsed > 0 ? (double)stats.publishedScans / elapsed : 0.0;
    float minDistance = stats.minDistanceMm == FLT_MAX ? 0.0f : stats.minDistanceMm;

    lidarLog("INFO",
             "event=scan_stats scans=%d skipped=%d grab_failures=%d hz=%.2f "
             "points_avg=%.1f points_min=%d points_max=%d valid_points=%d "
             "dist_min_mm=%.1f dist_max_mm=%.1f dist_avg_mm=%.1f radstep_avg=%.6f",
             stats.publishedScans, stats.skippedScans, stats.grabFailures, hz,
             avgPoints, stats.minPoints, stats.maxPoints, stats.validPoints,
             minDistance, stats.maxDistanceMm, avgDistanceMm, avgRadstep);

    resetStats(stats);
}

static void resetCurrentScan(std::vector<float> &ranges,
                             std::vector<float> &intensities,
                             int &nranges, int &nintensities,
                             int &validPoints, float &minDistanceMm,
                             float &maxDistanceMm, double &sumDistanceMm) {
    nranges = 0;
    nintensities = 0;
    validPoints = 0;
    minDistanceMm = FLT_MAX;
    maxDistanceMm = 0.0f;
    sumDistanceMm = 0.0;
    ranges.clear();
    intensities.clear();
}

static void appendCurrentPoint(std::vector<float> &ranges,
                               std::vector<float> &intensities,
                               int &nranges, int &nintensities,
                               int &validPoints, float &minDistanceMm,
                               float &maxDistanceMm, double &sumDistanceMm,
                               float distanceMm, float angleDeg) {
    ranges.push_back(distanceMm);
    intensities.push_back(angleDeg);
    nranges++;
    nintensities++;

    if (distanceMm > 0.0f) {
        validPoints++;
        if (distanceMm < minDistanceMm)
            minDistanceMm = distanceMm;
        if (distanceMm > maxDistanceMm)
            maxDistanceMm = distanceMm;
        sumDistanceMm += distanceMm;
    }
}

/*
判断雷达健康状态，若健康则在控制台输出健康信息，返回true;否则输出错误信息并返回false
*/
bool checkSLAMTECLIDARHealth(ILidarDriver *drv) {
    sl_result op_result;
    sl_lidar_response_device_health_t healthinfo;

    op_result = drv->getHealth(healthinfo);
    if (SL_IS_OK(
            op_result)) { // the macro IS_OK is the preperred way to judge
                          // whether the operation is
                          // succeed.succeed.//宏IS_OK是判断操作是否成功的首选方法。
        lidarLog("INFO", "event=health status=%d", healthinfo.status);
        if (healthinfo.status == SL_LIDAR_STATUS_ERROR) {
            lidarLog("ERROR", "event=health_error reason=internal_error");
            // enable the following code if you want slamtec lidar to be reboot
            // by software drv->reset();
            return false;
        } else {
            return true;
        }

    } else {
        lidarLog("ERROR", "event=health_error result=0x%x", op_result);
        return false;
    }
}

bool ctrl_c_pressed;
void ctrlc(int) { ctrl_c_pressed = true; }

int main() {
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    const char *opt_channel_param_first = "/dev/ttyUSB0";
    sl_u32 opt_channel_param_second = 115200;
    sl_result op_result;

    IChannel *_channel; // 通信信道的抽象接口

    //////////////////////////////////
    // 时间
    time_t ntime;
    // 第一个激光点的角度
    float rad0 = 0;
    // 上一个激光点的角度，用于记录一圈数据中的最后一个激光点的角度
    float rad_end = 0;
    // 一圈数据中的激光点个数（距离）
    int nranges = 0;
    // 一圈数据中的激光点个数（激光强度）
    int nintensities = 0;
    // 一圈激光距离。SDK 一次缓存最多 8192 个点，不能用固定 1024 数组承接整圈数据。
    std::vector<float> ranges;
    // 一圈激光强度。修改：在导航算法中，这部分数据改为角度值。
    std::vector<float> intensities;
    // 角度歩幅
    // 标识符
    bool flag = false;

    laser_t Laser_data;
    LidarLogStats logStats;
    int scanValidPoints = 0;
    float scanMinDistanceMm = FLT_MAX;
    float scanMaxDistanceMm = 0.0f;
    double scanSumDistanceMm = 0.0;

    ranges.reserve(LIDAR_SCAN_BUFFER_HINT);
    intensities.reserve(LIDAR_SCAN_BUFFER_HINT);
    resetStats(logStats);
    resetCurrentScan(ranges, intensities, nranges, nintensities,
                     scanValidPoints, scanMinDistanceMm,
                     scanMaxDistanceMm, scanSumDistanceMm);
    /////////////////////////////////

    lidarLog("INFO", "event=start sdk_version=%s port=%s baud=%u",
             SL_LIDAR_SDK_VERSION, opt_channel_param_first,
             opt_channel_param_second);
    lidarLog("INFO", "event=scan_buffers_reset");

    lcm = lcm_create(NULL);
    if (lcm == NULL) {
        lidarLog("ERROR", "event=lcm_create_failed");
        return -1;
    }

    // create the driver instance
    // 创建驱动程序实例
    ILidarDriver *drv = *createLidarDriver();

    if (!drv) {
        lidarLog("ERROR", "event=create_driver_failed reason=insufficient_memory");
        exit(-2);
    }

    sl_lidar_response_device_info_t devinfo;
    bool connectSuccess = false;

    // serial情况下
    // 将端口和波特率传入
    _channel = (*createSerialPortChannel(opt_channel_param_first,
                                         opt_channel_param_second));
    if (SL_IS_OK((drv)->connect(_channel))) {
        op_result = drv->getDeviceInfo(devinfo);

        if (SL_IS_OK(op_result)) {
            connectSuccess = true;
        } else {
            delete drv;
            drv = NULL;
        }
    }

    if (!connectSuccess) {
        lidarLog("ERROR", "event=connect_failed port=%s",
                 opt_channel_param_first);
        goto on_finished;
    }

    // print out the device serial number, firmware and hardware version
    // number..
    char serialNumber[33];
    for (int pos = 0; pos < 16; ++pos) {
        snprintf(serialNumber + pos * 2, sizeof(serialNumber) - pos * 2,
                 "%02X", devinfo.serialnum[pos]);
    }

    lidarLog("INFO", "event=device_info serial=%s firmware=%d.%02d hardware=%d",
             serialNumber, devinfo.firmware_version >> 8,
             devinfo.firmware_version & 0xFF, (int)devinfo.hardware_version);

    // check health...
    if (!checkSLAMTECLIDARHealth(drv)) {
        goto on_finished;
    }

    signal(SIGINT, ctrlc);

    drv->setMotorSpeed();
    // start scan...
    op_result = drv->startScan(0, 1);
    if (SL_IS_FAIL(op_result)) {
        lidarLog("ERROR", "event=start_scan_failed result=0x%x", op_result);
        goto on_finished;
    }
    lidarLog("INFO", "event=scan_started");

    // fetech result and print it out...

    while (1) {
        sl_lidar_response_measurement_node_hq_t nodes[8192];
        size_t count = _countof(nodes);
        op_result = drv->grabScanDataHq(nodes, count);

        if (SL_IS_OK(op_result)) {
            drv->ascendScanData(nodes, count);
            ////////////////////////////////////////
            ntime = time(NULL);
            ////////////////////////////////////////
            for (int pos = 0; pos < (int)count; ++pos) {
                // //////////////////////////////////
                // 第一个激光点需要作为第一圈数据的rad0
                if (!flag) {
                    flag = true;
                    rad0 = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
                }
                float temprad = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
                float distanceMm = nodes[pos].dist_mm_q2 / 4.0f;
                // 当前后两次激光数据的角度值之差大于Circle_span_rad，说明雷达已经扫完一圈数据，
                // 将记录的一圈数据存放到Laser_data中并发布出去，然后初始化记录激光数据的各种变量
                if (rad_end - temprad > Circle_span_rad) {
                    if (nranges > 1 && nintensities == nranges) {
                        float scanRadstep = (rad_end - rad0) / (nranges - 1);
                        Laser_data.nranges = nranges;
                        Laser_data.nintensities = nintensities;
                        Laser_data.utime = ntime;
                        Laser_data.rad0 = rad0;
                        Laser_data.radstep = scanRadstep;
                        Laser_data.ranges = ranges.data();
                        Laser_data.intensities = intensities.data();
                        laser_t_publish(lcm, "HOKUYO_LIDAR", &Laser_data);
                        addScanStats(logStats, nranges, scanValidPoints,
                                     scanMinDistanceMm, scanMaxDistanceMm,
                                     scanSumDistanceMm, scanRadstep);
                        maybeLogStats(logStats, false);
                    } else {
                        logStats.skippedScans++;
                        lidarLog("WARN",
                                 "event=skip_invalid_scan nranges=%d nintensities=%d",
                                 nranges, nintensities);
                        maybeLogStats(logStats, false);
                    }
                    ntime = time(NULL);
                    rad0 = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
                    resetCurrentScan(ranges, intensities, nranges, nintensities,
                                     scanValidPoints, scanMinDistanceMm,
                                     scanMaxDistanceMm, scanSumDistanceMm);
                }

                if (nranges >= LIDAR_MAX_SCAN_POINTS) {
                    logStats.skippedScans++;
                    lidarLog("WARN",
                             "event=drop_oversized_scan points=%d angle_deg=%.2f",
                             nranges, temprad);
                    rad0 = temprad;
                    resetCurrentScan(ranges, intensities, nranges, nintensities,
                                     scanValidPoints, scanMinDistanceMm,
                                     scanMaxDistanceMm, scanSumDistanceMm);
                    maybeLogStats(logStats, false);
                }

                rad_end = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
                // 注意，这里的激光强度值在导航算法中被改为角度值
                appendCurrentPoint(ranges, intensities, nranges, nintensities,
                                   scanValidPoints, scanMinDistanceMm,
                                   scanMaxDistanceMm, scanSumDistanceMm,
                                   distanceMm, temprad);
                // intensities[nintensities] =
                //     nodes[pos].quality >>
                //     SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT;
                // ranges.push_back(nodes[pos].dist_mm_q2/4.0f);
                // intensities.push_back(nodes[pos].quality >>
                // SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
                // //////////////////////////////////
            }
        } else {
            logStats.grabFailures++;
            if (logStats.grabFailures <= 3 ||
                logStats.grabFailures % 10 == 0) {
                lidarLog("WARN", "event=grab_scan_failed result=0x%x count=%d",
                         op_result, logStats.grabFailures);
            }
            maybeLogStats(logStats, false);
            delay(10);
        }

        if (ctrl_c_pressed) {
            lidarLog("INFO", "event=stop_requested");
            break;
        }
    }

    maybeLogStats(logStats, true);

    drv->stop();
    delay(200);
    drv->setMotorSpeed(0);
    lidarLog("INFO", "event=stopped");
    // done!
on_finished:
    if (drv) {
        delete drv;
        drv = NULL;
    }
    if (lcm) {
        lcm_destroy(lcm);
        lcm = NULL;
    }
    return 0;
}
