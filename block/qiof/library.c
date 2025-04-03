#include "library.h"
void register_QIOF(void *callback, QIOF_Disk_CBType type);
void register_QIOF(void *callback, QIOF_Disk_CBType type)
{
  switch (type)
  {
  case QIOF_Disk_SUBMIT:
    callback_QIOF_DiskIO_submit = callback;
    break;
  case QIOF_Disk_ABORT:
    callback_QIOF_DiskIO_abort = callback;
    break;
  case QIOF_Disk_GETDISKIO:
    callback_QIOF_getDiskIO = callback;
    break;
  case QIOF_GET_DISKINFO:
    callback_QIOF_get_DiskInfo = callback;
    break;
  case QIOF_SNAPSHOT:
    callback_QIOF_snapshot = callback;
    break;
  case QIOF_GET_MIRROR_FILE:
    callback_QIOF_getMirrorFile = callback;
  default:
    break;
  }
}
QIOF_Status replication(const char *path, QIOF_DiskIO *qio);
QIOF_Status replication(const char *path, QIOF_DiskIO *qio)
{
  assert(qio);
  assert(qio->type == QIOF_WRITEV);
  if (fd == 0)
    fd = open(path, O_RDWR);
  if (fd < 0)
  {
    QIOF_ERR("open file fail\n");
    fd = 0;
    return QIOF_FAILURE;
  }
  char timestamp[TIME_LEN];
  strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", localtime(&qio->timestamp));
  QIOF_LOG("%s", timestamp);
  QIOF_LOG("fd is %d, buf is %p, offset is 0x%lu, bytes is %lu\t", qio->fd, qio->buf, qio->offset, qio->bytes);
  ssize_t offset = 0, len;
  while (offset < qio->bytes)
  {
    len = pwrite(fd, qio->buf + offset, qio->bytes - offset, qio->offset + offset);
    QIOF_LOG("return bytes is %lu\n", len);
    if (len < 0)
    {
      QIOF_ERR("write file fail\n");
      char *errstr = strerror(len);
      QIOF_ERR("error : %s\n", errstr);
      return QIOF_FAILURE;
    }
    offset += len;
  }
  return QIOF_SUCCESS;
}
void callback_test(void *opaque, QIOF_Status status);
void callback_test(void *opaque, QIOF_Status status)
{
  //  printf("operate successfully\n");
}
void sync_mymain(void);
void sync_mymain(void)
{
  if (mirror_path == NULL)
  {
    mirror_path = callback_QIOF_getMirrorFile();
    QIOF_LOG("mirror path is %s\n", mirror_path);
  }
  QIOF_DiskIO *qio = callback_QIOF_getDiskIO();
  if (qio)
  {
    replication(mirror_path, qio);
    callback_QIOF_DiskIO_submit(qio);
  }
}
void *mymain(void *arg);
void *mymain(void *arg)
{
  while (1)
  {
    usleep(100000);
    if (mirror_path == NULL)
    {
      mirror_path = callback_QIOF_getMirrorFile();
      QIOF_LOG("mirror path is %s\n", mirror_path);
    }
    QIOF_DiskIO *qio = callback_QIOF_getDiskIO();
    if (qio)
    {
      replication(mirror_path, qio);
      callback_QIOF_DiskIO_submit(qio);
    }
  }
  return NULL;
}