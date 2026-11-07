#ifndef MYTHREAD_H
#define MYTHREAD_H

typedef struct mythread {
    int tid;
    void *stack;
    void *(*start_routine)(void *);
    void *arg;
    void *result;
    int detached;
    int cancel_requested;
    int canceled;
    int released;
    struct mythread *next;
} mythread;

typedef mythread *mythread_t;

int mythread_create(mythread_t thread, void *(*start_routine)(void *), void *arg);
#define MYTHREAD_CANCELED ((void *)-1)
/* retval может быть NULL, если нужен только факт завершения. */
int mythread_join(mythread_t thread, void **retval);
int mythread_detach(mythread_t thread);
int mythread_cancel(mythread_t thread);

void mythread_testcancel(mythread_t self);

int mythread_finished(mythread_t thread);

#endif
