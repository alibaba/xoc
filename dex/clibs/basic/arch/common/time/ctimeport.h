/*Copyright 2011 Alibaba Group*/
/*
 * ctimeport.h
 *
 *  Created on: Apr 17, 2009
 *      Author: misa
 */

#ifndef CLIBS_TIME_PORT_H
#define CLIBS_TIME_PORT_H

#include "time/ctime.h"

//get time in milliseconds since GMT 1970-01-01 00:00:00 000
EXTERN_C CTimeT cTimeGetTimePort(void);

EXTERN_C CTimeT cTimeGetNanoTimePort(void);

/** return ticks in milliseconds since the system starting */
EXTERN_C UInt32 cTimeTicksPort(void);

//from -12.00 to +12.00
EXTERN_C void cTimeSetTimeZonePort(Float32 tz);

EXTERN_C Float32 cTimeGetTimeZonePort(void);

#endif /* CTIMEPORT_H_ */
