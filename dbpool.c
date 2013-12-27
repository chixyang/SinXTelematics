
/**
 * 数据库池函数实现文件
 */

#include "dbpool.h"

//数据库池结构
struct DBpool
{
  dbBusyList *busylist;
  int busy_size;
  dbIdleList *idlelist;
  int idle_size;
};

//忙碌表结构
struct DBBusyList
{
  MYSQL * db_link;
  struct DBBusyList *next;
};

//空闲表结构
struct DBIdleList
{
  MYSQL * db_link;
  struct DBIdleList *next;
};

