/**
 * 数据库操作函数的实现
 */

#include "dbio.h"

//用户属性列表
UserAttr = {"pwd","license","city","phone"."status","honest","ip"};

//交通事件的信息结构
struct traffic_event{
	unsigned long long event_id;
	double lat;   
	double lng;
	char event_type;
	char *time;
	char *street;
	char cancel_type;
}

//增加用户可信度
static double honestIncerment(double honest)
{
  if((honest-0.5d) < -eps)  //小于0.5
    return (honest *= 2.0d); 
  else  //大于等于0.5
    return (honest = honest/2.0d + 0.5d);
}

//减少用户可信度
static double honestDecrement(double honest)
{
  return (honest /= 2.0d);
}

//计算事件可信度,简单的相加
static double calEventHonest(double honest1,double honest2)
{
  return (honest1 + honest2);
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
  mysql_setUTF8(conn);
	//设置插入语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"insert into UserAccount(account,pwd,license,city,phone,ip) values('%s','%s','%s','%s','%ld','%d')", \
	         account,pwd,license,city,phone,ip);
	//执行插入并判断插入是否成功
	if(mysql_query(conn,sql_str) || ((affected_rows = mysql_affected_rows(conn)) < 1))
	{
		perror("add user error");
		recycleConn(conn);
		free(sql_str);
		return -1;
	}
   
  //插入成功     
	recycleConn(conn);
	free(sql_str);
	return 0;
}

//获取用户city或者ip或者status
char* getUserInfo(char *account,char type)
{
		/*判断type是否越界*/
	if((type < PWD) || (type > IP))
		return NULL;
		
	MYSQL *conn = getIdleConn();
  MYSQL_RES *res;      //查询的result
  MYSQL_ROW row;
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"select %s from UserAccount where account = '%s'",UserAttr[type],account);
	//执行查询
	if(mysql_query(conn,sql_str))
	{
		perror("query user info error");
		recycleConn(conn);
		free(sql_str);
		return NULL;
	}
	//获取查询结果
	res = mysql_use_result(conn);
	//如果查询结果不为空
	if((row = mysql_fetch_row(res)) != NULL)
  {
  	int len = strlen(row[0]);
  	char *info = (char *)malloc(len + 1); //加上结束位
  	memcpy(info,row[0],len);
  	info[len + 1] = '\0'; //加上结束位
  	//释放资源
	  mysql_free_result(res);
	  recycleConn(conn);
	  free(sql_str);
	  return info;
  }
  //未查到数据
  mysql_free_result(res);
	recycleConn(conn);
	free(sql_str);
	return NULL;
}


//查询用户
int queryUser(char *account, char *pwd)
{
  MYSQL *conn = getIdleConn();
  MYSQL_RES *res;      //查询的result
  MYSQL_ROW row;       //result的row组，被定义为typedef char** MYSQL_ROW,可看出，mysql查询的返回结果都是char *形式的
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
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
int updateUser(char *account,void *info,char type)
{
			/*判断type是否越界*/
	if((type < PWD) || (type > IP))
		return -1;
  MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	
	switch(type)
	{
	case PHONE:  //如果传入的属性值是phone
		sprintf(sql_str,"update UserAccount set phone = '%ld' where account = '%s'", \
	          *(unsigned long long *)info,account);
	  break;
	case HONEST:  //传入的属性值是honest
	  sprintf(sql_str,"update UserAccount set honest = '%lf' where account = '%s'", \
	          *(double *)info,account);
	  break;
	case IP:      //传入的属性值是ip
	  sprintf(sql_str,"update UserAccount set ip = '%d' where account = '%s'", \
	          *(int *)info,account);
	  break;
	case STATUS:      //传入的属性值是ip
	  sprintf(sql_str,"update UserAccount set status = '%c' where account = '%s'", \
	          *(char *)info,account);
	  break;
	default:   //其他传入的属性值，pwd，license，city，status
		sprintf(sql_str,"update UserAccount set '%s' = '%s' where account = '%s'", \
	         type,(char *)info,account);
	}
	//执行插入并判断插入是否成功
	if(mysql_query(conn,sql_str) || ((affected_rows = mysql_affected_rows(conn)) < 1))
	{
		perror("update user error");
		recycleConn(conn);
		free(sql_str);
		return -1;
	}
   
  //插入成功     
	recycleConn(conn);
	free(sql_str);
	return 0;
}


//创建交通数据，返回event_id
unsigned long long addTrafficEvent(char event_type,double lat,double lng,char *street,char *city,double status)
{
  MYSQL *conn = getIdleConn();
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
	//设置插入语句
  sql_str = (char *)malloc(sizeof(char) * 200);
  memset(sql_str,0,200);
  sprintf(sql_str,"insert into TrafficEvent(event_type,lat,lng,street,city,status) values('%c','%lf','%lf','%s','%s','%lf')", \
	  event_type,lat,lng,street,city,status);
  //执行插入并判断插入是否成功,并且获取当前event_id值的返回,由于在同一个conn里面，所以多线程是安全的
  if((mysql_query(conn,sql_str)) || \
     ((affected_rows = mysql_affected_rows(conn)) < 1) || \
     (mysql_query(conn,"SELECT LAST_INSERT_ID()")))  //线程安全的
  {
   perror("add traffic event error");
   recycleConn(conn);
   free(sql_str);
   return 0ull;
  }
  //查询成功 
  unsigned long long ret = 0ull;
  res = mysql_use_result(conn);
  if((row = mysql_fetch_row(res)) != NULL)
		ret = atoll(row[0]); //转换为长整型
      
  recycleConn(conn);
  free(sql_str);
  return ret;
}

//更新事件的状态
int updateEventStatus(unsigned long long event_id, double status)
{
  MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"update TrafficEvent set status = '%lf' where event_id = '%ld'", \
	         status,event_id);
	//执行更新并判断更新是否成功
	if(mysql_query(conn,sql_str) || ((affected_rows = mysql_affected_rows(conn)) < 1))
	{
		perror("update event status error");
		recycleConn(conn);
		free(sql_str);
		return -1;
	}
   
  //插入成功     
	recycleConn(conn);
	free(sql_str);
	return 0;
}

//创建详细交通数据，返回description_id
unsigned long long addDescription(unsigned long long event_id,char *account,char description_type,char *description)
{
	MYSQL *conn = getIdleConn();
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置插入语句
  sql_str = (char *)malloc(sizeof(char) * 200);
  memset(sql_str,0,200);
  sprintf(sql_str,"insert into EventDescription(event_id,account,description_type,description) values('%ld','%s','%c','%s','%s','%lf')", \
	  event_id,account,description_type,description);
  //执行插入并判断插入是否成功,并且获取当前event_id值的返回,由于在同一个conn里面，所以多线程是安全的
  if((mysql_query(conn,sql_str)) || \
     ((affected_rows = mysql_affected_rows(conn)) < 1) || \
     (mysql_query(conn,"SELECT LAST_INSERT_ID()")))  //线程安全的
  {
   perror("add description error");
   recycleConn(conn);
   free(sql_str);
   return 0ull;
  }
  //查询成功 
  unsigned long long ret = 0ull;
  res = mysql_use_result(conn);
  if((row = mysql_fetch_row(res)) != NULL)
		ret = atoll(row[0]);  //转换为长整型
      
  recycleConn(conn);
  free(sql_str);
  return ret;
}

//添加用户取消交通信息数据
int addEventCancellation(unsigned long long event_id,char *account,char type)
{
	MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置插入语句
  sql_str = (char *)malloc(sizeof(char) * 200);
  memset(sql_str,0,200);
  sprintf(sql_str,"insert into EventCancellation(event_id,account,type) values('%ld','%s','%c')", \
	  event_id,account,type);
  //执行插入并判断插入是否成功
  if((mysql_query(conn,sql_str)) || \
     ((affected_rows = mysql_affected_rows(conn)) < 1))
  {
   perror("add event cancellation error");
   recycleConn(conn);
   free(sql_str);
   return -1;
  }
  //插入成功
  recycleConn(conn);
  free(sql_str);
  return 0;
}

//添加车队信息,返回车队id
int addTeam(char num,char status)
{
	MYSQL *conn = getIdleConn();
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置插入语句
  sql_str = (char *)malloc(sizeof(char) * 200);
  memset(sql_str,0,200);
  sprintf(sql_str,"insert into VehicleTeam(num,status) values('%c','%c')", \
	  			num,status);
  //执行插入并判断插入是否成功,并且获取当前event_id值的返回,由于在同一个conn里面，所以多线程是安全的
  if((mysql_query(conn,sql_str)) || \
     ((affected_rows = mysql_affected_rows(conn)) < 1) || \
     (mysql_query(conn,"SELECT LAST_INSERT_ID()")))  //线程安全的
  {
   perror("add team error");
   recycleConn(conn);
   free(sql_str);
   return 0;
  }
  //查询成功 
  int ret = 0;
  res = mysql_use_result(conn);
  if((row = mysql_fetch_row(res)) != NULL)
		ret = atoi(row[0]);  //转换为整型
      
  recycleConn(conn);
  free(sql_str);
  return ret;
}

//添加车队成员信息
int addTeamMember(int team_id,char *account)
{
	MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置插入语句
  sql_str = (char *)malloc(sizeof(char) * 200);
  memset(sql_str,0,200);
  sprintf(sql_str,"insert into TeamMember(team_id,account) values('%d','%s')", \
	  team_id,account);
  //执行插入并判断插入是否成功
  if((mysql_query(conn,sql_str)) || \
     ((affected_rows = mysql_affected_rows(conn)) < 1))
  {
   perror("add team member error");
   recycleConn(conn);
   free(sql_str);
   return -1;
  }
  //插入成功
  recycleConn(conn);
  free(sql_str);
  return 0;
}

//删除车队成员
int delTeamMember(int team_id,char *account)
{
	/**首先删除team member**/
	MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置删除语句
  sql_str = (char *)malloc(sizeof(char) * 200);
  memset(sql_str,0,200);
  sprintf(sql_str,"delete from TeamMember where team_id = '%d' and account = '%s'", \
	  team_id,account);
  //执行删除并判断删除是否成功
  if((mysql_query(conn,sql_str)) || \
     ((affected_rows = mysql_affected_rows(conn)) < 1))
  {
   perror("delete team member error");
   recycleConn(conn);
   free(sql_str);
   return -1;
  }
	recycleConn(conn);
	free(sql_str);
	return 0;
}

//获取车队中汽车数目
char getTeamNum(int team_id)
{
	MYSQL *conn = getIdleConn();
  MYSQL_RES *res;      //查询的result
  MYSQL_ROW row;       //result的row组，被定义为typedef char** MYSQL_ROW,可看出，mysql查询的返回结果都是char *形式的
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  	//设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"select num from VehicleTeam where team_id = '%d'", \
	         team_id);
		//执行查询
	if(mysql_query(conn,sql_str))
	{
		perror("select num error");
		recycleConn(conn);
		free(sql_str);
		return 0;
	}
	//获取查询结果
	res = mysql_use_result(conn);
	//如果查询结果不为为空
	if((row = mysql_fetch_row(res)) != NULL)
  {
  	 char num = *(char *)row[0];  //本来就是char类型的
	   mysql_free_result(res);
	   recycleConn(conn);
	   free(sql_str);
	   return num;
  }
  //未查到数据
  mysql_free_result(res);
	recycleConn(conn);
	free(sql_str);
	return 0;
}

//删除车队
int delTeam(int team_id)
{
	MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置删除语句
  sql_str = (char *)malloc(sizeof(char) * 200);
  memset(sql_str,0,200);
  sprintf(sql_str,"delete from VehicleTeam where team_id = '%d'", \
	  team_id);
  //执行删除并判断删除是否成功
  if((mysql_query(conn,sql_str)) || \
     ((affected_rows = mysql_affected_rows(conn)) < 1))
  {
   perror("delete team error");
   recycleConn(conn);
   free(sql_str);
   return -1;
  }
	recycleConn(conn);
	free(sql_str);
	return 0;
}
