#ifndef __HEAD_H_
#define __HEAD_H_

#include "stdint.h"
#include <FreeRTOS.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include "semphr.h"
#include "ti_msp_dl_config.h"
#include "ti/driverlib/dl_timer.h"
#include "math.h"
#include "sys.h"
#include "uart.h"
#include "I2C.h"
#include "pid.h"
#include "motor.h"

#define POINTER_ERROR   100000000
#define NORMAL_ERROR    99999999
#define OK              1

#endif