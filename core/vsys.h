#pragma once

#if defined(__WIN32__)
#include <Windows.h>
#else
#include <pthread.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#endif

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

/*
 * vlock
 */
struct vlock {
#if defined(__WIN32__)
    CRITICAL_SECTION critical_section;
#else
    pthread_mutex_t mutex;
#endif
};

#if defined(__WIN32__)
//todo:
#define VLOCK_INITIALIZER
#else
#define VLOCK_INITIALIZER {.mutex = PTHREAD_MUTEX_INITIALIZER }
#endif

extern int  vlock_init  (struct vlock*);
extern int  vlock_enter (struct vlock*);
extern int  vlock_leave (struct vlock*);
extern void vlock_deinit(struct vlock*);

/*
 * vcondition
 */
struct vcond {
#if defined(__WIN32__)
    int signal_times;
    HANDLE event;
#else
    int signal_times;
    pthread_cond_t cond;
#endif

};

#if defined(__WIN32__)
#define VCOND_INITIALIZER
#else
#define VCOND_INITIALIZER {.signal_times = 0, \
                           .cond = PTHREAD_COND_INITIALIZER }
#endif

extern int  vcond_init  (struct vcond*);
extern int  vcond_wait  (struct vcond*, struct vlock*);
extern int vcond_timedwait(struct vcond*, struct vlock*, int);
extern int  vcond_signal(struct vcond*);
extern void vcond_deinit(struct vcond*);

/*
 * vthread
 */

typedef int (*vthread_entry_t)(void*);
struct vthread {
#if defined (__WIN32__)
    HANDLE thread;
    DWORD tid;
#else
    pthread_mutex_t mutex;
    pthread_t thread;
#endif
    vthread_entry_t entry_cb;
    void* cookie;

    int started;
    int quit_code;
};

extern int  vthread_init  (struct vthread*, vthread_entry_t, void*);
extern int  vthread_start (struct vthread*);
extern int  vthread_detach(struct vthread*);
extern int  vthread_join  (struct vthread*, int*);
extern void vthread_deinit(struct vthread*);

/*
 * vtimer
 */

#if defined(__APPLE__)
    typedef int timer_t;
#endif

typedef int (*vtimer_cb_t)(void*);
struct vtimer {
    vtimer_cb_t cb;
    void* cookie;
    int once_flag;

#if defined(__WIN32__)
    UINT_PTR id;
#elif defined(__APPLE__)
    timer_t id;
    dispatch_source_t source;
#else
    timer_t id;
#endif
};

int  vtimer_init   (struct vtimer*, vtimer_cb_t, void*, int);
int  vtimer_start  (struct vtimer*, int, int);
int  vtimer_restart(struct vtimer*, int, int);
int  vtimer_stop   (struct vtimer*);
void vtimer_deinit (struct vtimer*);

