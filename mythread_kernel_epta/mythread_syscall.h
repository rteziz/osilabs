#ifndef MYTHREAD_SYSCALL_H
#define MYTHREAD_SYSCALL_H

static inline long raw_syscall4(long nr, long a, long b, long c, long d)
{
#if defined(__aarch64__)
    register long x8 __asm__("x8") = nr;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    __asm__ volatile("svc 0" : "+r"(x0)
                     : "r"(x8), "r"(x1), "r"(x2), "r"(x3) : "memory", "cc");
    return x0;
#elif defined(__x86_64__)
    register long r10 __asm__("r10") = d;
    long result;
    __asm__ volatile("syscall" : "=a"(result)
                     : "a"(nr), "D"(a), "S"(b), "d"(c), "r"(r10)
                     : "rcx", "r11", "memory", "cc");
    return result;
#else
#error "mythread supports Linux on aarch64 and x86_64"
#endif
}
#endif
