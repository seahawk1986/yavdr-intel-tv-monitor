/**
 * Detects if a TV is attached by looking for
 * the HDCP i2c port 0x3a. 
 *
 * Borrowed code from https://github.com/beralt/istvup/blob/master/istvup.c which contains
 * borrowed code from i2cdetect from the lm-sensors (License LGPL 2.1)
 * project.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/* dbus stuff */
#include "dbus/dbus.h"

#define HDCP_I2C_PORT 0x3a

/* thanks to lm-sensors */
__s32 i2c_smbus_access(int file, char read_write, __u8 command, int size, union i2c_smbus_data *data)
{
    struct i2c_smbus_ioctl_data args;
    __s32 err;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;

    err = ioctl(file, I2C_SMBUS, &args);
    if (err == -1)
        err = -errno;
    return err;
}

__s32 i2c_smbus_write_quick(int file, __u8 value)
{
    return i2c_smbus_access(file, value, 0, I2C_SMBUS_QUICK, NULL);
}

void sleep_seconds(double seconds)
{
    const int usec2sec = 1000 * 1000; // 1000 µs * 1000 ms = 1s
    usleep(usec2sec * seconds);
}

int main(int argc, char *argv[])
{
    int fd, ret, started = 1;

    const double SHORT_SLEEP_DURATION = 1.0f;
    const double LONG_SLEEP_DURATION = 5.0f;
    double sleep_duration = SHORT_SLEEP_DURATION;

    char filename[20];
    DBusConnection *conn;
    DBusError err;
    DBusMessage *response, *startmsg, *stopmsg;
    int fe_success;
    char* fe_msg;

    if(argc < 2) {
        fprintf(stderr, "usage: %s [i2c-dev]\n", argv[0]);
        exit(1);
    }

    snprintf(filename, 19, "/dev/i2c-%s", argv[1]);
    fd = open(filename, O_RDWR);
    if(fd < 0) {
        fprintf(stderr, "failed to open %s\n", filename);
        exit(1);
    }

    if(ioctl(fd, I2C_SLAVE, HDCP_I2C_PORT) < 0) {
        fprintf(stderr, "failed to set i2c slave\n");
        close(fd);
        exit(1);
    }

    /* setup dbus */
    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if(dbus_error_is_set(&err)) {
        fprintf(stderr, "failed to open dbus: %s\n", err.message);
        close(fd);
        exit(1);
    }

    stopmsg = dbus_message_new_method_call("de.yavdr.frontend", "/de/yavdr/frontend",
                                           "de.yavdr.frontend.Controller", "stop");
    if(!stopmsg) {
        fprintf(stderr, "failed to create dbus message\n");
        close(fd);
        exit(1);
    }
    
    startmsg = dbus_message_new_method_call("de.yavdr.frontend", "/de/yavdr/frontend",
                                            "de.yavdr.frontend.Controller", "start");
    if(!startmsg) {
        fprintf(stderr, "failed to create dbus message\n");
        close(fd);
        exit(1);
    }

    ret = 0;

    while(1) {
        /* probe by writing */
        ret = i2c_smbus_write_quick(fd, I2C_SMBUS_WRITE);
        if(ret < 0) {
            /* tv is not on */
            if(started > 0) {
                /* stop frontend */
                response = dbus_connection_send_with_reply_and_block(conn, stopmsg, -1, &err);
                if(dbus_error_is_set(&err)) {
                    fprintf(stderr, "failed to stop frontend: %s\n", err.message);
                    close(fd);
                    exit(1);
                } else {
                    dbus_message_get_args(response, &err,
                                          DBUS_TYPE_BOOLEAN, &fe_success,
                                          DBUS_TYPE_STRING, &fe_msg,
                                          DBUS_TYPE_INVALID);
                    if(dbus_error_is_set(&err)) {
                        fprintf(stderr, "failed to stop frontend: %s\n", err.message);
                    } else {
                        fprintf(stderr, "stop frontend got response: %d %s\n", fe_success, fe_msg);
                        if (fe_success == 0) {
                            // error
                            fprintf(stderr, "could not stop frontend\n");
                        } else {
                            fprintf(stderr, "stopped frontend\n");
                            started = 0;
                            sleep_duration = SHORT_SLEEP_DURATION;
                        }
                    }
                }
            }
        } else {
            if(started == 0) {
                /* tv is on, start frontend */
                response = dbus_connection_send_with_reply_and_block(conn, startmsg, -1, &err);
                if(dbus_error_is_set(&err)) {
                    fprintf(stderr, "failed to start frontend: %s\n", err.message);
                    close(fd);
                    exit(1);
                } else {
                    dbus_message_get_args(response, &err,
                                          DBUS_TYPE_BOOLEAN, &fe_success,
                                          DBUS_TYPE_STRING, &fe_msg,
                                          DBUS_TYPE_INVALID);
                    if(dbus_error_is_set(&err)) {
                        fprintf(stderr, "failed to stop frontend: %s\n", err.message);
                    } else {
                        fprintf(stderr, "start frontend got response: %d %s\n", fe_success, fe_msg);
                        if (fe_success == 0) {
                            // error
                            fprintf(stderr, "could not start frontend\n");
                        } else {
                            fprintf(stderr, "started frontend\n");
                            started = 0;
                            sleep_duration = LONG_SLEEP_DURATION;
                        }
                    }
                }
            }
        }
        dbus_connection_flush(conn);
        /* sleep a while */
        fprintf(stderr, "sleep for %.2f seconds\n", sleep_duration);
        sleep_seconds(sleep_duration);
    }

    // cleanup
    free(fe_msg);
    dbus_message_unref(startmsg);
    dbus_message_unref(stopmsg);
    dbus_connection_close(conn);
    close(fd);
    exit(0);
}
