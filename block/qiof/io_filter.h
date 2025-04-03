#ifndef IO_FILTER_H
#define IO_FILTER_H

#include "block/block.h"
#include "qemu/coroutine.h"

typedef struct IOFSubmitArgs
{
    BlockBackend *blk;
    uint64_t offset;
    QEMUIOVector *qiov;
    int flags;
    BlockCompletionFunc *cb;
    void *opaque;
} IOFSubmitArgs;

void coroutine_fn ioData_submit(BlockBackend *blk, uint64_t offset,
                                QEMUIOVector *qiov, int flags,
                                BlockCompletionFunc *cb, void *opaque);

static void coroutine_fn IOF_submit_co_entry(void *opaque)
{
    // fprintf(stdout, "Data intercepting...\n");
    IOFSubmitArgs *args = opaque;
    ioData_submit(args->blk, args->offset, args->qiov, args->flags, args->cb, args->opaque);
    args->cb(args, 1);
}

void QIOF_complete_cb(void *opaque, int ret);

#endif /* QIOF_DISK_H */