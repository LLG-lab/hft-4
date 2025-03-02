#include "timestamp.h"

#include <time.h> 

timestamp_t mk_timestamp(int day, int mon, int year, int hour, int minute, int sec, int microsecond)
{
    struct tm tm;
    timestamp_t ts;

    tm.tm_year  = year - 1900;
    tm.tm_mon   = mon - 1;
    tm.tm_mday  = day;
    tm.tm_hour  = hour;
    tm.tm_min   = minute;
    tm.tm_sec   = sec;
    tm.tm_isdst = 0;

    ts = mktime(&tm);
    ts *= 1000;
    ts += (microsecond / 1000);

    return ts;
}
