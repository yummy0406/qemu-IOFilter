#ifndef QIOF_IMG_H
#define QIOF_IMG_H
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include<stdbool.h>
int QIOF_img_cp(uint32_t size, char **argv);
#endif