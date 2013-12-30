/**
 * 数据库操作函数的实现
 */

#include "dbio.h"

//增加用户可信度
static double honestIncerment(double *honest)
{
  if(((*honest)-0.5d) < -eps)  //小于0.5
    return ((*honest) *= 2.0d); 
  else  //大于等于0.5
    return ((*honest) = (*honest)/2.0d + 0.5d);
}

//减少用户可信度
static double honestDecrement(double *honest)
{
  return ((*honest) /= 2.0d);
}

//计算事件可信度,简单的相加
static double calEventHonest(double *honest1,double *honest2)
{
  return ((*honest1) + (*honest2));
}

//角度转化为弧度
static double rad(double d)
{
  return (d * M_PI / 180.0d);
}

//经纬度转换为距离,lat表示纬度，lng表示经度，把地球当做椭圆来算
static double getDistance(double lat1,double lng1,double lat2,double lng2)
{
  double f = rad((lat1 + lat2)/2);
  double g = rad((lat1 - lat2)/2);
  double l = rad((lng1 - lng2)/2);
        
  double sg = sin(g);
  double sl = sin(l);
  double sf = sin(f);
        
  double s,c,w,r,d,h1,h2;
  double a = EARTH_RADIUS;
  double fl = 1/298.257;
        
  sg = sg*sg;
  sl = sl*sl;
  sf = sf*sf;
        
  s = sg*(1-sl) + (1-sf)*sl;
  c = (1-sg)*(1-sl) + sf*sl;
        
  w = atan(sqrt(s/c));
  r = sqrt(s*c)/w;
  d = 2*w*a;
  h1 = (3*r -1)/2/c;
  h2 = (3*r +1)/2/s;
        
  return d*(1 + fl*(h1*sf*(1-sg) - h2*(1-sf)*sg));
}

//打开数据库
int db_init(int link_num)
{
  int ret = dbpool_init(link_num);
  if(ret <= 0)
    return -1;
  
  return 0;
}

//关闭数据库
int db_close()
{
  dbpool_destory();
}

// 获取当前系统时间
char *getCurrentTime()
{
	time_t now;
	struct tm *timenow;
	time(&now); //获取当前国际标准时间
        timenow = localtime(&now); //将国际标准时间转换为本地时间
	return asctime(timenow);
}

//添加新用户
int addUser(char *account,char *pwd,char *license,char *city,unsigned long long phone,unsigned int ip)
{
  MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  setUTF8(conn);
	//设置插入语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"insert into UserAccount(account,pwd,license,city,phone,ip) values('%s','%s','%s','%ld','%s','%d')", \
	         account,pwd,license,city,phone,ip);
	//执行插入
	if(mysql_query(conn,sql_str))
	{
		perror("add user error");
		recycleConn(conn);
		free(sql_str);
		return -1;
	}
	//判断插入是否成功
	if((affected_rows = mysql_affected_rows(conn)) < 1)
	{
		perror("add new user fail.");
		recycleConn(conn);
		free(sql_str);
		return -1;
	}
   
  //插入成功     
	recycleConn(conn);
	free(sql_str);
	return 0;
}

//查询用户
int queryUser(char *account, char *pwd)
{
	MYSQL *conn = getIdleConn();
  MYSQL_RES *res;      //查询的result
  MYSQL_ROW row;       //result的row组，被定义为typedef char** MYSQL_ROW
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  setUTF8(conn);
  	//设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"select * from UserAccount where account = '%s' and pwd = '%s'", \
	         account,pwd);
		//执行查询
	if(mysql_query(conn,sql_str))
	{
		perror("query user error");
		recycleConn(conn);
		free(sql_str);
		return -1;
	}
	//获取查询结果
	res = mysql_use_result(conn);
	//如果查询结果为空
	if((row = mysql_fetch_row(res)) == NULL)
  {
	   mysql_free_result(res);
	   recycleConn(conn);
	   free(sql_str);
	   return -1;
  }
  //查到数据
  mysql_free_result(res);
	recycleConn(conn);
	free(sql_str);
	return 0;
}

//更新用户信息
int updateUser(int account,char *info,char *type)
{
   
}





