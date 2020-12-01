/*
 * emode.c
 *
 *  Created on: Apr 26, 2017
 *      Author: lidq
 */

#include <emode.h>
#include <engine.h>

//引擎
extern s_engine engine;
//参数
extern s_params params;
//用于存放动态链接库
extern s_list list;
//环境变量，安装位置
extern s8 quad_home[MAX_PATH_NAME];

void emode_start_gyro(char* argv2)
{
	char modname[MAX_PATH_NAME];
	snprintf(modname, MAX_PATH_NAME, "%s/lib/lib%s.so", quad_home, argv2);
	//重置引擎
	engine_reset(&engine);
	s_engine* e = &engine;
	s_params* p = &params;

	//载入MPU6050模块
	s_dlmod* mod_gyro = dlmod_open(modname);
	if (mod_gyro == NULL)
	{
		return;
	}

	//初始化模块链表
	list_init(&list, &dlmod_free_mod);
	//加入陀螺仪模块
	list_insert(&list, mod_gyro);
	//运行模块功能
	list_visit(&list, (void*) &dlmod_run_pt_init);

	while (1)
	{
		printf("[xyz: %+7.3f %+7.3f %+7.3f ][g: %+7.3f %+7.3f %+7.3f][a: %+7.3f %+7.3f %+7.3f]\n", e->x, e->y, e->z, e->gx, e->gy, e->gz, e->ax, e->ay, e->az);
		usleep(10 * 1000);
	}
}

void emode_start_control(char* argv2)
{
	char modname[0x200];
	snprintf(modname, 0x200, "%s/lib/lib%s.so", quad_home, argv2);
	//重置引擎
	engine_reset(&engine);
	s_engine* e = &engine;
	s_params* p = &params;

	s8 path[MAX_PATH_NAME];
	snprintf(path, MAX_PATH_NAME, "%s/lib/libparamsctl.so", quad_home);
	//载入参数调整模块
	s_dlmod* mod_paramsctl = dlmod_open(path);
	if (mod_paramsctl == NULL)
	{
		return;
	}

	//载入摇控器模块
	s_dlmod* mod_controller = dlmod_open(modname);
	if (mod_controller == NULL)
	{
		return;
	}

	//初始化模块链表
	list_init(&list, &dlmod_free_mod);
	//加入参数控制模块
	list_insert(&list, mod_paramsctl);
	//加入摇控器模块
	list_insert(&list, mod_controller);
	//运行模块功能
	list_visit(&list, (void*) &dlmod_run_pt_init);

	while (1)
	{
		//方向前后
		params.ctl_fb_zero = e->ctl_fb;
		//方向左右
		params.ctl_lr_zero = e->ctl_lr;
		//油门
		params.ctl_pw_zero = e->ctl_pw;
		params.ctl_md_zero = e->ctl_md;
		params.ctl_ud_zero = e->ctl_ud;
		params.ctl_di_zero = e->ctl_di;

		printf("[FB: %4d LR: %4d PW: %4d MD: %4d UD: %4d DI: %4d ] - [FB: %4d LR: %4d PW: %4d MD: %4d UD: %4d DI: %4d]\n", e->ctl_fb, e->ctl_lr, e->ctl_pw, e->ctl_md, e->ctl_ud, e->ctl_di, p->ctl_fb_zero, p->ctl_lr_zero, p->ctl_pw_zero, p->ctl_md_zero, p->ctl_ud_zero, p->ctl_di_zero);

		usleep(2 * 1000);
	}
}
