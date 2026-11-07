#define _XOPEN_SOURCE 600
#include "uthread.h"

#include <errno.h>
#include <stdlib.h>

#define STACK_SIZE (1024 * 1024)

static ucontext_t main_context;
static uthread_t current;
static uthread_t ready_head;
static uthread_t ready_tail;

static int fail(int error){
    errno = error;
    return -1;
}

static void enqueue(uthread_t thread){
    thread->next = NULL;
    if (ready_tail)
        ready_tail->next = thread;
    else
        ready_head = thread;
    ready_tail = thread;
}

static void release_stack(uthread_t thread){
    free(thread->stack);
    thread->stack = NULL;
    thread->released = 1;
}

static void run_thread(void){
    current->start_routine(current->arg);
    current->done = 1;
}

int uthread_create(uthread_t thread, void *(*start_routine)(void *), void *arg){
    if (current)
        return fail(EPERM);
    if (!thread || !start_routine)
        return fail(EINVAL);
    if (thread->stack)
        return fail(EBUSY);

    void *stack = malloc(STACK_SIZE);
    if (!stack)
        return fail(ENOMEM);
    if (getcontext(&thread->context) == -1) {
        int error = errno;
        free(stack);
        return fail(error);
    }

    thread->stack = stack;
    thread->start_routine = start_routine;
    thread->arg = arg;
    thread->done = 0;
    thread->detached = 0;
    thread->cancel_requested = 0;
    thread->canceled = 0;
    thread->released = 0;
    thread->context.uc_stack.ss_sp = stack;
    thread->context.uc_stack.ss_size = STACK_SIZE;
    thread->context.uc_stack.ss_flags = 0;
    thread->context.uc_link = &main_context;
    makecontext(&thread->context, run_thread, 0);
    enqueue(thread);
    return 0;
}

int uthread_yield(void){
    if (current)
        return swapcontext(&current->context, &main_context);
    if (!ready_head)
        return 0;

    uthread_t thread = ready_head;
    ready_head = thread->next;
    if (!ready_head)
        ready_tail = NULL;
    thread->next = NULL;
    current = thread;
    int result = swapcontext(&main_context, &thread->context);
    int error = errno;
    current = NULL;

    if (!thread->done)
        enqueue(thread);
    else if (thread->detached)
        release_stack(thread);
    if (result == -1)
        return fail(error);
    return 0;
}

int uthread_join(uthread_t thread){
    if (current)
        return fail(EDEADLK);
    if (!thread || !thread->stack || thread->detached)
        return fail(EINVAL);
    while (!thread->done) {
        if (uthread_yield() == -1)
            return -1;
    }
    release_stack(thread);
    return 0;
}

int uthread_detach(uthread_t thread){
    if (current)
        return fail(EPERM);
    if (!thread || !thread->stack || thread->detached)
        return fail(EINVAL);
    thread->detached = 1;
    if (thread->done)
        release_stack(thread);
    return 0;
}

int uthread_cancel(uthread_t thread){
    if (current)
        return fail(EPERM);
    if (!thread)
        return fail(EINVAL);
    if (!thread->stack || thread->done)
        return fail(ESRCH);
    thread->cancel_requested = 1;
    return 0;
}

void uthread_testcancel(uthread_t self){
    if (!self || self != current || !self->cancel_requested)
        return;
    self->canceled = 1;
    self->done = 1;
    if (setcontext(&main_context) == -1)
        abort();
}

int uthread_finished(uthread_t thread){
    if (!thread)
        return fail(EINVAL);
    return thread->released;
}
