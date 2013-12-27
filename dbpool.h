/**
 * 数据库池函数定义文件
 */
 
#ifndef DBPOOL_H
#define DBPOOL_H

#include "debug.h"

//数据库池节点
struct DBpool dbpool;

//忙碌列表类型
typedef struct DBBusyList dbBusyList;
//空闲列表类型
typedef struct DBIdleList dbIdleList;



#endif
