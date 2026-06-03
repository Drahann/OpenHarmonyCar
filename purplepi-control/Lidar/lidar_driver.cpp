#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include <vector>

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
lcm_t *lcm = lcm_create(NULL);

#define Circle_span_rad 300

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
        printf("SLAMTEC Lidar health status : %d\n", healthinfo.status);
        if (healthinfo.status == SL_LIDAR_STATUS_ERROR) {
            fprintf(stderr, "Error, slamtec lidar internal error detected. "
                            "Please reboot the device to retry.\n");
            // enable the following code if you want slamtec lidar to be reboot
            // by software drv->reset();
            return false;
        } else {
            return true;
        }

    } else {
        fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n",
                op_result);
        return false;
    }
}

bool ctrl_c_pressed;
void ctrlc(int) { ctrl_c_pressed = true; }

int main() {
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
    // 一圈激光距离
    float ranges[1024];
    // 一圈激光强度
    // 修改：在导航算法中，这部分数据改为角度值
    float intensities[1024];
    // std::vector<float> ranges;
    // std::vector<float> intensities;
    // 角度歩幅
    float radstep = 0;
    // 标识符
    bool flag = false;

    laser_t Laser_data;
    /////////////////////////////////

    printf("Ultra simple LIDAR data grabber for SLAMTEC LIDAR.\n"
           "Version: %s\n",
           SL_LIDAR_SDK_VERSION);

    // create the driver instance
    // 创建驱动程序实例
    ILidarDriver *drv = *createLidarDriver();

    if (!drv) {
        fprintf(stderr, "insufficent memory, exit\n");
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
        fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n",
                opt_channel_param_first);
        goto on_finished;
    }

    // print out the device serial number, firmware and hardware version
    // number..
    printf("SLAMTEC LIDAR S/N: ");
    for (int pos = 0; pos < 16; ++pos) {
        printf("%02X", devinfo.serialnum[pos]);
    }

    printf("\n"
           "Firmware Ver: %d.%02d\n"
           "Hardware Rev: %d\n",
           devinfo.firmware_version >> 8, devinfo.firmware_version & 0xFF,
           (int)devinfo.hardware_version);

    // check health...
    if (!checkSLAMTECLIDARHealth(drv)) {
        goto on_finished;
    }

    signal(SIGINT, ctrlc);

    drv->setMotorSpeed();
    // start scan...
    drv->startScan(0, 1);

    // fetech result and print it out...

    while (1) {
        sl_lidar_response_measurement_node_hq_t nodes[8192];
        size_t count = _countof(nodes);
        op_result = drv->grabScanDataHq(nodes, count);

        if (SL_IS_OK(op_result)) {
            printf("111");
            drv->ascendScanData(nodes, count);
            ////////////////////////////////////////
            ntime = time(NULL);
            ////////////////////////////////////////
            for (int pos = 0; pos < (int)count; ++pos) {
                printf("%s theta: %03.2f Dist: %08.2f Q: %d \n",
                       (nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ? "S "
                                                                         : "  ",
                       (nodes[pos].angle_z_q14 * 90.f) / 16384.f,
                       nodes[pos].dist_mm_q2 / 4.0f,
                       nodes[pos].quality >>
                           SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
                // //////////////////////////////////
                // 第一个激光点需要作为第一圈数据的rad0
                if (!flag) {
                    flag = true;
                    rad0 = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
                }
                float temprad = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
                // 当前后两次激光数据的角度值之差大于Circle_span_rad，说明雷达已经扫完一圈数据，
                // 将记录的一圈数据存放到Laser_data中并发布出去，然后初始化记录激光数据的各种变量
                if (rad_end - temprad > Circle_span_rad) {
                    Laser_data.nranges = nranges;
                    Laser_data.nintensities = nintensities;
                    Laser_data.utime = ntime;
                    Laser_data.rad0 = rad0;
                    Laser_data.radstep = (rad_end - rad0) / (nranges - 1);
                    Laser_data.ranges = ranges;
                    Laser_data.intensities = intensities;
                    printf("\nnutime=%d,nranges=%d,nintensities=%d,rad0=%03.2f,"
                           "radstep=%f\n\n",
                           Laser_data.utime, Laser_data.nranges,
                           Laser_data.nintensities, Laser_data.rad0,
                           Laser_data.radstep);
                    laser_t_publish(lcm, "HOKUYO_LIDAR", &Laser_data);
                    ntime = time(NULL);
                    rad0 = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
                    nranges = 0;
                    nintensities = 0;
                    memset(ranges, 0, 1024 * sizeof(float));
                    memset(intensities, 0, 1024 * sizeof(float));
                }
                rad_end = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
                ranges[nranges] = nodes[pos].dist_mm_q2 / 4.0f;
                // 注意，这里的激光强度值在导航算法中被改为角度值
                intensities[nintensities] = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
                // intensities[nintensities] =
                //     nodes[pos].quality >>
                //     SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT;
                nranges++;
                nintensities++;
                // ranges.push_back(nodes[pos].dist_mm_q2/4.0f);
                // intensities.push_back(nodes[pos].quality >>
                // SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
                // //////////////////////////////////
            }
        } else {
            printf("222");
        }

        if (ctrl_c_pressed) {
            break;
        }
    }

    drv->stop();
    delay(200);
    drv->setMotorSpeed(0);
    // done!
on_finished:
    if (drv) {
        delete drv;
        drv = NULL;
    }
    return 0;
}
