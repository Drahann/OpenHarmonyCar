#include "serial.h"

int fd = -1;         // 轮子的串口文件描述符
const char *portname; // 串口设备名
struct termios tty;
static pthread_mutex_t wheelFdMutex = PTHREAD_MUTEX_INITIALIZER;

static bool configureWheelFdLocked(void) {
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "Error from tcgetattr on %s: %s\n", portname,
                strerror(errno));
        return false;
    }

    cfsetospeed(&tty, B115200); // 设置输出波特率为115200
    cfsetispeed(&tty, B115200); // 设置输入波特率为115200

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // ignore break signal
    tty.c_lflag = 0;                            // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;     // no remapping, no delays
    tty.c_cc[VMIN] = 0;  // read doesn't block
    tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);        // ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag |= 0;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error from tcsetattr on %s: %s\n", portname,
                strerror(errno));
        return false;
    }
    return true;
}

static bool openWheelPortLocked(void) {
    const char *envPort = getenv("WHEEL_SERIAL_PORT");
    const char *selectedPort = (envPort != NULL && envPort[0] != '\0')
                                   ? envPort
                                   : "/dev/ttyUSB1";

    portname = selectedPort;
    printf("Wheel serial port: %s\n", portname);
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", portname, strerror(errno));
        const char *detectedPort = findUSBDev("wheel");
        if (detectedPort != NULL && strcmp(detectedPort, portname) != 0) {
            portname = detectedPort;
            printf("Wheel serial fallback port: %s\n", portname);
            fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
        }
    }
    if (fd < 0) {
        fprintf(stderr, "Error opening wheel serial port: %s\n",
                strerror(errno));
        return false;
    }

    if (!configureWheelFdLocked()) {
        close(fd);
        fd = -1;
        return false;
    }
    return true;
}

static void closeWheelPortLocked(void) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

static bool reopenWheelPortLocked(void) {
    closeWheelPortLocked();
    usleep(200 * 1000);
    return openWheelPortLocked();
}

/* 初始化串口通信 */
bool whellInit() {
    pthread_mutex_lock(&wheelFdMutex);
    bool ok = openWheelPortLocked();
    pthread_mutex_unlock(&wheelFdMutex);
    return ok;
}

/* 通过串口发送控制命令 */
bool wheelSend(byte a, byte a_v, byte b, byte b_v) {
    // 数据格式为 7 字节的自定义协议：{0x53, 0x05, a, a_v, b, b_v, checksum}
    // 0x53：固定的起始字节
    // 0x05：数据包长度
    // a，a_v：第一个轮子编号和速度值
    // b，b_v：第二个轮子编号和速度值
    // checksum：校验和（前6字节异或运算的结果）
    unsigned char data[7] = {0x53, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}; // 数据格式为 7 字节的自定义协议
    byte checksum = 0;
    data[2] = a;
    // TODO: 速度转换
    // 因为我们的机器人左右轮在相同速度数值之下，真实的速度不一致
    // int temp = ((int)a_v + 2);  // 所以在这里需要调整左轮速度
    int temp = ((int)a_v);  // 不需要在这里需要调整左轮速度
    a_v = (byte)temp;
    ////////////////////////
    data[3] = a_v;
    data[4] = b;
    data[5] = b_v;

    for (int i = 0; i < 6; i++) {
        checksum ^= data[i];
    }
    data[6] = checksum;

    pthread_mutex_lock(&wheelFdMutex);
    for (int attempt = 1; attempt <= 3; attempt++) {
        if (fd < 0 && !openWheelPortLocked()) {
            fprintf(stderr, "SERIAL_WRITE_ERROR reopen failed attempt=%d\n",
                    attempt);
            continue;
        }

        size_t written = 0;
        int savedErrno = 0;
        while (written < sizeof(data)) {
            ssize_t n = write(fd, data + written, sizeof(data) - written);
            if (n > 0) {
                written += (size_t)n;
                continue;
            }
            if (n < 0 && errno == EINTR) {
                continue;
            }
            savedErrno = (n < 0) ? errno : EIO;
            break;
        }

        if (written == sizeof(data)) {
            if (tcdrain(fd) != 0) {
                savedErrno = errno;
            } else {
                printf("Data %02X %02X %02X %02X %02X %02X %02X wheelSend successfully!\n",
                       data[0], data[1], data[2], data[3], data[4], data[5],
                       data[6]);
                pthread_mutex_unlock(&wheelFdMutex);
                return true;
            }
        }

        fprintf(stderr,
                "SERIAL_WRITE_ERROR port=%s fd=%d attempt=%d wrote=%zu/7 errno=%d(%s) data=%02X %02X %02X %02X %02X %02X %02X\n",
                portname ? portname : "(null)", fd, attempt, written,
                savedErrno, strerror(savedErrno), data[0], data[1], data[2],
                data[3], data[4], data[5], data[6]);

        if (!reopenWheelPortLocked()) {
            fprintf(stderr,
                    "SERIAL_RECOVER_ERROR port=%s attempt=%d errno=%d(%s)\n",
                    portname ? portname : "(null)", attempt, errno,
                    strerror(errno));
        }
    }

    pthread_mutex_unlock(&wheelFdMutex);
    return false;
}
