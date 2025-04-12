#include "qemu/osdep.h"
#include "block/block_int.h"
#include "block/block.h"
#include "qemu/main-loop.h"
#include "qemu/queue.h"
#include "qapi/error.h"

typedef struct BDRVQaioState BDRVQaioState;
typedef struct IOFilter IOFilter;
typedef struct IOFilterOps IOFilterOps;

struct BDRVQaioState
{
    BlockDriverState *bs;
    QEMUIOVector *qiov;
    int64_t offset;
    int64_t bytes;
    time_t timestamp;
    QTAILQ_HEAD(, IOFilter)
    filters;
};

struct IOFilter
{
    char *filter_name;
    void *opaque;
    IOFilterOps *ops;
    QTAILQ_ENTRY(IOFilter)
    entry;
};

struct IOFilterOps
{
    int (*init)(BDRVQaioState *qs, Error **errp);
    void (*start)(BDRVQaioState *qs, Error **errp);
    void (*stop)(BDRVQaioState *qs, Error **errp);
    void (*get_error)(BDRVQaioState *qs, Error **errp);
};

static int IOFilter_create(BDRVQaioState *qs, IOFilterOps *ops, const char *filter_name, Error **errp)
{
    IOFilter *iof = g_new0(IOFilter, 1);
    int ret;

    iof->filter_name = g_strdup(filter_name);
    iof->opaque = qs;
    iof->ops = ops;
    if (iof->ops->init)
    {
        ret = iof->ops->init(qs, errp);
        if (ret < 0)
        {
            g_free(iof->filter_name);
            g_free(iof);
            return ret;
        }
    }

    QTAILQ_INSERT_TAIL(&qs->filters, iof, entry);
    return 0;
}

static void qaio_work(BDRVQaioState *qs)
{
    IOFilter *iof, *next;
    Error *local_err = NULL;

    QTAILQ_FOREACH_SAFE(iof, &qs->filters, entry, next)
    {
        if (iof->ops && iof->ops->start)
        {
            iof->ops->start(qs, &local_err);
        }
        if (local_err)
        {
            error_report_err(local_err);
            return;
        }
    }
}

static int qaio_open(BlockDriverState *bs, QDict *options, int flags, Error **errp)
{
    BDRVQaioState *qs = bs->opaque;
    int ret;

    ret = bdrv_open_file_child(NULL, options, "file", bs, errp);
    if (ret < 0)
    {
        return ret;
    }

    QTAILQ_INIT(&qs->filters);
    qs->qiov = NULL;

    // 示例：添加统计过滤器（第三方开发者可在此添加自己的过滤器）
    static IOFilterOps test_ops = {
        .init = NULL,      // 需实现
        .start = NULL,     // 需实现
        .stop = NULL,      // 需实现
        .get_error = NULL, // 需实现
    };
    ret = IOFilter_create(qs, &test_ops, "test_filter", errp);
    if (ret < 0)
    {
        return ret;
    }

    fprintf(stdout, "BDRV qaio_open success \n");
    printf("Child file: %s\n", bs->file ? bdrv_get_node_name(bs->file->bs) : "NULL");

    return 0;
}

static void qaio_close(BlockDriverState *bs)
{
    BDRVQaioState *qs = bs->opaque;
    IOFilter *iof, *next;
    Error *local_err = NULL;

    QTAILQ_FOREACH_SAFE(iof, &qs->filters, entry, next)
    {
        QTAILQ_REMOVE(&qs->filters, iof, entry);
        if (iof->ops && iof->ops->stop)
        {
            iof->ops->stop(qs, &local_err);
            if (local_err)
            {
                error_report_err(local_err);
                local_err = NULL;
            }
        }
        g_free(iof->filter_name);
        g_free(iof);
    }

    if (qs->qiov)
    {
        qemu_iovec_destroy(qs->qiov);
        g_free(qs->qiov);
    }

    fprintf(stdout, "qaio_close\n");
}

static int coroutine_fn GRAPH_RDLOCK
qaio_co_flush(BlockDriverState *bs)
{
    return bdrv_co_flush(bs->file->bs);
}

static int64_t coroutine_fn GRAPH_RDLOCK
qaio_co_getlength(BlockDriverState *bs)
{
    return bdrv_co_getlength(bs->file->bs);
}

static int coroutine_fn GRAPH_RDLOCK
qaio_co_preadv(BlockDriverState *bs, int64_t offset, int64_t bytes,
               QEMUIOVector *qiov, BdrvRequestFlags flags)
{
    return bdrv_co_preadv(bs->file, offset, bytes, qiov, flags);
}

static int coroutine_fn GRAPH_RDLOCK
qaio_co_pwrite_zeroes(BlockDriverState *bs, int64_t offset,
                      int64_t bytes, BdrvRequestFlags flags)
{
    return bdrv_co_pwrite_zeroes(bs->file, offset, bytes, flags);
}

static int coroutine_fn GRAPH_RDLOCK
qaio_co_pwritev(BlockDriverState *bs, int64_t offset, int64_t bytes,
                QEMUIOVector *qiov, BdrvRequestFlags flags)
{
    BDRVQaioState *s = bs->opaque;

    // 捕获 I/O 数据
    if (s->qiov)
    {
        qemu_iovec_destroy(s->qiov);
        g_free(s->qiov);
    }
    s->qiov = g_new(QEMUIOVector, 1);
    qemu_iovec_init(s->qiov, qiov->niov);
    qemu_iovec_concat(s->qiov, qiov, 0, bytes); // 使用 bytes 而非 qiov->size
    s->offset = offset;
    s->bytes = bytes;
    s->timestamp = time(NULL);

    // 执行过滤器
    qaio_work(s);

    return bdrv_co_pwritev(bs->file, offset, bytes, qiov, flags);
}

BlockDriver bdrv_qaio = {
    .format_name = "qaio",
    .instance_size = sizeof(BDRVQaioState),

    .bdrv_open = qaio_open,
    .bdrv_close = qaio_close,
    .bdrv_co_flush = qaio_co_flush,

    .bdrv_child_perm = bdrv_default_perms,
    .bdrv_co_getlength = qaio_co_getlength,

    .bdrv_co_pwrite_zeroes = qaio_co_pwrite_zeroes,
    .bdrv_co_preadv = qaio_co_preadv,
    .bdrv_co_pwritev = qaio_co_pwritev,

    .is_filter = true,
};

static void qaio_register(void)
{
    printf("Registering qaio\n");
    bdrv_register(&bdrv_qaio);
}

block_init(qaio_register);