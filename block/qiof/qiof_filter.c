#include "qemu/osdep.h"
#include "block/block_int.h"
#include "qemu/queue.h"
// #include "qiof_filter.h"

typedef struct BufferBlock
{
    QTAILQ_ENTRY(BufferBlock)
    entry;
    uint8_t *data;
    size_t size;
    uint64_t offset;
} BufferBlock;

typedef struct RingBuffer
{
    QTAILQ_HEAD(, BufferBlock)
    queue;
    size_t total_size;
    size_t current_size;
} RingBuffer;

typedef struct BDRVQiofFilterState
{
    BlockDriverState *bs;
    RingBuffer ring_buffer;
} BDRVQiofFilterState;

static void ring_buffer_init(RingBuffer *rb)
{
    QTAILQ_INIT(&rb->queue);
    rb->total_size = 16 * 1024 * 1024;
    rb->current_size = 0;
}

static void ring_buffer_write(RingBuffer *rb, uint8_t *data, size_t size, uint64_t offset)
{
    while (rb->current_size + size > rb->total_size)
    {
        BufferBlock *old_block = QTAILQ_FIRST(&rb->queue);
        if (!old_block)
            break;
        QTAILQ_REMOVE(&rb->queue, old_block, entry);
        rb->current_size -= old_block->size;
        g_free(old_block->data);
        g_free(old_block);
    }
    BufferBlock *new_block = g_new(BufferBlock, 1);
    new_block->data = g_malloc(size);
    if (!new_block->data)
    {
        g_free(new_block);
        return;
    }
    memcpy(new_block->data, data, size);
    new_block->size = size;
    new_block->offset = offset;
    QTAILQ_INSERT_TAIL(&rb->queue, new_block, entry);
    rb->current_size += size;
}

static void ring_buffer_cleanup(RingBuffer *rb)
{
    BufferBlock *block;
    while ((block = QTAILQ_FIRST(&rb->queue)))
    {
        QTAILQ_REMOVE(&rb->queue, block, entry);
        g_free(block->data);
        g_free(block);
    }
}

static void qiof_filter_close(BlockDriverState *bs)
{
    BDRVQiofFilterState *s = bs->opaque;
    ring_buffer_cleanup(&s->ring_buffer);
}

static int qiof_filter_open(BlockDriverState *bs, QDict *options, int flags, Error **errp)
{
    BDRVQiofFilterState *s = bs->opaque;
    s->bs = bdrv_open_child(NULL, options, "file", bs, &child_of_bds, BDRV_CHILD_FILTERED, false, errp);
    if (!s->bs)
    {
        return -EINVAL;
    }
    ring_buffer_init(&s->ring_buffer);
    return 0;
}

static coroutine_fn int qiof_filter_co_pwritev(BlockDriverState *bs, uint64_t offset, uint64_t bytes,
                                               QEMUIOVector *qiov, int flags)
{
    BDRVQiofFilterState *s = bs->opaque;
    size_t total_size = qiov->size;
    uint8_t *data = g_malloc(total_size);
    if (!data)
        return -ENOMEM;
    qemu_iovec_to_buf(qiov, 0, data, total_size);
    ring_buffer_write(&s->ring_buffer, data, total_size, offset);
    g_free(data);
    return bdrv_co_pwritev(s->bs, offset, bytes, qiov, flags);
}

BlockDriver bdrv_qiof_filter = {
    .format_name = "qiof_filter",
    .instance_size = sizeof(BDRVQiofFilterState),
    .bdrv_open = qiof_filter_open,
    .bdrv_close = qiof_filter_close,
    .bdrv_co_pwritev = qiof_filter_co_pwritev,
    .is_filter = true,
};

static void qiof_filter_register(void)
{
    printf("Registering qiof_filter\n");
    bdrv_register(&bdrv_qiof_filter);
}

block_init(qiof_filter_register);