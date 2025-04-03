#include "qiof_thread.h"
void qiof_thread_mutex_lock(qiof_thread_mutex *mutex)
{
    pthread_mutex_lock(mutex);
}
void qiof_thread_mutex_unlock(qiof_thread_mutex *mutex)
{
    pthread_mutex_unlock(mutex);
}
void qiof_thread_cond_signal(qiof_thread_cond *cond)
{
    pthread_cond_signal(cond);
}
void qiof_thread_cond_wait(qiof_thread_cond *cond, qiof_thread_mutex *mutex)
{
    pthread_cond_wait(cond, mutex);
}
void qiof_thread_mutex_init(qiof_thread_mutex *mutex)
{
    pthread_mutex_init(mutex, NULL);
}
void qiof_thread_cond_init(qiof_thread_cond *cond)
{
    pthread_cond_init(cond, NULL);
}
void qiof_thread_mutex_destroy(qiof_thread_mutex *mutex)
{
    pthread_mutex_destroy(mutex);
}
void qiof_thread_cond_destroy(qiof_thread_cond *cond)
{
    pthread_cond_destroy(cond);
}
int qiof_thread_create(qiof_thread *thread, qiof_thread_func *func, void *opaque)
{
    return pthread_create(thread, NULL, func, opaque);
}
void qiof_thread_detach(qiof_thread thread)
{
    pthread_detach(thread);
}
void qiof_thread_exit(void)
{
    pthread_exit(NULL);
}
int qiof_thread_join(qiof_thread *thread)
{
    int *thread_retval;
    return pthread_join(*thread, (void **)&thread_retval);
}