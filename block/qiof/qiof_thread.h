#ifndef QIOF_THREAD_H
#define QIOF_THREAD_H
#include<pthread.h>
typedef pthread_mutex_t qiof_thread_mutex;
typedef pthread_cond_t qiof_thread_cond;
typedef pthread_t qiof_thread;
typedef void* qiof_thread_func(void *opaque);
void qiof_thread_mutex_lock(qiof_thread_mutex *mutex);
void qiof_thread_mutex_unlock(qiof_thread_mutex *mutex);
void qiof_thread_cond_signal(qiof_thread_cond *cond);
void qiof_thread_cond_wait(qiof_thread_cond *cond,qiof_thread_mutex *mutex);
void qiof_thread_mutex_init(qiof_thread_mutex *mutex);
void qiof_thread_cond_init(qiof_thread_cond *cond);
void qiof_thread_mutex_destroy(qiof_thread_mutex *mutex);
void qiof_thread_cond_destroy(qiof_thread_cond *cond);
int qiof_thread_create(qiof_thread *thread,qiof_thread_func *func,void *opaque);
void qiof_thread_detach(qiof_thread thread);
void qiof_thread_exit(void);
int qiof_thread_join(qiof_thread *thread);
#endif