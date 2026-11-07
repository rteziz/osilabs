#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include "mythread.h"

static void *calculate(void *arg){
    int *number = arg;
    *number *= 2;
    return number;
}

static void *work_until_canceled(void *arg){
    mythread_t self = arg;
    for (;;)
        mythread_testcancel(self);
    return NULL;
}

int main(){
    mythread thread = {0};
    int number = 21;
    void *result = NULL;
    if (mythread_create(&thread, calculate, &number) == -1 ||
        mythread_join(&thread, &result) == -1) {
        perror("create/join");
        return 1;
    }
    printf("Результат работы потока: %d\n", *(int *)result);

    if (mythread_create(&thread, work_until_canceled, &thread) == -1 ||
        mythread_cancel(&thread) == -1 || mythread_join(&thread, NULL) == -1) {
        perror("cancel/join");
        return 1;
    }
    printf("Поток отменён: %s\n", thread.canceled ? "да" : "нет");

    number = 10;
    if (mythread_create(&thread, calculate, &number) == -1 ||
        mythread_detach(&thread) == -1) {
        perror("create/detach");
        return 1;
    }

    const struct timespec delay = {0, 1000000};
    while (!mythread_finished(&thread))
        nanosleep(&delay, NULL);
    printf("Detached-поток завершён, стек освобождён, результат: %d\n", number);
    return 0;
}
