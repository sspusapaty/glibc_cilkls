#ifndef CILK_STRUCTS_H_
#define CILK_STRUCTS_H_

#include "pthreadP.h"

// Multicilk Functions
int (*is_cilk_thread)(void) = NULL;
pthread_mutex_t* (*get_cilkls_mutex)(void) = NULL;
pthread_t (*cilk_current)(void) = NULL;
extern void reg_is_cilk_thread(int (*func)(void));
extern void reg_cilk_current(pthread_t (*func)(void));
extern void reg_get_cilkls_mutex(pthread_mutex_t* (*func)(void));

#endif
