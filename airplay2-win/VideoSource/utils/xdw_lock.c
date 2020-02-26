#include <threads.h>
#include <stdlib.h>
#include "xdw_lock.h"
// On sucess, returns 0.
void *xdw_mutex_init(void)
{
#if 0
	pthread_mutex_t *hmutex = malloc(sizeof(pthread_mutex_t));
	if(pthread_mutex_init(hmutex, NULL))
	{
		free(hmutex);
		return NULL;
	}
	return (void *)hmutex;
#else
	mutex_handle_t *hmutex;// = malloc(sizeof(mutex_handle_t));
	MUTEX_CREATE(hmutex);
	return (void *)hmutex;

#endif
}

// On sucess, returns 0.
int xdw_mutex_lock(void *hmutex)
{
#if 0
	return pthread_mutex_lock((pthread_mutex_t *)hmutex);
#else
    MUTEX_LOCK((mutex_handle_t *)hmutex);
#endif
}


// On sucess, returns 0.
int xdw_mutex_unlock(void *hmutex)
{
#if 0
	return pthread_mutex_unlock((pthread_mutex_t *)hmutex);
#else
    MUTEX_UNLOCK((mutex_handle_t *)hmutex);
#endif
}


void xdw_mutex_uninit(void *hmutex)
{
#if 0
	pthread_mutex_destroy((pthread_mutex_t *)hmutex);
	free(hmutex);
#else
	MUTEX_DESTROY((mutex_handle_t *)hmutex);
	//free(hmutex);
#endif
}