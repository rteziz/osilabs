#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "uthread.h"

static int order[6];
static int position;

static void *record(void *arg){
    int id = *(int *)arg;
    for (int i = 0; i < 3; ++i) {
        order[position++] = id;
        assert(uthread_yield() == 0);
    }
    return NULL;
}

static void *increment(void *arg){
    ++*(int *)arg;
    return NULL;
}

static void *cancelable(void *arg){
    for (;;) {
        uthread_testcancel(arg);
        assert(uthread_yield() == 0);
    }
    return NULL;
}

static void *check_worker_join(void *arg){
    assert(uthread_join(arg) == -1 && errno == EDEADLK);
    return NULL;
}

int main(void){
    uthread a = {0}, b = {0};
    int one = 1, two = 2, value = 0;
    assert(uthread_create(NULL, increment, &value) == -1 && errno == EINVAL);
    assert(uthread_create(&a, NULL, NULL) == -1 && errno == EINVAL);
    assert(uthread_join(&a) == -1 && errno == EINVAL);
    assert(uthread_cancel(&a) == -1 && errno == ESRCH);
    assert(uthread_finished(NULL) == -1 && errno == EINVAL);
    assert(uthread_yield() == 0);

    assert(uthread_create(&a, record, &one) == 0);
    assert(uthread_create(&a, record, &one) == -1 && errno == EBUSY);
    assert(uthread_create(&b, record, &two) == 0);
    assert(position == 0);
    assert(uthread_join(&a) == 0);
    assert(uthread_join(&b) == 0);
    assert(position == 6);
    for (int i = 0; i < 6; ++i)
        assert(order[i] == i % 2 + 1);
    assert(a.stack == NULL && b.stack == NULL);
    assert(uthread_join(&a) == -1 && errno == EINVAL);

    assert(uthread_create(&a, cancelable, &a) == 0);
    assert(uthread_yield() == 0);
    assert(uthread_cancel(&a) == 0);
    assert(uthread_join(&a) == 0);
    assert(a.canceled && a.done && a.released);

    assert(uthread_create(&a, cancelable, &a) == 0);
    assert(uthread_cancel(&a) == 0);
    assert(uthread_detach(&a) == 0);
    assert(uthread_yield() == 0);
    assert(a.canceled && a.released);

    assert(uthread_create(&a, increment, &value) == 0);
    assert(uthread_detach(&a) == 0);
    assert(uthread_detach(&a) == -1 && errno == EINVAL);
    assert(uthread_join(&a) == -1 && errno == EINVAL);
    assert(!uthread_finished(&a));
    assert(uthread_yield() == 0);
    assert(value == 1 && uthread_finished(&a) == 1 && !a.stack);

    assert(uthread_create(&a, increment, &value) == 0);
    assert(uthread_yield() == 0);
    assert(a.done && !a.released);
    assert(uthread_cancel(&a) == -1 && errno == ESRCH);
    assert(uthread_detach(&a) == 0);
    assert(value == 2 && a.released);

    assert(uthread_create(&a, check_worker_join, &a) == 0);
    assert(uthread_join(&a) == 0);
    for (int i = 0; i < 100; ++i) {
        assert(uthread_create(&a, increment, &value) == 0);
        assert(uthread_join(&a) == 0);
    }
    assert(value == 102);
    puts("All uthread tests passed");
    return 0;
}
