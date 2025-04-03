#include "qemu/osdep.h"
#include "qiof_disk.h"
#include "qiof_thread.h"
#include "qiof_log.h"
#include "qiof_heap.h"
#include "qiof_img.h"
#include "qiof_bitmap.h"
#include "block/block_int-common.h"
#include "block/qcow2.h"

#include "util/qemu-thread-common.h"
#include "qemu/typedefs.h"

#define IOS_POOL_BYTE_SIZE 500 * 1024 * 1024
static QIOF_DiskIO *sync_io = NULL;
static int snapshot_num = 0;
void (*QIOF_qmp_blockdev_snapshot_sync)(const char *, const char *, const char *,
										const char *, const char *, bool, int, Error **errp) = NULL;
void *snapshot_fun_addr = NULL;
static QTAILQ_HEAD(, QIOF_DiskIO) qiof_ios_queue = QTAILQ_HEAD_INITIALIZER(qiof_ios_queue); // write aios tail qeueue pool
static QTAILQ_HEAD(, QIOF_DiskIO) qiof_bitmap_data_queue = QTAILQ_HEAD_INITIALIZER(qiof_bitmap_data_queue);
char *soname = NULL;
// static bool qiof_bitmap_created = false;
static bool read_from_bitmap_queue = false;
static void *pdlHandle = NULL;
bool is_syn = false;
// 是否开启框架
bool enable_qiof = false;
// 是否初始化
bool is_init = false;
// 是否加载so
static bool isLoadSo = false;
// 自定义的io内存池
static PMEMORYPOOL io_pool = NULL;
static void *pBuf = NULL;
// 磁盘信息
static QIOF_DiskInfo *qdisk = NULL;
static QIOF_snapshot_info *qsnapshot = NULL;
static qiof_thread tid_syn;
// static qiof_thread tid_read_from_bitmap;
void async_QIOF_DiskIO_submit(const BlockDriverState *bs, QIOF_IO_type type, time_t timestamp,
							  int64_t sector_num, const QEMUIOVector *qiov, int nb_sectors, QIOF_DiskCallback *cb, void *opaque);
void sync_QIOF_DiskIO_submit(const BlockDriverState *bs, QIOF_IO_type type, time_t timestamp,
							 int64_t sector_num, const QEMUIOVector *qiov, int nb_sectors, QIOF_DiskCallback *cb, void *opaque);
static QIOF_Status init_pool(void);

void QIOF_DiskIO_delete(QIOF_DiskIO *io);
void QIOF_DiskIO_complete(QIOF_DiskCB *qcb);
QIOF_Status QIOF_DiskIO_find(QIOF_DiskIO *io);
void QIOF_DiskIOContinue(QIOF_DiskIO *io);
void *QIOF_first_Synchronize(void *arg);
void *QIOF_read_from_bitmap(void *arg);
QIOF_Status QIOF_init(const BlockDriverState *bs, const char *device);
void *qiov_to_linearBuf(const QEMUIOVector *qiov, int64_t bytes);
QEMUIOVector *QIOF_linearBuf_to_qiov(void *buf, int64_t bytes);
/************************************************************************/
/* 打快照（外部快照） */
/************************************************************************/
void QIOF_snapshot(void)
{
	// QIOF_img_snapshot(qsnapshot->device_name,snapshot_fun_addr);
	Error *err = NULL;
	char out_file[100];
	sprintf(out_file, "snapshot%d.img", snapshot_num++);
	QIOF_qmp_blockdev_snapshot_sync = snapshot_fun_addr;
	QIOF_qmp_blockdev_snapshot_sync(qsnapshot->device_name,
									NULL, out_file, NULL, NULL,
									true, 1, &err);
}
/************************************************************************/
/* 首次同步*/
/************************************************************************/
void *QIOF_first_Synchronize(void *arg)
{
	char *target = (char *)"/mirror.img";
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("getcwd");
		return NULL;
	}
	strcat(cwd, target);
	QIOF_LOG("Current working directory: %s\n", cwd);
	char *argv[] = {qdisk->name, cwd};
	QIOF_LOG("start Syncing......\n");
	int ret = QIOF_img_cp(qdisk->total_sectors, argv);
	if (ret != 0)
	{
		QIOF_ERR("Synchronize failed\n");
		return NULL;
	}
	QIOF_LOG("end Syncing......\n");
	qdisk->mirror = cwd;
	return NULL;
}
/************************************************************************/
/* 初始化存储池*/
/************************************************************************/
QIOF_Status init_pool(void)
{
	size_t sBufSize = IOS_POOL_BYTE_SIZE;
	pBuf = (void *)malloc(sBufSize);
	if (pBuf == NULL)
	{
		QIOF_ERR("malloc failed");
		return QIOF_NO_MEMORY;
	}
	memset(pBuf, 0, sBufSize);
	io_pool = CreateMemoryPool(pBuf, sBufSize);
	return QIOF_SUCCESS;
}
/************************************************************************/
/* 加载三方so*/
/************************************************************************/
void load_lib(void);
void load_lib(void)
{
	// load lib
	QIOF_LOG("loading so...\n");
	pdlHandle = dlopen(soname, RTLD_LAZY);
	if (pdlHandle == NULL)
	{
		QIOF_ERR("Failed load library\n");
		return;
	}
	char *pszErr = dlerror();
	if (pszErr != NULL)
	{
		QIOF_ERR("%s\n", pszErr);
		return;
	}
	// function ptr
	void (*pRegister)(void *, QIOF_Disk_CBType);
	pRegister = dlsym(pdlHandle, "register_QIOF");
	pszErr = dlerror();
	if (pszErr != NULL)
	{
		QIOF_ERR("%s\n", pszErr);
		return;
	}
	// register func and var ptr to 3th party so
	pRegister((void *)QIOF_Disk_submit, QIOF_Disk_SUBMIT);
	pRegister((void *)QIOF_Disk_abort, QIOF_Disk_ABORT);
	pRegister((void *)QIOF_getDiskIO, QIOF_Disk_GETDISKIO);
	pRegister((void *)QIOF_get_DiskInfo, QIOF_GET_DISKINFO);
	pRegister((void *)QIOF_snapshot, QIOF_SNAPSHOT);
	pRegister((void *)QIOF_getMirrorFile, QIOF_GET_MIRROR_FILE);
	if (!is_syn)
	{
		void *(*pMain)(void *arg);
		pMain = dlsym(pdlHandle, "mymain");
		pszErr = dlerror();
		if (pszErr != NULL)
		{
			QIOF_ERR("%s\n", pszErr);
			return;
		}
		qiof_thread tid;
		int ret = qiof_thread_create(&tid, pMain, NULL);
		if (ret != 0)
		{
			QIOF_ERR("thread create fail\n");
			return;
		}
		qiof_thread_detach(tid);
	}
	isLoadSo = true;
}
/************************************************************************/
/* 卸载三方so*/
/************************************************************************/
void unload_lib(void);
__attribute__((destructor)) void unload_lib(void)
{
	// unload lib
	if (isLoadSo)
	{
		QIOF_LOG("unloading so...\n");
		while (!QTAILQ_EMPTY(&qiof_ios_queue) && !QTAILQ_EMPTY(&qiof_bitmap_data_queue))
			;
		free(pBuf);
		dlclose(pdlHandle);
		char *pszErr = dlerror();
		if (pszErr != NULL)
		{
			QIOF_ERR("%s\n", pszErr);
			return;
		}
	}
}

/************************************************************************/
/* 删除一个磁盘IO对象*/
/************************************************************************/
void QIOF_DiskIO_delete(QIOF_DiskIO *io)
{
	if (io == NULL)
	{
		return;
	}
	if (is_syn)
	{
		if (sync_io->buf)
			free(sync_io->buf);
		free(sync_io);
		sync_io = NULL;
	}
	else
	{
		QIOF_DiskIO *tmp = NULL;
		bool isFind = false;
		if (read_from_bitmap_queue)
		{
			QTAILQ_FOREACH(tmp, &qiof_bitmap_data_queue, DiskIO_link)
			{
				if (tmp == io)
				{
					isFind = true;
					break;
				}
			}
			if (isFind)
			{
				QTAILQ_REMOVE(&qiof_bitmap_data_queue, io, DiskIO_link);
				if (io->buf)
					free(io->buf);
				free(io);
				io = NULL;
			}
		}
		else
		{
			QTAILQ_FOREACH(tmp, &qiof_ios_queue, DiskIO_link)
			{
				if (tmp == io)
				{
					isFind = true;
					break;
				}
			}
			if (isFind)
			{
				QTAILQ_REMOVE(&qiof_ios_queue, io, DiskIO_link);
				if (io->buf)
					FreeMemory(io->buf, io_pool);
				free(io);
				io = NULL;
			}
		}
	}
}
/************************************************************************/
/* 调用三方so的回调函数*/
/************************************************************************/
void QIOF_DiskIO_complete(QIOF_DiskCB *qcb)
{
	assert(qcb);
	if (qcb->cb)
		qcb->cb(qcb->opaque, qcb->status);
	free(qcb);
}
/************************************************************************/
/* 查询队列中是否有相同chunk*/
/************************************************************************/
QIOF_Status QIOF_DiskIO_find(QIOF_DiskIO *io)
{ // if it is the same block
	assert(io);
	QIOF_DiskIO *tmp = NULL;
	QTAILQ_FOREACH(tmp, &qiof_ios_queue, DiskIO_link)
	{
		if (tmp->fd == io->fd && tmp->offset == io->offset && tmp->bytes == io->bytes)
		{
			if (memcmp(tmp->buf, io->buf, io->bytes) == 0)
				return QIOF_SUCCESS;
		}
	}
	return QIOF_NOT_FOUND;
}
/************************************************************************/
/* 将所有非连续的qemu向量数据块（一般为4kb）合成连续内存数据*/
/************************************************************************/
void *qiov_to_linearBuf(const QEMUIOVector *qiov, int64_t bytes)
{
	assert(bytes % 512 == 0);
	void *buf = NULL;
	if (is_syn)
	{
		buf = (void *)malloc(bytes);
		if (buf == NULL)
		{
			QIOF_ERR("malloc failed\n");
			return NULL;
		}
		memset(buf, 0, bytes);
	}
	else
	{
		buf = (void *)GetMemory(bytes, io_pool);
		if (buf == NULL)
		{
			QIOF_LOG("mem size is %lu, used size is %lu\n", io_pool->size, io_pool->mem_used_size);
			QIOF_ERR("alloc %llu mem from io pool failed\n", bytes);
			return NULL;
		}
		memset(buf, 0, bytes); // copy qemu io vector to another linear buf
	}
	void *p = buf;
	for (int i = 0; i < qiov->niov; i++)
	{
		memmove(p, qiov->iov[i].iov_base, qiov->iov[i].iov_len);
		p += qiov->iov[i].iov_len;
	}
	return buf;
}
/************************************************************************/
/* 将连续的内存数据拆分成4096B大小的数据块*/
/************************************************************************/
QEMUIOVector *QIOF_linearBuf_to_qiov(void *buf, int64_t bytes)
{
	assert(bytes % 4096 == 0);
	QEMUIOVector *qiov = (QEMUIOVector *)malloc(sizeof(QEMUIOVector));
	qiov->size = bytes;
	qiov->nalloc = -1;
	qiov->niov = bytes / 4096;
	qiov->iov = NULL;
	struct iovec *iov = (struct iovec *)malloc(sizeof(struct iovec) * qiov->niov);
	struct iovec *piov = iov;
	void *p = buf;
	for (int i = 0; i < qiov->niov; i++)
	{
		piov->iov_base = p;
		piov->iov_len = 4096;
		piov++;
		p += 4096;
	}
	qiov->iov = iov;
	return NULL;
}
/************************************************************************/
/* 初始化框架 */
/************************************************************************/
QIOF_Status QIOF_init(const BlockDriverState *bs, const char *device)
{
	qdisk = (QIOF_DiskInfo *)malloc(sizeof(QIOF_DiskInfo));
	if (qdisk == NULL)
	{
		QIOF_ERR("malloc failed\n");
		return QIOF_NO_MEMORY;
	}
	memset(qdisk, 0, sizeof(QIOF_DiskInfo));
	qdisk->name = (char *)bs->filename;
	// brief 此处的total_sectors是实际上占用的物理空间 全部虚拟空间为bs->total_sectors
	// qdisk->total_sectors=bs->file->bs->total_sectors;
	qdisk->total_sectors = bs->total_sectors;
	QIOF_LOG("%s's size is %lu\n", qdisk->name, qdisk->total_sectors);
	qsnapshot = (QIOF_snapshot_info *)malloc(sizeof(QIOF_snapshot_info));
	if (qsnapshot == NULL)
	{
		free(qdisk);
		qdisk = NULL;
		QIOF_ERR("malloc failed\n");
		return QIOF_NO_MEMORY;
	}
	memset(qsnapshot, 0, sizeof(QIOF_snapshot_info));
	QIOF_blk *blk = (QIOF_blk *)malloc(sizeof(QIOF_blk));
	if (blk == NULL)
	{
		free(qdisk);
		qdisk = NULL;
		qsnapshot = NULL;
		QIOF_ERR("malloc failed\n");
		return QIOF_NO_MEMORY;
	}
	memset(blk, 0, sizeof(QIOF_blk));

	blk->name = g_strdup(device);
	blk->refcnt = 1;
	qsnapshot->device_name = blk->name;
	qsnapshot->format = (char *)bs->drv->format_name;
	QIOF_LOG("device name is %s,format is %s\n", qsnapshot->device_name, qsnapshot->format);

	init_pool();
	if (strcmp(bs->file->bs->drv->format_name, "file") == 0)
	{
		QIOF_snapshot();
		sleep(3);
		int ret = qiof_thread_create(&tid_syn, QIOF_first_Synchronize, NULL);
		if (ret != 0)
		{
			free(qdisk);
			free(qsnapshot);
			free(blk);
			qdisk = NULL;
			qsnapshot = NULL;
			blk = NULL;
			QIOF_ERR("thread create fail\n");
			return QIOF_THREAD_FAILURE;
		}
	}
	return QIOF_SUCCESS;
}

/************************************************************************/
/* (异步)blk提交IO数据给框架*/
/************************************************************************/

void async_QIOF_DiskIO_submit(const BlockDriverState *bs, QIOF_IO_type type, time_t timestamp,
							  int64_t sector_num, const QEMUIOVector *qiov, int nb_sectors, QIOF_DiskCallback *cb, void *opaque)
{
	assert(bs);
	assert(type == QIOF_WRITEV);

	QIOF_DiskIO *io = g_new0(QIOF_DiskIO, 1);
	if (io == NULL)
	{
		return;
	}
	io->bytes = nb_sectors << BDRV_SECTOR_BITS;
	io->type = type;
	io->timestamp = timestamp;
	io->offset = sector_num << BDRV_SECTOR_BITS;
	io->buf = qiov_to_linearBuf(qiov, io->bytes);
	int cnt = 0;
	if (io->buf == NULL)
	{
		cnt++;
		QIOF_LOG("%d remalloc...\n", cnt);
		sleep(1);
		io->buf = qiov_to_linearBuf(qiov, io->bytes);
	}
	if (cnt == 3 && io->buf == NULL)
	{
		QIOF_LOG("pool is full\n");
		// return QIOF_FAILURE;
	}
	if (strcmp(bs->file->bs->drv->format_name, "file") == 0)
	{ // local file
		int *fd = (int *)bs->file->bs->opaque;
		io->fd = *fd;
	}
	else
	{ // network file , for example:rbd
		io->fd = -1;
	}
	QTAILQ_INSERT_TAIL(&qiof_ios_queue, io, DiskIO_link);
	QIOF_DiskCB *qcb = (QIOF_DiskCB *)malloc(sizeof(QIOF_DiskCB));
	if (NULL == qcb)
	{
		QIOF_ERR("malloc failed\n");
		return;
	}
	memset(qcb, 0, sizeof(QIOF_DiskCB));
	qcb->status = QIOF_SUCCESS; // other
	qcb->cb = cb;
	qcb->opaque = opaque;
	QIOF_DiskIO_complete(qcb);
}

/************************************************************************/
/* (同步)blk提交IO数据给框架*/
/************************************************************************/
void sync_QIOF_DiskIO_submit(const BlockDriverState *bs, QIOF_IO_type type, time_t timestamp,
							 int64_t sector_num, const QEMUIOVector *qiov, int nb_sectors, QIOF_DiskCallback *cb, void *opaque)
{
	assert(bs);
	assert(type == QIOF_WRITEV);
	sync_io = g_new0(QIOF_DiskIO, 1);
	if (sync_io == NULL)
	{
		return;
	}
	sync_io->bytes = nb_sectors << BDRV_SECTOR_BITS;
	sync_io->type = type;
	sync_io->timestamp = timestamp;
	sync_io->offset = sector_num << BDRV_SECTOR_BITS;
	sync_io->buf = qiov_to_linearBuf(qiov, sync_io->bytes);
	if (sync_io->buf == NULL)
	{
		QIOF_DiskIO_delete(sync_io);
		return;
	}
	if (strcmp(bs->file->bs->drv->format_name, "file") == 0)
	{ // local file
		int *fd = (int *)bs->file->bs->opaque;
		sync_io->fd = *fd;
	}
	else
	{ // network file , for example:rbd
		sync_io->fd = -1;
	}
	void (*pMain)(void);
	pMain = dlsym(pdlHandle, "sync_mymain");
	char *pszErr = dlerror();
	if (pszErr != NULL)
	{
		QIOF_ERR("%s\n", pszErr);
		return;
	}
	pMain();
	QIOF_DiskCB *qcb = (QIOF_DiskCB *)malloc(sizeof(QIOF_DiskCB));
	if (NULL == qcb)
	{
		QIOF_ERR("malloc failed\n");
		return;
	}
	memset(qcb, 0, sizeof(QIOF_DiskCB));
	qcb->status = QIOF_SUCCESS; // other
	qcb->cb = cb;
	qcb->opaque = opaque;
	QIOF_DiskIO_complete(qcb);
}

void QIOF_DiskIOContinue(QIOF_DiskIO *io)
{
}
/************************************************************************/
/* so执行完毕向框架提交IO*/
/************************************************************************/
QIOF_Status QIOF_Disk_submit(QIOF_DiskIO *io)
{
	assert(io);
	QIOF_DiskIO_delete(io);
	return QIOF_SUCCESS;
}
QIOF_Status QIOF_Disk_abort(QIOF_DiskIO *io)
{
	assert(io);
	/****
	 *
	 *
	 */
	return QIOF_IO_ABORTED;
}
/************************************************************************/
/* 获取磁盘IO*/
/************************************************************************/
QIOF_DiskIO *QIOF_getDiskIO(void)
{
	if (is_syn)
	{

		return sync_io;
	}
	else
	{
		QIOF_DiskIO *io = NULL;
		if (read_from_bitmap_queue)
		{
			if (QTAILQ_EMPTY(&qiof_bitmap_data_queue))
			{
				QIOF_ERR("bitmap data queue is empty");
				read_from_bitmap_queue = false;
				io = QTAILQ_FIRST(&qiof_ios_queue);
			}
			else
				io = QTAILQ_FIRST(&qiof_bitmap_data_queue);
		}
		else
			io = QTAILQ_FIRST(&qiof_ios_queue);
		return io;
	}
}
/************************************************************************/
/* 获取队列尾*/
/************************************************************************/
QIOF_DiskIO *QIOF_getPollTail(void)
{
	assert(!QTAILQ_EMPTY(&qiof_ios_queue));
	QIOF_DiskIO *io = QTAILQ_LAST(&qiof_ios_queue);
	return io;
}
/************************************************************************/
/* 获取磁盘信息*/
/************************************************************************/
QIOF_DiskInfo *QIOF_get_DiskInfo(void)
{
	assert(qdisk);
	return qdisk;
}
/************************************************************************/
/* 获取同步文件*/
/************************************************************************/
char *QIOF_getMirrorFile(void)
{
	while (!qdisk->mirror)
		;
	return qdisk->mirror;
}
int qiof_test(void)
{
	QIOF_LOG("qiof test\n");
	return 0;
}
