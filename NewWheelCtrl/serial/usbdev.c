#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define LIDAR_SERIAL "dce74d177fdc724ba5757727edc269bf"
#define WHEEL_SERIAL "0001"

enum usbDevice { lidar, wheel, other };
char device_path[256];

int identify_device(const char *port) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "udevadm info --name=%s | grep 'E: ID_SERIAL='",
             port);
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("popen");
        return other;
    }

    static char serial[256];
    if (fgets(serial, sizeof(serial), fp) != NULL) {
        if (strstr(serial, WHEEL_SERIAL)) {
            pclose(fp);
            return wheel;
        } else if (strstr(serial, LIDAR_SERIAL)) {
            pclose(fp);
            return lidar;
        }
    }
    pclose(fp);
    return other;
}

const char *findUSBDev(const char *device_type) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir("/dev");
    if (dir == NULL) {
        perror("opendir");
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "ttyUSB", 6) == 0) {
            snprintf(device_path, sizeof(device_path), "/dev/%s",
                     entry->d_name);
            int dev_type = identify_device(device_path);
            if ((dev_type == lidar && strcmp(device_type, "lidar") == 0) ||
                (dev_type == wheel && strcmp(device_type, "wheel") == 0)) {
                closedir(dir);
                return device_path;
            }
        }
    }

    closedir(dir);
    return NULL;
}
