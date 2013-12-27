/**
 * 数据库池函数定义文件
 */
 
#ifndef DBPOOL_H
#define DBPOOL_H

#include "debug.h"

//服务器名
static const char* server;
//用户名
static const char* username;
//密码
static const char* password;
//数据库名
static const char* database;

//数据库池节点
static struct DBpool *dbpool;

//忙碌列表类型
typedef struct DBBusyList dbBusyList;
//空闲列表类型
typedef struct DBIdleList dbIdleList;



#endif
