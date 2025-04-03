#ifndef QIOF_H
#define QIOF_H
#include<limits.h>
#include<unistd.h>
#include<stdbool.h>
#include<stdint.h>
#include<sys/uio.h>
#define BDRV_SECTOR_BITS    9
#define BDRV_SECTOR_SIZE    (1ULL << BDRV_SECTOR_BITS)
typedef struct QIOF_blk {
    char *name;
    int refcnt;
}QIOF_blk;
#endif
