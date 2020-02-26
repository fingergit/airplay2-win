#ifndef __XDW_LOCK_H__
#define __XDW_LOCK_H__

// On sucess, returns a mutex lock.
void *xdw_mutex_init(void);

// On sucess, returns 0.
int xdw_mutex_lock(void *hmutex);


// On sucess, returns 0.
int xdw_mutex_unlock(void *hmutex);

void xdw_mutex_uninit(void *hmutex);


#endif // __XDW_LOCK_H__
