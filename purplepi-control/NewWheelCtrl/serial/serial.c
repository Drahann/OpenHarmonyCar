/* 
* 程序实现了一个基于串口通信的轮控模块控制逻辑
* 并通过LCM实现消息订阅和发布
* 主要功能包括：
*   1. 初始化模块
*   2. 处理控制命令
*   3. 解析路径规划指令
*   4. 更新和发布位置信息等
*/
#include "serial.h"
#include <math.h>

lcm_t *lcm;

// TODO 屏蔽停止指令
bool stopFlag = false;
////////////////////////

// curStatus：机器人当前的运行状态，1 表示前进，2 表示左转，3 表示右转
// curSpeed：机器人当前的线速度，通常以百分比形式表示（相当于最大速度的百分比）
int curStatus = -1, curSpeed = 0; // 这里的速度指百分比速度
double curOmega = 0.0;  // 当前的角速度，即机器人的旋转速度，初始状态下为0表示机器人没有旋转（角速度为0）
// 临界区数据
pthread_mutex_t curPoseMutex;
clock_t lastTime;
double curPose[3] = {0, 0, 0};

static double clampDouble(double value, double minValue, double maxValue) {
    if (value < minValue)
        return minValue;
    if (value > maxValue)
        return maxValue;
    return value;
}

static void encodeWheelPercent(double percent, byte *direction, byte *speed) {
    double magnitude = fabs(percent);
    if (magnitude <= 0.5) {
        *direction = 0x00;
        *speed = 0x00;
        return;
    }
    if (magnitude < MIN_NONZERO_WHEEL_PERCENT) {
        magnitude = MIN_NONZERO_WHEEL_PERCENT;
    }
    magnitude = clampDouble(magnitude, 0.0, 60.0);
    *direction = percent > 0 ? 0x01 : 0x02;
    *speed = (byte)magnitude;
}

static double signedWheelPercent(byte direction, byte speed) {
    if (direction == 0x01) {
        return (double)speed;
    }
    if (direction == 0x02) {
        return -(double)speed;
    }
    return 0.0;
}

int main() {
    // 串口初始化
    if (!whellInit()) {
        goto err;
    }
    // 创建LCM对象
    lcm = lcm_create(NULL);
    if (!lcm) {
        fprintf(stderr, "Failed to create LCM\n");
        goto err;
    }

    // 订阅来自手机端的轮控消息
    lastTime = clock();
    path_ctrl_t_subscribe(lcm, "wheel_ctrl", parseCmd, NULL);
    // 订阅来自算法模块的路径规划消息
    path_t_subscribe(lcm, "PATH", parsePath, NULL);
    // 来自算法模块的当前位置信息
    pthread_mutex_init(&curPoseMutex, NULL);
    pose_t_subscribe(lcm, "CURRENTPOSE", setCurPose, NULL);
    // 新线程，定时发送当前位置信息
    // pthread_t 是一个类型，用于标识线程对象
    // pthread_t 是POSIX线程库定义的线程标识符数据类型
    // thread 是一个变量，它将用于创建和操作线程
    pthread_t thread;   // 是POSIX线程（pthread）库中用来声明线程变量的语句
    if (pthread_create(&thread, NULL, sendCurPose, NULL) != 0) {    // 创建线程
                        // &thread：传递 pthread_t 类型的地址，用于保存新线程的 ID
                        // sendCurPose：线程执行的函数
                        // 后一个NULL：传递给线程函数的参数
        fprintf(stderr, "Error: Failed to create thread\n");
        goto err;
    }

    while (true) {
        // 进入一个无限循环，持续调用 lcm_handle()
        // 处理所有订阅的消息（例如来自 wheel_ctrl 和 PATH 的消息）
        // 每当有新消息到达时，lcm_handle 会触发对应的回调函数，例如：
        //  parseCmd 用于处理轮控命令
        //  parsePath 用于处理路径规划数据
        lcm_handle(lcm);
    }

err:
    // close(fd);
    return 0;
}

/* 处理轮控命令，订阅来自 wheel_ctrl 的消息并解析，响应不同的状态 */
void parseCmd(const lcm_recv_buf_t *rbuf, const char *channel,
              const path_ctrl_t *msg, void *userdata) {
    byte status = msg->cmd, speed = msg->speed;
    // 手机端在转弯过程中只能设置状态，速度是默认的
    // 所以对于转弯，只需要判断状态是否改变
    if (curStatus == status && curSpeed == speed) {
        return;
    }

    // 状态改变，计算新的位置
    renewCurPose();

    // 下发新的指令
    switch (status) {
    case 1:
        // 状态 1：双轮前进，设置相同速度
        if (wheelSend(0x01, speed, 0x01, speed)) {
            curSpeed = speed;
            curOmega = 0;
        } else {
            curSpeed = 0;
            curOmega = 0;
        }
        break;
    case 2:
        // 状态 2：左转，右轮运动，左轮停止
        if (wheelSend(0x02, DEFAULT_SPEED, 0x01, DEFAULT_SPEED)) {
            curOmega = (double)DEFAULT_SPEED * FULLSPEED / 100 / RADIUS;    // 正角速度，表示顺时针旋转
            curSpeed = 0;
        } else {
            curOmega = 0;
            curSpeed = 0;
        }
        break;
    case 3:
        // 状态 3：右转，左轮运动，右轮停止
        if (wheelSend(0x01, DEFAULT_SPEED, 0x02, DEFAULT_SPEED)) {
            curOmega = (double)-DEFAULT_SPEED * FULLSPEED / 100 / RADIUS;   // 负角速度，表示逆时针旋转
            curSpeed = 0;
        } else {
            curOmega = 0;
            curSpeed = 0;
        }
        break;
    // TODO 屏蔽停止指令
    case 4:
        // 状态 4：停止 2 秒（通过 stopFlag 标记）
        wheelSend(0x00, 0x00, 0x00, 0x00);
        curOmega = 0;
        curSpeed = 0;
        stopFlag = true;
        printf("Stop for 2 seconds\n");
        break;
    // 取消屏蔽
    case 5:
        // 状态 5：停止但允许接收新命令
        wheelSend(0x00, 0x00, 0x00, 0x00);
        curOmega = 0;
        curSpeed = 0;
        stopFlag = false;
        printf("Stop but can recieve cmd\n");
        break;
    ////////////////////////////////////
    case 0:
    default:
        // 默认：停止
        wheelSend(0x00, 0x00, 0x00, 0x00);
        curOmega = 0;
        curSpeed = 0;
        break;
    }
    // 更新状态
    curStatus = status;
}

/* 解析路径规划指令，订阅来自 PATH 的路径规划消息，计算轮速并发送至轮控模块 */
void parsePath(const lcm_recv_buf_t *rbuf, const char *channel,
               const path_t *msg, void *userdata) {
    // TODO
    if (stopFlag) {
        // 如果停止标志（stopFlag）为真，则直接返回
        return;
    }
    /////////////////
    int16_t length = msg->length;   // 表示从路径规划模块接收到的路径消息中，路径点的数量
    double v, w;
    double vWheels[2];
    int8_t bWheels[2];
    if (length <= 0) {
        fprintf(stderr, "Error: Invalid path\n");
        return;
    }
    // 解析路径规划消息，提取线速度 v 和角速度 w
    v = msg->xyr[0][0]; // 线速度
    w = msg->xyr[0][1]; // 角速度
    printf("v: %f m/s, w: %f rad/s\n", v, w);
    if (fabs(v) <= 0.01 && fabs(w) <= 0.01) {
        renewCurPose();
        if (wheelSend(0x00, 0x00, 0x00, 0x00)) {
            curSpeed = 0;
            curOmega = 0;
        } else {
            curSpeed = 0;
            curOmega = 0;
            fprintf(stderr, "SERIAL_ODOM_FREEZE write failed for stop command\n");
        }
    } else {
        if (fabs(w) <= 0.2) {
            w *= 2;
        }
        w = clampDouble(w, -0.8, 0.8);
        vWheels[0] = (double)(v - w * RADIUS) / FULLSPEED * 100;
        vWheels[1] = (double)(v + w * RADIUS) / FULLSPEED * 100;
        vWheels[0] = clampDouble(vWheels[0], -60.0, 60.0);
        vWheels[1] = clampDouble(vWheels[1], -60.0, 60.0);
        renewCurPose();
        byte leftDir;
        byte leftSpeed;
        byte rightDir;
        byte rightSpeed;
        encodeWheelPercent(vWheels[0], &leftDir, &leftSpeed);
        encodeWheelPercent(vWheels[1], &rightDir, &rightSpeed);
        bWheels[0] = leftDir;
        bWheels[1] = rightDir;
        if (wheelSend(bWheels[0], leftSpeed, bWheels[1], rightSpeed)) {
            double leftPercent = signedWheelPercent(leftDir, leftSpeed);
            double rightPercent = signedWheelPercent(rightDir, rightSpeed);
            curSpeed = (int8_t)((leftPercent + rightPercent) / 2.0);
            curOmega = ((rightPercent - leftPercent) / 2.0) *
                       FULLSPEED / 100.0 / RADIUS;
        } else {
            curSpeed = 0;
            curOmega = 0;
            fprintf(stderr,
                    "SERIAL_ODOM_FREEZE write failed for PATH v=%f w=%f\n",
                    v, w);
        }
    }
    printf("curSpeed: %d, curOmega: %f\n", curSpeed, curOmega);
}

void setCurPose(const lcm_recv_buf_t *rbuf, const char *channel,
                const pose_t *msg, void *userdata) {
    pthread_mutex_lock(&curPoseMutex);
    curPose[0] = msg->pos[0];
    curPose[1] = msg->pos[1];
    curPose[2] = msg->pos[2];
    lastTime = clock();
    pthread_mutex_unlock(&curPoseMutex);
}

/* 发布当前位置，在独立线程中定期计算并发布当前位置信息到 POSE 频道，使用互斥锁保证数据一致性 */
void *sendCurPose(void *args) {
    pose_t pose;
    while (true) {
        renewCurPose();
        pthread_mutex_lock(&curPoseMutex);
        pose.pos[0] = curPose[0];
        pose.pos[1] = curPose[1];
        pose.pos[2] = curPose[2];
        pthread_mutex_unlock(&curPoseMutex);
        pose_t_publish(lcm, "POSE", &pose);
        usleep(15 * 1000);
    }
}

/* 根据当前时间和速度更新位置信息 */
void renewCurPose() {
    clock_t curTime = clock();
    double dt = (double)(curTime - lastTime) / CLOCKS_PER_SEC;
    double speed = (double)curSpeed * FULLSPEED / 100;

    // 线程安全：通过互斥锁保护对 curPose 的访问
    pthread_mutex_lock(&curPoseMutex);
    curPose[0] += speed * cos(curPose[2]) * dt;
    curPose[1] += speed * sin(curPose[2]) * dt;
    curPose[2] += curOmega * dt;
    lastTime = clock();
    pthread_mutex_unlock(&curPoseMutex);
}
