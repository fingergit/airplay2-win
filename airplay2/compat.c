#include "compat.h"

#ifdef WIN32
int gettimeofday(struct timeval* tv/*in*/, struct timezone* tz/*in*/)
{
    FILETIME ft;
    __int64 tmpres = 0;
    TIME_ZONE_INFORMATION tz_winapi;
    int rez = 0;

    ZeroMemory(&ft, sizeof(ft));
    ZeroMemory(&tz_winapi, sizeof(tz_winapi));

    GetSystemTimeAsFileTime(&ft);

    tmpres = ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    /*converting file time to unix epoch*/
    tmpres /= 10;  /*convert into microseconds*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
    tv->tv_sec = (__int32)(tmpres * 0.000001);
    tv->tv_usec = (tmpres % 1000000);


    //_tzset(),don't work properly, so we use GetTimeZoneInformation
    rez = GetTimeZoneInformation(&tz_winapi);
    if (tz != NULL) {
        tz->tz_dsttime = (rez == 2) ? TRUE : FALSE;
        tz->tz_minuteswest = tz_winapi.Bias + ((rez == 2) ? tz_winapi.DaylightBias : 0);
    }

    return 0;
}

int pthread_cond_timedwait(cond_handle_t* __cond, mutex_handle_t* __mutex, const struct timespec* __timeout) {
    if (__timeout != NULL) {
        struct timeval now;
        gettimeofday(&now, NULL);
        WaitForSingleObject(*__cond, (__timeout->tv_sec - now.tv_sec) * 1000 + __timeout->tv_nsec / 1000000);
    }
    else {
        WaitForSingleObject(*__cond, INFINITE);
    }
    return 0;
}

#endif // WIN32

