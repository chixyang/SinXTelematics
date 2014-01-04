/**
 * 线程任务函数定义文件
 */

#ifndef THREADTASK_H
#define THREADTASK_H

#include <stdlib.h>
#include "debug.h"

//车队结构体
typedef struct team VehicleTeam;

//车队列表
VehicleTeam TeamList;

//单个车结构体
typedef struct vehicle Vehicle;

//列表的最大长度，即最多允许9999个组队的情况同时发生
#define MAX_LIST_NUM 9999




#endif
