#include "udp2lcm.h"

/* 初始化函数，用于配置并分配 robot_control_t 结构的各个字段 */
/*
 * 函数参数说明：
 * robot_control_t *robotCtrlData：
 *  1. 指向待初始化的控制数据结构
 *  2. 函数会通过指针直接修改该结构体的内容
 * int64_t utime：时间戳，用于记录指令的时间
 * int8_t commandid：指令的类型或编号，标识具体的控制命令
 * int8_t robotid：被控制的机器人 ID，适用于多机器人场景
 * int8_t ndparams：double 类型参数数量（如速度、位置等连续值）
 * int8_t niparams：int8_t 类型参数数量（如简单标志值、选项等）
 * int8_t nsparams：字符串参数数量（如配置文件路径等）
 * int64_t nbparams：字节流参数数量（如二进制数据块）
 */
void robotCtrlInit(robot_control_t *robotCtrlData, int64_t utime,
                   int8_t commandid, int8_t robotid, int8_t ndparams,
                   int8_t niparams, int8_t nsparams, int64_t nbparams) {
    if (robotCtrlData == NULL) {
        fprintf(stderr, "Error: Invalid robot control data\n");
        return;
    }
    robotCtrlData->utime = utime;
    robotCtrlData->commandid = commandid;
    robotCtrlData->robotid = robotid;
    robotCtrlData->ndparams = ndparams;
    robotCtrlData->dparams = NULL;
    robotCtrlData->niparams = niparams;
    robotCtrlData->iparams = NULL;
    robotCtrlData->nsparams = nsparams;
    robotCtrlData->sparams = NULL;
    robotCtrlData->nbparams = nbparams;
    robotCtrlData->bparams = NULL;
    if (ndparams > 0) {
        robotCtrlData->dparams = (double *)malloc(ndparams * sizeof(double));
        memset(robotCtrlData->dparams, 0, ndparams * sizeof(double));
    }
    if (niparams > 0) {
        robotCtrlData->iparams = (int8_t *)malloc(niparams * sizeof(int8_t));
        memset(robotCtrlData->iparams, 0, niparams * sizeof(int8_t));
    }
    if (nsparams > 0) {
        robotCtrlData->sparams = (char **)malloc(nsparams * sizeof(char *));
        memset(robotCtrlData->sparams, 0, nsparams * sizeof(char *));
    }
    if (nbparams > 0) {
        robotCtrlData->bparams = (uint8_t *)malloc(nbparams * sizeof(uint8_t));
        memset(robotCtrlData->bparams, 0, nbparams * sizeof(uint8_t));
    }
}

void freeRobotCtrl(robot_control_t *robotCtrlData) {
    if (robotCtrlData == NULL) {
        fprintf(stderr, "Error: Invalid robot control data\n");
        return;
    }
    if (robotCtrlData->dparams != NULL) {
        free(robotCtrlData->dparams);
        robotCtrlData->dparams = NULL;
    }
    if (robotCtrlData->iparams != NULL) {
        free(robotCtrlData->iparams);
        robotCtrlData->iparams = NULL;
    }
    if (robotCtrlData->sparams != NULL) {
        free(robotCtrlData->sparams);
        robotCtrlData->sparams = NULL;
    }
    if (robotCtrlData->bparams != NULL) {
        free(robotCtrlData->bparams);
        robotCtrlData->bparams = NULL;
    }
}