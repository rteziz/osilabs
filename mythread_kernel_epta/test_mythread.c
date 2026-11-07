#define _POSIX_C_SOURCE 200809L
#include "mythread.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

static void *increment(void *arg){
    ++*(int *)arg;
    return arg;
}

static void *cancel_loop(void *arg){
    for (;;)
        mythread_testcancel(arg);
    return NULL;
}

static void *wait_gate(void *arg){
    while (!__atomic_load_n((int *)arg, __ATOMIC_ACQUIRE))
        ;
    return NULL;
}

static void wait_released(mythread_t thread){
    const struct timespec delay = {0, 1000000};
    int attempts = 5000;
    while (!mythread_finished(thread) && attempts-- > 0)
        nanosleep(&delay, NULL);
    assert(mythread_finished(thread) == 1);
    assert(thread->stack == NULL);
}

int main(){
    mythread thread = {0};
    int number = 0;
    void *result = &number;
    assert(mythread_detach(NULL) == -1 && errno == EINVAL);
    assert(mythread_cancel(NULL) == -1 && errno == EINVAL);
    assert(mythread_join(&thread, &result) == -1 && errno == EINVAL);
    assert(result == &number);
    assert(mythread_cancel(&thread) == -1 && errno == ESRCH);

    for (int i = 0; i < 100; ++i) {
        assert(mythread_create(&thread, increment, &number) == 0);
        assert(mythread_join(&thread, &result) == 0);
        assert(result == &number);
        assert(number == i + 1);
        assert(mythread_finished(&thread) == 1);
    }

    assert(mythread_create(&thread, cancel_loop, &thread) == 0);
    assert(mythread_cancel(&thread) == 0);
    assert(mythread_cancel(&thread) == 0 || errno == ESRCH);
    assert(mythread_join(&thread, &result) == 0);
    assert(result == MYTHREAD_CANCELED);
    assert(thread.canceled == 1);
    assert(mythread_cancel(&thread) == -1 && errno == ESRCH);

    int gate = 0;
    assert(mythread_create(&thread, wait_gate, &gate) == 0);
    assert(mythread_cancel(&thread) == 0);
    __atomic_store_n(&gate, 1, __ATOMIC_RELEASE);
    assert(mythread_join(&thread, &result) == 0);
    assert(result == NULL);
    assert(thread.canceled == 0);

    mythread slow = {0};
    gate = 0;
    assert(mythread_create(&slow, wait_gate, &gate) == 0);
    assert(mythread_detach(&slow) == 0);
    assert(mythread_join(&slow, NULL) == -1 && errno == EINVAL);
    assert(mythread_detach(&slow) == -1 && errno == EINVAL);
    assert(mythread_create(&slow, increment, &number) == -1 && errno == EBUSY);

    mythread batch[32] = {0};
    int values[32] = {0};
    for (int i = 0; i < 32; ++i) {
        assert(mythread_create(&batch[i], increment, &values[i]) == 0);
        assert(mythread_detach(&batch[i]) == 0);
    }
    for (int i = 0; i < 32; ++i) {
        wait_released(&batch[i]);
        assert(values[i] == 1);
    }
    assert(mythread_finished(&slow) == 0);
    __atomic_store_n(&gate, 1, __ATOMIC_RELEASE);
    wait_released(&slow);

    assert(mythread_create(&thread, cancel_loop, &thread) == 0);
    assert(mythread_detach(&thread) == 0);
    assert(mythread_cancel(&thread) == 0);
    wait_released(&thread);
    assert(thread.canceled == 1);

    for (int i = 0; i < 20; ++i) {
        assert(mythread_create(&thread, increment, &number) == 0);
        while (__atomic_load_n(&thread.tid, __ATOMIC_ACQUIRE) != 0)
            ;
        assert(mythread_detach(&thread) == 0);
        wait_released(&thread);
    }
    assert(mythread_create(&thread, increment, &number) == 0);
    assert(mythread_join(&thread, NULL) == 0);
    puts("All mythread tests passed");
    return 0;
}
