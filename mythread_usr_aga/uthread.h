#ifndef UTHREAD_H
#define UTHREAD_H

#include <ucontext.h>

typedef struct uthread {
    ucontext_t context;
    void *stack;
    void *(*start_routine)(void *);
    void *arg;
    int done;
    int detached;
    int cancel_requested;
    int canceled;
    int released;
    struct uthread *next;
} uthread;

typedef uthread *uthread_t;

/* Before the first create: uthread thread = {0}; */
/* create/join/detach/cancel are called only from main. */
int uthread_create(uthread_t thread, void *(*start_routine)(void *), void *arg);
int uthread_join(uthread_t thread);
int uthread_detach(uthread_t thread);
int uthread_cancel(uthread_t thread);
void uthread_testcancel(uthread_t self);
int uthread_finished(uthread_t thread);
int uthread_yield(void);

#endif
