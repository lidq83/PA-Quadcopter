/*
 * driver.h
 *
 *  Created on: Apr 16, 2016
 *
 *  四轴飞行控制器  Copyright (C) 2016  李德强
 */

#ifndef _INCLUDE_IO_STM32_H_
#define _INCLUDE_IO_STM32_H_

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <typedef.h>

#define PWM_ERR_MAX								(10)
//摇控器接收机的4个通道读数范围
#define CTL_PWM_MIN (1000)
#define CTL_PWM_MAX (2000)
//摇控器幅度通道比例范围,通过此通道来修改方向通道的数值比例
#define CTL_DI_MIN (1000)
#define CTL_DI_MAX (2000)
//最大倾斜角
#define MAX_ANGLE (MAX_PALSTANCE)
#define MOTOR_COUNT	(8)
#define RCCH_COUNT	(8)

#define BUFF_SIZE	(256)

#define	RC_POS_START1                         	0
#define	RC_POS_START2	                      	1
#define	RC_POS_LEN	                      		2
#define	RC_POS_DATA	                      		3
#define	RC_POS_CRC1 	                      	19
#define	RC_POS_CRC2  	                      	20
#define	RC_POS_END1  	                     	21
#define	RC_POS_END2  	                     	22

#define RC_BYTE_HEAD_1                         	0X55
#define RC_BYTE_HEAD_2                         	0XAA
#define RC_BYTE_END_1                          	0XA5
#define RC_BYTE_END_2		                   	0X5A

#define RC_HEAD                                 0
#define RC_LEN    								1
#define RC_END    								2

//

#define	PWM_POS_START1                         	0
#define	PWM_POS_START2	                      	1
#define	PWM_POS_LEN	                      		2
#define	PWM_POS_DATA	                      	3
#define	PWM_POS_CRC1 	                      	21
#define	PWM_POS_CRC2  	                      	22
#define	PWM_POS_END1  	                     	23
#define	PWM_POS_END2  	                     	24

#define PWM_BYTE_HEAD_1                         0X55
#define PWM_BYTE_HEAD_2                         0XAA
#define PWM_BYTE_END_1                          0XA5
#define PWM_BYTE_END_2		                   	0X5A

#define PWM_HEAD                                0
#define PWM_LEN    								1
#define PWM_END    								2

typedef struct s_buff
{
	s16 head;
	s16 tail;
	s16 size;
	u8 buffer[BUFF_SIZE];
	u32 total_len;
	u32 over;
	u32 user_buf_over;
} s_buff;

s32 __init(s_engine* engine, s_params* params);

s32 __destory(s_engine* e, s_params* p);

s32 __status();

void io_pwm_data();

void io_rc_data();

u16 crc16_value(u8 *buff, u8 len);

int crc16_check(u8 *buff, u8 len, u16 crc16);

int frame_send_pwm_data(u16 *pwm);

void frame_read_rc_data();

int frame_count_rc(s_buff *lb);

int frame_parse_rc();

int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop);

//读入摇控器“前/后”的PWM信号
void controller_pitch_pwm(s32 fb);

//读入摇控器“左/右”的PWM信号
void controller_roll_pwm(s32 lr);

//读入摇控器“油门”的PWM信号
void controller_power_pwm(s32 pw);

//读入摇控器航向通道PWM信号
void controller_yaw_pwm(s32 md);

//取绝对值
f32 controller_abs(f32 x);

//二次曲线函数
f32 controller_parabola(f32 x);

/***
 * est预估值
 * est_devi预估偏差
 * measure测量读数
 * measure_devi测量噪声
 * devi上一次最优偏差
 */
f32 controller_kalman_filter(f32 est, f32 est_devi, f32 measure, f32 measure_devi, float* devi);

#endif /* INCLUDE_DRIVER_H_ */
