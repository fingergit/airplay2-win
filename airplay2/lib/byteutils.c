//
// Created by Administrator on 2019/1/10/010.
//

#include <time.h>
#include "byteutils.h"
#ifdef WIN32
#include <Windows.h>
#endif // WIN32

int byteutils_get_int(unsigned char* b, int offset) {
    return ((b[offset + 3] & 0xff) << 24) | ((b[offset + 2] & 0xff) << 16) | ((b[offset + 1] & 0xff) << 8) | (b[offset] & 0xff);
}

short byteutils_get_short(unsigned char* b, int offset) {
    return (short) ((b[offset + 1] << 8) | (b[offset] & 0xff));
}

float byteutils_get_float(unsigned char* b, int offset) {
    //unsigned char tmp[4] = {b[offset + 3], b[offset + 2], b[offset + 1], b[offset]};
    return *((float *)(b + offset));
}


uint64_t byteutils_get_int2(unsigned char* b, int offset) {
    return ((uint64_t)(b[offset + 3] & 0xff) << 24) | ((uint64_t)(b[offset + 2] & 0xff) << 16) | ((uint64_t)(b[offset + 1] & 0xff) << 8) | ((uint64_t)b[offset] & 0xff);
}

uint64_t byteutils_get_long(unsigned char* b, int offset) {
    return (byteutils_get_int2(b, offset + 4)) << 32 | byteutils_get_int2(b, offset);
}

//s -> us
uint64_t ntptopts(uint64_t ntp) {
    return (((ntp >> 32) & 0xffffffff)* 1000000) + ((ntp & 0xffffffff) * 1000 * 1000 / INT_32_MAX) ;
}

uint64_t byteutils_read_int(unsigned char* b, int offset) {
    return ((uint64_t)b[offset]  << 24) | ((uint64_t)b[offset + 1]  << 16) | ((uint64_t)b[offset + 2] << 8) | ((uint64_t)b[offset + 3]  << 0);
}
//s->us
uint64_t byteutils_read_timeStamp(unsigned char* b, int offset) {
    return (byteutils_read_int(b, offset) * 1000000) + ((byteutils_read_int(b, offset + 4) * 1000000) / INT_32_MAX);
}
// us time to ntp
void byteutils_put_timeStamp(unsigned char* b, int offset, uint64_t time) {

    // time= ms
    uint64_t seconds = time / 1000000L;
    uint64_t microseconds = time - seconds * 1000000L;
    seconds += OFFSET_1900_TO_1970;

    // write seconds in big endian format
    b[offset++] = (uint8_t)(seconds >> 24);
    b[offset++] = (uint8_t)(seconds >> 16);
    b[offset++] = (uint8_t)(seconds >> 8);
    b[offset++] = (uint8_t)(seconds >> 0);

    uint64_t fraction = microseconds * 0x100000000L / 1000000L;
    // write fraction in big endian format
    b[offset++] = (uint8_t)(fraction >> 24);
    b[offset++] = (uint8_t)(fraction >> 16);
    b[offset++] = (uint8_t)(fraction >> 8);
    // low order bits should be random data
    b[offset++] = (uint8_t)(fraction >> 0);
    //b[offset++] = (Math.random() * 255.0);
}

#ifdef WIN32
// https://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows?r=SearchResults
#define CLOCK_PROCESS_CPUTIME_ID 0
LARGE_INTEGER
getFILETIMEoffset()
{
    SYSTEMTIME s;
    FILETIME f;
    LARGE_INTEGER t;

    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return (t);
}

int
clock_gettime(int X, struct timespec* tv)
{
    LARGE_INTEGER           t;
    FILETIME            f;
    double                  microseconds;
    static LARGE_INTEGER    offset;
    static double           frequencyToMicroseconds;
    static int              initialized = 0;
    static BOOL             usePerformanceCounter = 0;

    if (!initialized) {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter) {
            QueryPerformanceCounter(&offset);
            frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
        }
        else {
            offset = getFILETIMEoffset();
            frequencyToMicroseconds = 10.;
        }
    }
    if (usePerformanceCounter) QueryPerformanceCounter(&t);
    else {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double)t.QuadPart / frequencyToMicroseconds;
    t.QuadPart = (LONGLONG)microseconds;
    tv->tv_sec = (long)(t.QuadPart / 1000000);
    tv->tv_nsec = t.QuadPart % 1000000;
    return (0);
}
#endif // WIN32

uint64_t now_us() {
    struct timespec time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
    return (uint64_t)time.tv_sec * 10000000L + (uint64_t)(time.tv_nsec / 1000);
}
