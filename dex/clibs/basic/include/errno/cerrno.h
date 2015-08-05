/*Copyright 2011 Alibaba Group*/
/*
 *  cerrno.h
 *  madagascar
 *
 *  Created by Misa.Z on 2/19/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_ERRNO_H
#define CLIBS_ERRNO_H

#include "std/cstd.h"

#define ERROR_NOTHING                            0

#define ERROR_MEM_ALLOC                            -1
#define ERROR_MEM_FREE                            -2
#define ERROR_MEM_NULL_OBJ                        -3

#define ERROR_ILLEGAL_ARGUMENT                    -10

#define ERROR_GUI_VIDEO_MODE                    -100
#define ERROR_GUI_VIDEO_ROTATE                    -101
#define ERROR_GUI_CANVAS_MODE                    -102
#define ERROR_GUI_BPP_NOT_SUPPORT                -103

#define ERROR_IO_ID                                -200
#define ERROR_IO_MAX                            -201
#define ERROR_IO_EAGAIN                            -202

#define ERROR_DEV_NONE                            -300
#define ERROR_DEV_DISABLED                        -301
#define ERROR_DEV_COMMAND                        -302
#define ERROR_DEV_BUSY                            -303
#define ERROR_DEV_ASYNC_NOTIFY                    -304
#define ERROR_DEV_CMD_NOT_SUPPORT                -320
#define ERROR_DEV_GENERNAL_ERROR                -321

#define ERROR_EV_OVERFLOW                        -400

#define ERROR_SOCKET_GENERAL_ERROR                -500
#define ERROR_SOCKET_NO_SERVICE                    -501
#define ERROR_SOCKET_CREATE_FAILED                -502
#define ERROR_SOCKET_SET_OPTION                    -503
#define ERROR_SOCKET_INVALID_SOCKET                -504
#define ERROR_SOCKET_INVALID_ADDR                -505
#define ERROR_SOCKET_GET_HOST_BY_NAME            -506
#define ERROR_SOCKET_MSG_TOO_LONG                -507
#define ERROR_SOCKET_NOT_CONNECT                -508
#define ERROR_SOCKET_NOT_BINDED                    -509//listen without bind first
#define ERROR_SOCKET_NOT_LISTENED                -510//accept without listen first
#define ERROR_SOCKET_BROKEN                        -520

#define ERROR_MEDIA_OPEN                        -600


#define ERROR_SMS_GENERAL_ERROR                    -700
#define ERROR_SMS_BAD_FORMAT                    -701
#define ERROR_SMS_TOO_LARGE                        -702
#define ERROR_SMS_INVALID_HOST                    -703


#define ERROR_PIM_GENERAL_ERROR                    -800
#define ERROR_PIM_INVALID_INDEX                    -801


#define ERROR_HTTPS_NOT_SUPPORT                    -900

#define ERROR_ICONV_INSUFFICIENT_BUFFER            -1000
//ERROR_NO_UNICODE_TRANSLATION is returned if the input string is a UTF-8 string that contains invalid characters and the MB_ERR_INVALID_CHARS flag is set.
#define ERROR_ICONV_NO_UNICODE_TRANSLATION        -1001

EXTERN_C Int32 _cerrno;

#endif
