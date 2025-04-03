#ifndef _BITMAP_H_
#define _BITMAP_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "qiof_log.h"
/*
 *功能：初始化bitmap
 *参数：
 *size：bitmap的大小，即bit位的个数
 *start：起始值
 *返回值：0表示失败，1表示成功
 */
int qiof_bitmap_init(uint64_t size, uint64_t start);

/*
 *功能：将值index的对应位设为1
 *index:要设的值
 *返回值：0表示失败，1表示成功
 */
int qiof_bitmap_set(uint64_t index);

/*
 *功能：取bitmap第i位的值
 *i：待取位
 *返回值：-1表示失败，否则返回对应位的值
 */
int qiof_bitmap_get(uint64_t i);

/*
 *功能：返回index位对应的值
 */
uint64_t qiof_bitmap_data(uint64_t index);

/*释放内存*/
void qiof_bitmap_free(void);

/*检查bitmap是否为空*/
int qiof_bitmap_check(void);

#endif