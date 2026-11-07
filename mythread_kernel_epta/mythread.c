#define _GNU_SOURCE
#include "mythread.h"
#include "mythread_syscall.h"

#include <errno.h>
#include <linux/futex.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)
#define THREAD_FLAGS (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD)

static _Alignas(16) char reaper_stack[64 * 1024]; //адрес начала кратен (16)
static int reaper_started;
static int list_lock;
static mythread_t detached_list;

static void lock_list(void){
    while (__atomic_exchange_n(&list_lock, 1, __ATOMIC_ACQUIRE));
}

static void unlock_list(void){
    __atomic_store_n(&list_lock, 0, __ATOMIC_RELEASE);
}

static int reap_threads(void *unused){
    (void)unused;
    const struct timespec delay = {0, 10000000};
    for (;;) {
        lock_list();
        mythread_t *link = &detached_list;
        while (*link) {
            mythread_t thread = *link;
            if (__atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE) == 0 &&
                raw_syscall4(SYS_munmap, (long)thread->stack, STACK_SIZE, 0, 0) == 0) {
                *link = thread->next;
                thread->stack = NULL;
                __atomic_store_n(&thread->released, 1, __ATOMIC_RELEASE);
            } else {
                link = &thread->next;
            }
        }
        unlock_list();
        raw_syscall4(SYS_nanosleep, (long)&delay, 0, 0, 0);
    }
    return 0;
}

static int run_thread(void *arg){
    mythread_t thread = arg;
    thread->result = thread->start_routine(thread->arg);
    return 0;
}

int mythread_create(mythread_t thread, void *(*start_routine)(void *), void *arg){
    if (!thread || !start_routine) {
        errno = EINVAL;
        return -1;
    }

    if ((thread->detached && !__atomic_load_n(&thread->released, __ATOMIC_ACQUIRE)) ||
        thread->stack) {
        errno = EBUSY;
        return -1;
    }

    void *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED)
        return -1;

    thread->stack = stack;
    thread->start_routine = start_routine;
    thread->arg = arg;
    thread->result = NULL;
    thread->detached = 0;
    thread->cancel_requested = 0;
    thread->canceled = 0;
    thread->released = 0;
    thread->next = NULL;

    int flags = THREAD_FLAGS | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID;
    if (clone(run_thread, (char *)stack + STACK_SIZE, flags, thread,
              &thread->tid, NULL, &thread->tid) == -1) {
        int error = errno;
        munmap(stack, STACK_SIZE);
        thread->stack = NULL;
        errno = error;
        return -1;
    }
    return 0;
}

int mythread_join(mythread_t thread, void **retval){
    if (!thread || thread->detached || !thread->stack) {
        errno = EINVAL;
        return -1;
    }
    int tid;
    while ((tid = __atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE)) != 0) {
        long result = raw_syscall4(SYS_futex, (long)&thread->tid, FUTEX_WAIT, tid, 0);
        if (result < 0 && result != -EAGAIN && result != -EINTR) {
            errno = (int)-result;
            return -1;
        }
    }
    if (munmap(thread->stack, STACK_SIZE) == -1)
        return -1;
    thread->stack = NULL;
    if (retval)
        *retval = thread->canceled ? MYTHREAD_CANCELED : thread->result;
    __atomic_store_n(&thread->released, 1, __ATOMIC_RELEASE);
    return 0;
}

int mythread_detach(mythread_t thread){
    if (!thread || thread->detached || !thread->stack) {
        errno = EINVAL;
        return -1;
    }
    if (!reaper_started) {
        if (clone(reap_threads, reaper_stack + sizeof(reaper_stack),
                  THREAD_FLAGS, NULL) == -1)
            return -1;
        reaper_started = 1;
    }
    thread->detached = 1;
    lock_list();
    thread->next = detached_list;
    detached_list = thread;
    unlock_list();
    return 0;
}

int mythread_cancel(mythread_t thread){
    if (!thread) {
        errno = EINVAL;
        return -1;
    }
    if (__atomic_load_n(&thread->released, __ATOMIC_ACQUIRE) ||
        __atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE) == 0) {
        errno = ESRCH;
        return -1;
    }
    __atomic_store_n(&thread->cancel_requested, 1, __ATOMIC_RELEASE);
    return 0;
}

void mythread_testcancel(mythread_t self){
    if (__atomic_load_n(&self->cancel_requested, __ATOMIC_ACQUIRE)) {
        self->canceled = 1;
        raw_syscall4(SYS_exit, 0, 0, 0, 0);
        __builtin_unreachable();
    }
}

int mythread_finished(mythread_t thread){
    if (!thread) {
        errno = EINVAL;
        return -1;
    }
    return __atomic_load_n(&thread->released, __ATOMIC_ACQUIRE);
}
