/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_TIME_H
#define CLIBS_TIME_H

#include "std/cstd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct STDate {
    Float32 zone;    /* time zone, -12 - +12 */
    UInt16 ms;    /* milliseconds, 0 - 999 */
    UInt8 sec;      /* seconds, 0 - 59 */
    UInt8 min;      /* minutes, 0 - 59 */
    UInt8 hour;     /* hours, 0 - 23 */
    UInt8 mday;     /* day of the month, 1 - 31(28,29,30) */
    UInt8 mon;      /* month, 1 - 12 */
    UInt16 year;    /* full year as "2007" */
    UInt8 wday;    /* day of week, 1 - 7 mon - sun */
    UInt16 yday;    /* day of year, 1 - 365(366) */
    UInt8 isdst;    /* is daylight saving time */
} CDateT;

//typedef struct STTime {
//    UInt32 sec;    /* seconds since GMT 1970-01-01 00:00:00 */
//    UInt16 ms;    /* milliseconds, 0 - 999*/
//} CTimeT;

typedef UInt64 CTimeT;

#define CTIME_SECOND(t)     ((t)/1000)
#define CTIME_MS(t)        ((t)%1000)

/** max number of ticks in milliseconds */
#define maxTicksNum ((UInt32)-1)

/***************************************************/
//cTimeset time api

/**
* set the local timezone, this will affect cTimeGetLocalDate() and mcTime2LocalDate()
* @param z: timezone, values of [-12 to +12]
*/
void cTimeSetTimeZone(Float32 z);

/**
* get the local timezone
*/
Float32 cTimeGetTimeZone(void);

/**
* get date of the specified timezone, if need GMT date, set param z to 0
*/
void cTimeGetDate(CDateT* d, Float32 z);

/**
* get local date
*/
void cTimeGetLocalDate(CDateT* d);

/**
* get the time in millseconds, since GMT 1970-01-01 00:00:00 000
*/
CTimeT cTimeGetTime(void);

CTimeT cTimeGetNanoTime(void);
/**
 * get ticks in milliseconds from system start up
 */
UInt32 cTimeTicks(void);

/**
* convert one date to another date of a specified timezone
* @param d1: source date
* @param d2: dist date
* @param z: dist timezone
*/
void cTimeDateConvert(CDateT* d1, CDateT* d2, Float32 z);

/**
* convert time to date of specified timezone, if need GMT date, set param z to 0
*/
void cTimeTime2Date(CTimeT t, CDateT* d, Float32 z);

/**
* convert time to local date
*/
void cTimeTime2LocalDate(CTimeT t, CDateT* d);

/**
* convert date to time
*/
CTimeT cTimeDate2Time(CDateT* d);


/******************************************/
//util time api
/**
 * is the year leap year
 */
bool cTimeIsLeapYear(UInt16 year);
/**
 * is the date valid
 */
bool cTimeIsDateValid(UInt16 year, UInt8 month, UInt8 mday);
/**
 * get days since 1970-1-1, begin from 0
 * for example:
 *     cTimeGetDays(1970,1,2)  returns 1
 */
UInt32 cTimeGetDays(UInt16 year, UInt8 month, UInt8 mday);
/**
 * get the day of the year
 * for example:
 *     cTimeGetYearDay(1970,1,1) returns 1
 */
UInt16 cTimeGetYearDay(UInt16 year, UInt8 month, UInt8 mday);
/**
 * get the week day
 *
 */
UInt8 cTimeGetWeekDay(UInt16 year, UInt8 month, UInt8 mday);
/**
 * get month and day of month, return value combines the month and mday,
 * month in the high byte while mday in the low byte
 */
UInt16 cTimeGetMonthAndDay(UInt16 year, UInt16 yday);

#ifdef __cplusplus
}
#endif
#endif

