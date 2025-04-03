#ifndef LIBRARY_H
#define LIBRARY_H
#include "qiof_disk.h"
#include "qiof_heap.h"
#include "qiof_log.h"
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
QIOF_Status (*callback_QIOF_DiskIO_submit)(QIOF_DiskIO *qio) = NULL;
QIOF_Status (*callback_QIOF_DiskIO_abort)(QIOF_DiskIO *qio) = NULL;
QIOF_DiskIO *(*callback_QIOF_getDiskIO)(void) = NULL;
QIOF_DiskInfo *(*callback_QIOF_get_DiskInfo)(void) = NULL;
void (*callback_QIOF_snapshot)(void) = NULL;
char *(*callback_QIOF_getMirrorFile)(void) = NULL;
#define IOS_POOL_BYTE_SIZE 100 * 1024 * 1024;
PMEMORYPOOL io_pool = NULL;
void *pBuf;
uint64_t disk_size = 0;
int fd = 0;
QIOF_DiskInfo *qdisk = NULL;
typedef struct block
{
    void *buf;
    int size;
} block;
block *map_tbl = NULL;
int snapshot_cnt = 0;
char *mirror_path = NULL;
#endif
