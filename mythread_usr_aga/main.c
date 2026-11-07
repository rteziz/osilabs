#include <stdio.h>
#include <stdlib.h>
#include "uthread.h"

static void check(int result){
    if (result == -1) {
        perror("uthread");
        exit(EXIT_FAILURE);
    }
}

static void *count(void *arg){
    const char *name = arg;
    for (int i = 1; i <= 3; ++i) {
        printf("%s: %d\n", name, i);
        check(uthread_yield());
    }
    return NULL;
}

static void *work_until_canceled(void *arg){
    uthread_t self = arg;
    for (;;) {
        uthread_testcancel(self);
        puts("Работаю до отмены");
        check(uthread_yield());
    }
    return NULL;
}

static void *calculate(void *arg){
    int *number = arg;
    *number *= 2;
    return NULL;
}

int main(void){
    uthread first = {0}, second = {0};
    check(uthread_create(&first, count, "A"));
    check(uthread_create(&second, count, "B"));
    check(uthread_join(&first));
    check(uthread_join(&second));

    check(uthread_create(&first, work_until_canceled, &first));
    check(uthread_yield());
    check(uthread_cancel(&first));
    check(uthread_join(&first));
    printf("Поток отменён: %s\n", first.canceled ? "да" : "нет");

    int number = 21;
    check(uthread_create(&first, calculate, &number));
    check(uthread_detach(&first));
    while (!uthread_finished(&first))
        check(uthread_yield());
    printf("Стек освобождён, результат: %d\n", number);
    return 0;
}
