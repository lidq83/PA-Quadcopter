/*
 * engine.c
 *
 *  Created on: Apr 12, 2016
 *
 *  四轴飞行控制器  Copyright (C) 2016  李德强
 */

#include <emode.h>
#include <engine.h>

//引擎
s_engine engine;
//参数
s_params params;
//参数缓存，保存用
s_params params_cache;
//用于存放动态链接库
s_list list;

//信号量
sem_t sem_engine;
//多线程描述符
pthread_t pthd;

//环境变量，安装位置
s8 quad_home[MAX_PATH_NAME];

//启动引擎
void engine_start(s32 argc, char* argv[])
{
	//处理Ctrl + C退出信号
	signal(SIGINT, (void (*)(int)) &engine_handler);

	//初始化引擎信号量
	sem_init(&sem_engine, 0, 0);

	//读取环境变量，安装位置
	config_env(quad_home, "QUAD_HOME");

	//处理启动参数
	if (argc >= 2)
	{
		//正常模式，飞行，调参
		if (strcmp(argv[1], "--fly") == 0)
		{
			engine_start_fly();
			return;
		}

		//陀螺仪读数模式
		if (strcmp(argv[1], "--gyro") == 0 && argc == 3)
		{
			emode_start_gyro(argv[2]);
			return;
		}
	}

	printf("unknown option ...\n");
	printf("usage: quadcopter\n");
	printf("\t[--fly: Fly with remote control and adjust quadcopter's parameters by keybroad.]\n");
	printf("\t[--test [GPIO] [SPEED 0-1000] [MS]: Test the connection to the motor come in raspberry.]\n");
	printf("\t[--ctl: Display remote control values.]\n");
	printf("\t[--gyro [MODULE]: Display gyro values.]\n");
	printf("\tex. quadcopter --test [GPIO] [SPEED] [MSECS]\n");
	return;
}

//启动飞行模式
void engine_start_fly()
{
	//重置引擎
	engine_reset(&engine);
	//启动摇控器锁定、解锁电机
	pthread_create(&pthd, (const pthread_attr_t*) NULL, (void* (*)(void*)) &engine_lock, NULL);
	//启动飞行引擎
	pthread_create(&pthd, (const pthread_attr_t*) NULL, (void* (*)(void*)) &engine_fly, NULL);
	//载入并执行动态链接库
	dlmod_init();

	//主线程休眠
	sem_wait(&sem_engine);
}

//引擎核心算法平衡算法
void engine_fly()
{
	s_engine* e = &engine;

	//外环:输入-欧拉角的上一次读数
	f32 x_last = 0.0;
	f32 y_last = 0.0;
	f32 z_last = 0.0;

	//外环:输入-欧拉角的积分变量
	f32 x_sum = 0.0;
	f32 y_sum = 0.0;
	f32 z_sum = 0.0;

	//外环:输出-角速度期望值
	f32 xv_et = 0.0;
	f32 yv_et = 0.0;
	f32 zv_et = 0.0;

	//内环:输入-角速度的上一次读数
	f32 xv_last = 0.0;
	f32 yv_last = 0.0;
	f32 zv_last = 0.0;

	//外环:输入-角速度的积分变量
	f32 xv_sum = 0.0;
	f32 yv_sum = 0.0;
	f32 zv_sum = 0.0;

	//上一次垂直速度
	f32 vz_et = 0.0;

	while (1)
	{
		//实际欧拉角
		e->tx = e->x + e->dx + e->dax + e->ctlmx;
		e->ty = e->y + e->dy + e->day + e->ctlmy;
		e->tz = e->z + e->dz + e->ctlmz;

		//当前实际角速度
		e->tgx = e->gx + e->dgx;
		e->tgy = e->gy + e->dgy;
		e->tgz = e->gz + e->dgz;

		//外环PID对根据欧拉角控制期望角速度
		//这里应该是期望角度-当前实际角度，所以这里为 0 - x_et
		xv_et = engine_outside_pid(-e->tx, -x_last, &x_sum);
		yv_et = engine_outside_pid(-e->ty, -y_last, &y_sum);
		zv_et = engine_outside_pid(-e->tz, -z_last, NULL);

		//内环角速度PID
		//这里应该期望角速度-当前实际角速度
		xv_et -= e->tgx;
		yv_et -= e->tgy;
		zv_et -= e->tgz;

		e->xv_devi = engine_inside_pid(xv_et, xv_last, &xv_sum);
		e->yv_devi = engine_inside_pid(yv_et, yv_last, &yv_sum);
		e->zv_devi = engine_inside_pid(zv_et, zv_last, NULL);

		//记录欧拉角的上一次读数
		x_last = e->tx;
		y_last = e->ty;
		z_last = e->tz;

		//记录角速度的上一次读数
		xv_last = xv_et;
		yv_last = yv_et;
		zv_last = zv_et;

		e->tax = e->ax + e->dax;
		e->tay = e->ay + e->day;
		e->taz = e->az + e->daz;
		e->vx += e->tax;
		e->vy += e->tay;
		e->vz += e->taz;

		//在电机锁定时，停止转动，并禁用平衡补偿，保护措施
		if (e->lock || e->v < PROCTED_SPEED)
		{
			//设置速度为0
			e->v = 0;
			//在电机停转时，做陀螺仪补偿
			engine_set_dxy();
		}
		//原定计算频率1000Hz，但由于MPU6050的输出为100hz只好降低到100hz
		usleep(ENG_TIMER * 1000);
	}
}

//电机锁定解锁处理
void engine_lock()
{
	s_engine* e = &engine;

	//动作开始时间
	struct timeval start;
	//动作计时
	struct timeval end;

	while (1)
	{
		u32 status = e->lock_status;
		//计时状态
		s32 timer_start = 0;
		while (1)
		{
			//最低油门方向最左方向最右
			if (!timer_start && (e->lock_status == 3 || e->lock_status == 5))
			{
				//开始计时
				gettimeofday(&start, NULL);
				//计时状态
				timer_start = 1;
			}

			if (timer_start)
			{
				if (status != e->lock_status)
				{
					break;
				}

				gettimeofday(&end, NULL);
				long timer = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
				if (timer >= 1000 * 1000)
				{
					//方向最左侧解锁电机
					if ((e->lock_status >> 2) & 0x1)
					{
						engine.lock = 0;
						break;
					}
					//方向最右侧锁定电机
					if ((e->lock_status >> 1) & 0x1)
					{
						engine.lock = 1;
						break;
					}
				}
			}
			usleep(100 * 1000);
		}
		usleep(100 * 1000);
	}
}

//内环PID输入角度输出角速度
f32 engine_outside_pid(f32 et, f32 et2, float* sum)
{
	//输出期望角速度
	f32 palstance = 0.0;
	s_engine* e = &engine;
	if (sum == NULL)
	{
		//Z轴PID中只做P和D
		palstance = params.kp * et + params.kd * (et - et2);
		//输出限幅
		engine_limit_palstance(&palstance);
		return palstance;
	}
	*sum += params.ki * et * 0.01;
	//积分限幅
	engine_limit_palstance(sum);
	//XY轴PID反馈控制
	palstance = params.kp * et + (*sum) + params.kd * (et - et2);
	//输出限幅
	engine_limit_palstance(&palstance);
	return palstance;
}

//内环PID输入角速度输出PWM比例（千分比）
f32 engine_inside_pid(f32 et, f32 et2, float* sum)
{
	//输入期望pwm千分比
	f32 pwm = 0.0;
	s_engine* e = &engine;
	if (sum == NULL)
	{
		//Z轴PID中只做P和D
		pwm = params.v_kp * et + params.v_kd * (et - et2);
		//输出限幅
		engine_limit_pwm(&pwm);
		return pwm;
	}
	//积分限幅
	*sum += params.v_ki * et * 0.01;
	engine_limit_pwm(sum);
	//XY轴PID反馈控制
	pwm = params.v_kp * et + (*sum) + params.v_kd * (et - et2);
	//输出限幅
	engine_limit_pwm(&pwm);
	return pwm;
}

//垂直方向速度PID补偿速度
f32 engine_vz_pid(f32 et, f32 et2, float* sum)
{
	f32 v = 0.0;
	s_engine* e = &engine;
	if (sum == NULL)
	{
		v = params.vz_kp * et + params.vz_kd * (et - et2);
		return v;
	}
	*sum += params.vz_ki * et * 0.01;
	v = params.vz_kp * et + (*sum) + params.vz_kd * (et - et2);
	return v;
}

//外环角速度限幅
void engine_limit_palstance(float* palstance)
{
	if (palstance == NULL)
	{
		return;
	}
	s_engine* e = &engine;
	float max_pal = MAX_PALSTANCE * (e->v / MAX_SPEED_RUN_MAX);
	*palstance = *palstance > max_pal ? max_pal : *palstance;
	*palstance = *palstance < -max_pal ? -max_pal : *palstance;
}

//内环PWM限幅
void engine_limit_pwm(float* v)
{
	if (v == NULL)
	{
		return;
	}
	//不大于电机当前速度，防止在低油门是积分项的累加导致一侧转数变的非常大
	s_engine* e = &engine;
	*v = *v > e->v ? e->v : *v;
	*v = *v < -e->v ? -e->v : *v;
}

//引擎重置
void engine_reset(s_engine* e)
{
	e->lock = 1;
	//实际欧拉角
	e->tx = 0;
	e->ty = 0;
	e->tz = 0;
	//陀螺仪修正补偿XYZ轴
	e->dx = 0;
	e->dy = 0;
	e->dz = 0;
	//起飞前根据重力方向的角度补偿
	e->dax = 0;
	e->day = 0;
	e->daz = 0;
	//欧拉角
	e->x = 0;
	e->y = 0;
	e->z = 0;
	//加速度
	e->ax = 0;
	e->ay = 0;
	e->az = 0;
	e->tax = 0;
	e->tay = 0;
	e->taz = 0;
	e->vx = 0;
	e->vy = 0;
	e->vz = 0;
	//摇控器飞行移动倾斜角
	e->ctlmx = 0;
	e->ctlmy = 0;
	e->ctlmz = 0;
	//角速度
	e->gx = 0;
	e->gy = 0;
	e->gz = 0;
	//角速度修正补偿
	e->dgx = 0;
	e->dgy = 0;
	e->dgz = 0;
	//实际角速度
	e->tgx = 0;
	e->tgy = 0;
	e->tgz = 0;
	//重置速度速度置为0
	e->v = 0;
	// XYZ角速度补偿
	e->xv_devi = 0;
	e->yv_devi = 0;
	e->zv_devi = 0;
	//显示摇控器读数
	e->ctl_fb = 0;
	e->ctl_lr = 0;
	e->ctl_pw = 0;
	e->ctl_md = 0;
	e->ctl_ud = 0;
	e->ctl_di = 0;
	//最低油门,最左，最右
	e->lock_status = 0;
}

//陀螺仪补偿
void engine_set_dxy()
{
	s_engine* e = &engine;
	//补偿陀螺仪读数，将3个轴的欧拉角都补偿为0
	e->dx = -e->x;
	e->dy = -e->y;
	e->dz = -e->z;

	//补偿陀螺仪读数，将3个轴的角速度都补偿为0
	e->dgx = -e->gx;
	e->dgy = -e->gy;
	e->dgz = -e->gz;

	//补偿陀螺仪读数，将3个轴的角速度都补偿为0
	e->dax = -e->ax;
	e->day = -e->ay;
	e->daz = -e->az;
	
	e->vx = 0;
	e->vy = 0;
	e->vz = 0;

	e->ctlmx = 0;
	e->ctlmy = 0;
	e->ctlmz = 0;
}

//系统信号处理
void engine_handler()
{
	//重置引擎
	engine_reset(&engine);

	//清理动态链接库
	dlmod_destory();

	//退出
	exit(0);
}
