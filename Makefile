# Makefile
#
#  Created on: May 24, 2016
#
# 四轴飞行控制器  Copyright (C) 2016  李德强

#安装路径
PATH_INSTALL		= /home/pi/work/quadcopter

################################################################################

#X型 _FLY_MODE_X_ 
#I型 _FLY_MODE_I_
FLY_MODE			= _FLY_MODE_X_
#保护最低速度
PROCTED_SPEED		= (100)
#电机最大速度
MAX_SPEED_RUN_MAX	= (1000)
#电机最小速度
MAX_SPEED_RUN_MIN	= (0)
#10ms 100Hz
ENG_TIMER 			= (10)
#文件名及路径最大长度
MAX_PATH_NAME		= (0x200)
#角度限幅
MAX_PALSTANCE		= (60.0)
#重力读数
MAX_ACC				= (32.0)
#i2c设备路径
I2C_DEV 			= \"/dev/i2c-1\"

#以下为STM32扩展板IO定义
#8路输出PWM_OUT
PWM_OUT0			= (0)
PWM_OUT1			= (1)
PWM_OUT2			= (2)
PWM_OUT3			= (3)
PWM_OUT4			= (4)
PWM_OUT5			= (5)
PWM_OUT6			= (6)
PWM_OUT7			= (7)
##8路输入PWM_IN
RC_PITCH			= (1)	#俯仰
RC_ROLL				= (0)	#横滚
RC_POW				= (2)	#油门
RC_YAW				= (3)	#预留
RC_SENSITIVE		= (5)	#俯仰横滚灵敏度
RC_NULL0			= (4)	#预留
RC_NULL1			= (6)	#预留
RC_NULL2			= (7)	#预留

###############################################################################

#工程
MOD_PROJECT				= quadcopter
#模块动态链接库
MOD_MODULES				= modlibs
MOD_MODULES_IO			= mode_io
MOD_MOTOR				= motor
MOD_PARAMSCTL			= paramsctl
MOD_MPU6050				= mpu6050
MOD_DISPLAY				= display
MOD_COMMAND				= command
MOD_IO					= modio
MOD_STM32				= stm32

#编译目录
MOD_MKDIR			= mkdir
#编译目录
RELEASE_PATH		= release
#头文件
MOD_INCLUDE			= -Iinclude
#编译选项
C_FLAGS				= -pthread -lm -ldl -std=gnu11

all: default

default: defconfig $(MOD_MKDIR) $(MOD_PROJECT) $(MOD_MODULES_IO)

install:
	./shell/install.sh $(PATH_INSTALL)

defconfig:
	echo "#ifndef _DEFCONFIG_H_" > include/defconfig.h
	echo "#define _DEFCONFIG_H_" >> include/defconfig.h
	echo "" >> include/defconfig.h

	echo "#define $(FLY_MODE)" >> include/defconfig.h
	echo "#define PROCTED_SPEED	$(PROCTED_SPEED)" >> include/defconfig.h
	echo "#define MAX_SPEED_RUN_MAX	$(MAX_SPEED_RUN_MAX)" >> include/defconfig.h
	echo "#define MAX_SPEED_RUN_MIN	$(MAX_SPEED_RUN_MIN)" >> include/defconfig.h
	echo "#define ENG_TIMER	$(ENG_TIMER)" >> include/defconfig.h
	echo "#define MAX_PATH_NAME	$(MAX_PATH_NAME)" >> include/defconfig.h
	echo "#define MAX_ACC	$(MAX_ACC)" >> include/defconfig.h
	echo "#define MAX_PALSTANCE	$(MAX_PALSTANCE)" >> include/defconfig.h
	echo "#define I2C_DEV	$(I2C_DEV)" >> include/defconfig.h
	
	echo "#define PWM_OUT0	$(PWM_OUT0)" >> include/defconfig.h
	echo "#define PWM_OUT1	$(PWM_OUT1)" >> include/defconfig.h
	echo "#define PWM_OUT2	$(PWM_OUT2)" >> include/defconfig.h
	echo "#define PWM_OUT3	$(PWM_OUT3)" >> include/defconfig.h
	echo "#define PWM_OUT4	$(PWM_OUT4)" >> include/defconfig.h
	echo "#define PWM_OUT5	$(PWM_OUT5)" >> include/defconfig.h
	echo "#define PWM_OUT6	$(PWM_OUT6)" >> include/defconfig.h
	echo "#define PWM_OUT7	$(PWM_OUT7)" >> include/defconfig.h
	
	echo "#define RC_PITCH	$(RC_PITCH)" >> include/defconfig.h
	echo "#define RC_ROLL	$(RC_ROLL)" >> include/defconfig.h
	echo "#define RC_POW	$(RC_POW)" >> include/defconfig.h
	echo "#define RC_YAW	$(RC_YAW)" >> include/defconfig.h
	echo "#define RC_SENSITIVE	$(RC_SENSITIVE)" >> include/defconfig.h
	echo "#define RC_NULL0	$(RC_NULL0)" >> include/defconfig.h
	echo "#define RC_NULL1	$(RC_NULL1)" >> include/defconfig.h
	echo "#define RC_NULL2	$(RC_NULL2)" >> include/defconfig.h

	echo "" >> include/defconfig.h
	echo "#endif" >> include/defconfig.h

engine:	$(MOD_PROJECT)

$(MOD_PROJECT):
	gcc $(C_FLAGS) -o $(RELEASE_PATH)/bin/$(MOD_PROJECT) $(MOD_INCLUDE)			\
	main/main.c								\
	engine/engine.c							\
	engine/dlmod.c							\
	engine/emode.c							\
	engine/config.c							\
	util/list.c
	
#default option
$(MOD_MODULES):	$(MOD_PARAMSCTL)	$(MOD_MOTOR)	$(MOD_CONTROLLER)	$(MOD_MPU6050)		$(MOD_DISPLAY)	$(MOD_LOGGER)

#io use stm32
$(MOD_MODULES_IO):	$(MOD_PARAMSCTL)	$(MOD_IO) 	$(MOD_MPU6050)		$(MOD_DISPLAY)	$(MOD_LOGGER)


$(MOD_MOTOR):
	cd mods/motor/			&& make

$(MOD_PARAMSCTL):
	cd mods/paramsctl/		&& make

$(MOD_CONTROLLER):
	cd mods/controller/		&& make

$(MOD_MPU6050):
	cd mods/mpu6050/		&& make
	
$(MOD_HCSR04):
	cd mods/hcsr04/			&& make
	
$(MOD_FHEIGHT):
	cd mods/fheight/		&& make

$(MOD_DISPLAY):
	cd mods/display/		&& make
	
$(MOD_LOGGER):
	cd mods/logger/			&& make

$(MOD_COMMAND):
	cd mods/command/		&& make

$(MOD_IO):
	cd mods/io_stm32/		&& make

$(MOD_STM32):
	cd io/stm32/			&& make

$(MOD_MKDIR):
	mkdir -p $(RELEASE_PATH)/bin/ lib/

clean:
	rm -rvf $(RELEASE_PATH)
	rm -rvf lib/*



#ctags
#ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .
#
