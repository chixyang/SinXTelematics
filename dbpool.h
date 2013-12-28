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

typedef struct DBList dbList;

//忙碌列表类型
typedef dbList dbBusyList;
//空闲列表类型
typedef dbList dbIdleList;


/**
 * 初始化数据库池，建立max_size个链接
 * @param max_size 所要建立的最大链接个数
 * @return 建立的链接个数，-1表示建立出错
 */
int dbpool_init(int max_size);

/**
 * 获取空闲sql链接，若当前无空闲sql链接，则该函数阻塞，等待有的时候再返回
 * @return 空闲的MYSQL链接,NULL表示出错
 */
MYSQL* getIdleConn();

/**
 * 将节点插入忙碌列表
 * @param dbl 待插入的节点
 * @return 0表示插入成功，其他表示插入不成功
 */
int inBusyList(dbBusyList *dbl);

/**
 * 将节点插入空闲列表
 * @param dil 待插入的节点
 * @return 0表示插入成功，其他表示不成功
 */
int inIdleList(dbIdleList *dil);

/**
 * sql链接的回收
 * @param link 待回收的节点
 * @return 0表示回收成功，其他表示回收失败
 */
int recycleConn(MYSQL *link);

/**
 * 在链表中获取某个节点的前一个节点，该函数必须在dbpool->db_busylock或者dbpool->db_idlelock之间使用，否则是不安全的
 * @param dblist 提供的链表头
 * @param link 要查询的mysql链接
 * @param preNode 返回的前一个节点
 * @return 0 表示查到节点，若此时preNode为NULL，则查到的节点为第一个节点，无preNode，否则为查到节点的前一个节点
 *         返回其他表示未查到节点
 */
int getPreNode(dbList *dblist,MYSQL *link,dbList *preNode);




#endif
