#ifndef QIOF_DISK_H
#define QIOF_DISK_H
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <assert.h>
#include "qiof_status.h"
#include "qemu/queue.h"
// #include "qiof_queue.h"
#include "qemu/typedefs.h"
#include "qiof.h"
#define TIME_LEN 22

typedef struct QIOF_snapshot_info
{
	char *device_name;
	char *format;
} QIOF_snapshot_info;

typedef enum QIOF_IO_type
{
	QIOF_READ,
	QIOF_WRITE,
	QIOF_READV,
	QIOF_WRITEV
} QIOF_IO_type;
typedef int QIOF_DiskHandle;
typedef void QIOF_DiskCallback(void *opaque, QIOF_Status status);

typedef struct QIOF_DiskCB
{
	QIOF_DiskCallback *cb; // callback function
	void *opaque;
	QIOF_Status status;
} QIOF_DiskCB;

typedef struct QIOF_DiskIO
{
	time_t timestamp;
	QIOF_IO_type type;	// I/O operation type
	QIOF_DiskHandle fd; // file descriptor
	int64_t offset;
	int64_t bytes;
	void *buf; // linear buf address
	QTAILQ_ENTRY(QIOF_DiskIO)
	DiskIO_link; // qeueu DiskIO_link
} QIOF_DiskIO;

typedef struct QIOF_DiskInfo
{
	char *name; // disk name
	char *mirror;
	int64_t total_sectors;
} QIOF_DiskInfo;

typedef enum QIOF_Disk_CBType
{
	QIOF_SNAPSHOT,
	QIOF_GET_DISKINFO,
	QIOF_Disk_SUBMIT,
	QIOF_Disk_ABORT,
	QIOF_Disk_GETDISKIO,
	QIOF_GET_MIRROR_FILE,
} QIOF_Disk_CBType;

QIOF_Status QIOF_Disk_submit(QIOF_DiskIO *qaio);
QIOF_Status QIOF_Disk_abort(QIOF_DiskIO *qaio);
QIOF_DiskIO *QIOF_getDiskIO(void);
QIOF_DiskIO *QIOF_getPollTail(void);
QIOF_DiskInfo *QIOF_get_DiskInfo(void);
void QIOF_snapshot(void);
char *QIOF_getMirrorFile(void);
int qiof_test(void);
#endif
