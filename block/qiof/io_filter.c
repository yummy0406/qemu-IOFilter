#include "qemu/osdep.h"
#include "block/block.h"
#include "block/block_int.h"
#include "sysemu/block-backend.h"
#include "qemu/queue.h"
#include "qemu/coroutine.h"
#include "io_filter.h"

// 环形缓冲区大小（16M）
#define BUFFER_SIZE (16 * 1024 * 1024)

// 数据块结构
typedef struct BufferBlock
{
    QTAILQ_ENTRY(BufferBlock)
    entry;           // QTAILQ队列节点
    uint8_t *data;   // 数据指针
    size_t size;     // 数据大小
    uint64_t offset; // 写入偏移量
} BufferBlock;

// 环形缓冲区结构
typedef struct RingBuffer
{
    QTAILQ_HEAD(, BufferBlock)
    queue;               // QTAILQ队列头
    size_t total_size;   // 缓冲区总大小（16M）
    size_t current_size; // 当前已使用大小
} RingBuffer;

// 全局环形缓冲区
static RingBuffer ring_buffer;

// 初始化环形缓冲区
static void ring_buffer_init(void)
{
    QTAILQ_INIT(&ring_buffer.queue);
    ring_buffer.total_size = BUFFER_SIZE;
    ring_buffer.current_size = 0;
}

// 释放数据块
static void free_buffer_block(BufferBlock *block)
{
    if (block)
    {
        qemu_vfree(block->data);
        g_free(block);
    }
}

// 写入数据到环形缓冲区
static void ring_buffer_write(uint8_t *data, size_t size, uint64_t offset)
{
    // 如果新数据将导致缓冲区溢出，移除最早的数据块
    while (ring_buffer.current_size + size > ring_buffer.total_size)
    {
        BufferBlock *old_block = QTAILQ_FIRST(&ring_buffer.queue);
        if (!old_block)
        {
            break; // 队列为空
        }
        QTAILQ_REMOVE(&ring_buffer.queue, old_block, entry);
        ring_buffer.current_size -= old_block->size;
        free_buffer_block(old_block);
    }

    // 创建新数据块
    BufferBlock *new_block = g_new(BufferBlock, 1);
    new_block->data = qemu_blockalign(NULL, size); // 分配对齐的内存
    new_block->size = size;
    new_block->offset = offset;

    // 复制数据
    memcpy(new_block->data, data, size);

    // 加入队列
    QTAILQ_INSERT_TAIL(&ring_buffer.queue, new_block, entry);
    ring_buffer.current_size += size;
}

// IO过滤器提交函数
void coroutine_fn ioData_submit(BlockBackend *blk, uint64_t offset,
                                QEMUIOVector *qiov, int flags,
                                BlockCompletionFunc *cb, void *opaque)
{
    // 初始化环形缓冲区（仅在第一次调用时执行）
    static bool initialized = false;
    if (!initialized)
    {
        ring_buffer_init();
        initialized = true;
    }

    // 截获数据
    size_t total_size = qiov->size;
    BlockDriverState *bs = blk_bs(blk);
    uint8_t *data = qemu_blockalign(bs, total_size);
    qemu_iovec_to_buf(qiov, 0, data, total_size);

    // 写入环形缓冲区
    ring_buffer_write(data, total_size, offset);
    // 释放临时数据
    qemu_vfree(data);
}

// IO完成回调函数
void QIOF_complete_cb(void *opaque, int ret)
{
    IOFSubmitArgs *args = opaque;

    if (ret < 0)
    {
        fprintf(stderr, "IO operation failed: %d\n", ret);
    }
    g_free(args);

    fprintf(stdout, "Data intercepted!\n");
}
