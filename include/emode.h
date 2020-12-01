/*
 * emode.h
 *
 *  Created on: Apr 26, 2017
 *      Author: lidq
 */

#ifndef INCLUDE_EMODE_H_
#define INCLUDE_EMODE_H_

#include <typedef.h>

//电调起始时长(仅在--ctl模式下用)
#define TEST_ZERO_MS (1000)
//最大调试时长10秒(仅在--ctl模式下用)
#define TEST_MAX_MS (10000)

//陀螺仪读数
void emode_start_gyro(char* argv2);

//摇控器校准
void emode_start_control(char* argv2);

#endif /* INCLUDE_EMODE_H_ */
