#ifndef QIOF_DISK_H
#define QIOF_DISK_H
#include "qiof.h"
#include "qiof_status.h"
typedef int QIOF_DiskHandle;
typedef struct QIOF_DiskIOElem
{
    uint64_t addr;
    uint64_t length;
} QIOF_DiskIOElem;
typedef struct QIOF_DiskIO
{
    uint32_t numElems;
    uint64_t offset;
    QIOF_DiskIOElem *elems;
} QIOF_DiskIO;
typedef struct QIOF_DiskFilterProperty
{
    const char *name;
    const char *value;
} QIOF_DiskFilterProperty;
typedef struct QIOF_DiskLayout
{
    uint64_t capacity;
    uint32_t logicalSectorSize;
    uint32_t physicalSectorSize;
} QIOF_DiskLayout;
typedef struct QIOF_DiskInfo
{
    QIOF_DiskLayout diskLayout;
} QIOF_DiskInfo;
typedef struct QIOF_DiskOps
{
    /* data */
} QIOF_DiskOps;
typedef struct QIOF_DiskFilterInfo
{
    const char *name;
    const char *vendor;
    const char *version;
    const char *filterClass;
    const QIOF_DiskOps *ops;
} QIOF_DiskFilterInfo;
uint32_t QIOF_DiskMaxOutstandingIOsGet(void);
void QIOF_DiskIOContinue(QIOF_DiskIO *io);
void QIOF_DiskIOComplete(QIOF_DiskHandle *handle, QIOF_DiskIO *io, QIOF_Status status);
// QIOF_Status QIOF_DiskIOAlloc(QIOF_DiskHandle *handle, QIOF_HeapHandle *heap, uint32_t numElems, QIOF_DiskIO **outIO);
// QIOF_Status VMIOF_DiskIODup(QIOF_DiskHandle *handle, QIOF_HeapHandle *heap,const QIOF_DiskIO *origIO, QIOF_DiskIO **outIO);
void QIOF_DiskIOSubmit(QIOF_DiskHandle *handle, QIOF_DiskIO *io);
void QIOF_DiskIOAbort(QIOF_DiskHandle *handle, QIOF_DiskIO *io);
void QIOF_DiskIOFree(QIOF_DiskHandle *handle, QIOF_DiskIO *io);
void QIOF_DiskIOReInit(QIOF_DiskIO *io);
#endif