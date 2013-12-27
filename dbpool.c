/**
 * 数据库池函数实现文件
 */

#include "dbpool.h"

server = "localhost";
username = "root";
password = "sinx123";
database = "telematics";

//数据库池结构
struct DBpool
{
  pthread_mutex_t dblock;   //互斥锁
  pthread_cond_t dbcond;    //条件
  dbBusyList *busylist;     //忙碌列表
  int busy_size;            //忙碌列表大小
  dbIdleList *idlelist;     //空闲列表
  int idle_size;            //空闲列表大小
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

//初始化数据库池
int dbpool_init(int max_size)
{
  if(max_size == 0)
    return 0;
    
  int bytesize = max_size * sizeof(struct DBpool);
  dbpool = (struct DBpool*)malloc(bytesize);  
  //先都初始化为0
  memset(dbpool,0,bytesize);
  
  //初始化互斥和条件变量
  dblock = PTHREAD_MUTEX_INITIALIZER;
  dbcond = PTHREAD_COND_INITIALIZER;
  
  //先建立1个空闲节点
  dbpool->idlelist = (struct DBIdleList *)malloc(sizeof(struct DBIdleList));
  dbIdleList tmpnode = dbpool->idlelist;
  //建立空闲节点的数据库连接  
  MYSQL *conn=mysql_init((MYSQL *)NULL);
  //连接到数据库
	if(!mysql_real_connect(conn, server, user, password, database, 3306, NULL, 0)) 
	{
		perror("create mysql connection error\n");
		mysql_close(conn);
		conn = NULL;
		free(tmpnode);
		return -1;
	}
  tmpnode->db_link = conn;
  //然后再建立max_size-1个空闲节点
  int i = 1;
  for(;i < max_size;i++)
  {
    tmpnode->next = (struct DBIdleList *)malloc(sizeof(struct DBIdleList));
    conn = mysql_init((MYSQL *)NULL);
    if(!mysql_real_connect(conn, server, user, password, database, 3306, NULL, 0)) 
	  {
		  perror("create mysql connection error\n");
		  mysql_close(conn);
		  conn = NULL;
		  free(tmpnode->next);
		  break;
	  }
    tmpnode->next->db_link = conn;
    //下一个节点
    tmpnode = tmpnode->next;
  }
  
  dbpool->idle_size = i;
  dbpool->busylist = NULL;   //初始时候忙碌列表为空
  dbpool->busy_size = 0;
  
  return i;
}

int getIdleConn()










