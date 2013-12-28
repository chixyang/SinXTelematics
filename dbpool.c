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
  pthread_mutex_t db_idlelock;   //空闲表互斥锁
  pthread_mutex_t db_busylock;   //忙碌表互斥锁
  pthread_cond_t dbcond;    //条件
  dbBusyList *busylist;     //忙碌列表
  dbIdleList *idlelist;     //空闲列表
  int idle_size;            //空闲列表大小,专门加这个值是为了确保程序的正确性
};

//忙碌表结构
struct DBList
{
  MYSQL * db_link;
  struct DBList *next;
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
  db_idlelock = PTHREAD_MUTEX_INITIALIZER;
  db_busylock = PTHREAD_MUTEX_INITIALIZER;
  dbcond = PTHREAD_COND_INITIALIZER;
  
  //先建立1个空闲节点
  dbpool->idlelist = (struct DBList *)malloc(sizeof(struct DBList));
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
    tmpnode->next = (struct DBList *)malloc(sizeof(struct DBList));
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
  
  return i;
}

//获取空闲链接如果空闲列表不为空
MYSQL* getIdleConn()
{
  pthread_mutex_lock (&(dbpool->db_idlelock));
  
  while((dbpool == NULL)|| (dbpool->idle_size == 0))  //数据库池或者空闲列表为空
     pthread_cond_wait(&(dbpool->dbcond),&(dbpool->db_idlelock));
  //空闲表出现错误
  if(dbpool->idlelist == NULL)
  {
     pthread_mutex_unlock (&db_idlelock);
     perror("dbpool error!");
     return NULL;
  }
  //有空链接
  dbIdleList * tmp = dbpool->idlelist;
  dbpool->idlelist = dbpool->idlelist->next;
  //节点加入忙碌列表
  inBusyList(tmp);
  
  pthread_mutex_unlock (&db_idlelock);
  
  return tmp->db_link;
}

//插入忙碌列表
int inBusyList(dbBusyList *dbl)
{
  if(dbl == NULL)
    return 0;
    
  pthread_mutex_lock (&(dbpool->db_busylock));
  
  //如果忙碌表为空，直接作为第一个元素
  if(dbpool->busylist == NULL)
  {
     dbpool->busylist = dbl;
     dbl->next = NULL;
     pthread_mutex_unlock (&(dbpool->db_busylock));
     return 0;
  }
  //如果忙碌表不为空,插入作为第一个元素
  dbBusyList *tmp = dbpool->busylist->next;
  dbpool->busylist = dbl;
  dbl->next = tmp;
  /*空闲列表不为空的时候不需要signal*/
  pthread_mutex_unlock (&(dbpool->db_busylock));
  
  return 0;
}

//插入空闲列表
int inIdleList(dbIdleList *dil)
{
  if(dil == NULL)
    return 0;
    
  pthread_mutex_lock (&(dbpool->db_idlelock));
  
  //如果空闲列表为空，直接作为第一个元素
  if(dbpool->idle_size == 0)
  {
    dbpool->idle_size++;
    dbpool->idlelist = dil;
    dil->next = NULL;
    pthread_mutex_unlock (&(dbpool->db_idlelock));
    //唤醒等待程序
    pthread_cond_signal (&(dbpool->dbcond));
    return 0;
  }
  //空闲列表出错
  if(dbpool->idlelist == NULL)
  {
    pthread_mutex_unlock (&(dbpool->db_idlelock));
    perror("idlelist 出错！");
    return -1;
  }
  //如果空闲列表不为空，插入作为第一个元素
  dbIdleList *tmp = dbpool->idlelist->next;
  dbpool->idlelist = dil;
  dil->next = tmp;
  dbpool->idle_size++;
  
  pthread_mutex_unlock (&(dbpool->db_idlelock));
  
  return 0;
}

//回收链接
int recycleConn(MYSQL *link)
{
  if(link == NULL)
    return 0;
  
  pthread_mutex_lock (&(dbpool->db_busylock));
  //获得该节点的前一个节点
  dbBusyList *preNode = NULL, curNode = NULL;
  int tmp = getPreNode(dbpool->busylist,link,preNode);
  if(tmp != 0)//列表中无该节点
  {
    pthread_mutex_unlock (&(dbpool->db_busylock));
    return -1;
  }
  else //列表中有该节点
  {
    if(preNode == NULL)  //该节点为第一个节点，无前一节点
    {
      curNode = dbpool->busylist;
      dbpool->busylist = NULL;    //删除当前节点，并置为NULL
    }
    else
    {
      curNode = preNode->next;
      preNode->next = curNode->next;   //删除当前节点
    }
  }
  pthread_mutex_unlock (&(dbpool->db_busylock));
  curNode->next = NULL;
  inIdleList(curNode);
  
  return 0;
}

int getPreNode(dbList *dblist,MYSQL *link,dbList *curNode)
{
  if((dblist == NULL) || (link == NULL) || (curNode == NULL))   //不能允许任何一个为空
    return -1;
  
  //把给定的dblist当做链表的第一个节点
  dbList *dl = dblist;
  int bytesize = sizeof(MYSQL);
  if(!memcmp(dl->db_link,link,bytesize)) //link为首节点的
  {
    curNode = NULL;
    return 0;
  }
  //比较所有节点的next
}





