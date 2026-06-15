#ifndef SERIAL_H
#define SERIAL_H

#include "path_ctrl_t.h"
#include "path_t.h"
#include "pose_t.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024
#define DEFAULT_SPEED 0x15 // 默认速度，百分比
#define MIN_NONZERO_WHEEL_PERCENT 12
// TODO: 两个待测数据
#define RADIUS 0.13    // 两轮之间的距离的一半，米
#define FULLSPEED 0.60 // 轮子全速，m/s
// #define fabs(x) ((x) > 0 ? (x) : -(x))
typedef unsigned char byte;

int identify_device(const char *port);
const char *findUSBDev(const char *device_type);

bool whellInit();
bool wheelSend(byte a, byte a_v, byte b, byte b_v);
void parseCmd(const lcm_recv_buf_t *rbuf, const char *channel,
              const path_ctrl_t *msg, void *userdata);
void parsePath(const lcm_recv_buf_t *rbuf, const char *channel,
               const path_t *msg, void *userdata);
void setCurPose(const lcm_recv_buf_t *rbuf, const char *channel,
                const pose_t *msg, void *userdata);
void renewCurPose();
void *sendCurPose(void *args);

#endif
