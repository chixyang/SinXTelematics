/**
 * 数据库操作函数的实现,mysql行级锁的设定
 */

#include "dbio.h"

//用户属性列表
UserAttr = {"pwd","city","phone"."status","honest","ip"};

//交通事件的信息结构,struct内部的char指针数据最好直接定义为数组格式，因为新建struct的时候，会malloc，不占用栈空间
struct traffic_event{
	unsigned long long event_id;
	double lat;   
	double lng;
	char event_type;
	int time;
	char street[30];    //数据库里为30个字符
	char city[15];      //数据库里为15个字符
};

//事件详细信息描述结构
struct event_description{
	char description_type;
	char description[100];
	struct event_description *next;
};

/*用户结构体，主要用于交通事件的发送*/
struct user_info{
		char account[20]; //用户账号
		int ip; //用户ip
		struct user_info *next;
};


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
  return dbpool_destory();
}

// 获取当前系统时间
time_t getCurrentTime()
{
	return time((time_t *)NULL); //获取当前国际标准时间，存的是总的秒数，因为time_t其实是int，所以所存储最大时间到2038年
}

//将系统时间转化为字符串形式
char *timeToString(time_t *tt)
{
	struct tm *timenow;
	timenow = localtime(tt); //将国际标准时间转换为本地时区对应的时间
	return asctime(timenow);  //将转化的时间结构体转为字符串形式，形如：Sat Oct 28 02:10:06 2000
}

//添加新用户
int addUser(char *account,char *pwd,char *city,unsigned long long phone,unsigned int ip)
{
  MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
	//设置插入语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"insert into UserAccount(account,pwd,city,phone,ip) values('%s','%s','%s','%ld','%d')", \
	         account,pwd,city,phone,ip);
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
	if((type < USER_PWD) || (type > USER_IP))
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
	case USER_PHONE:  //如果传入的属性值是phone
		sprintf(sql_str,"update UserAccount set phone = '%ld' where account = '%s'", \
	          *(unsigned long long *)info,account);
	  break;
	case USER_HONEST:  //传入的属性值是honest
	  sprintf(sql_str,"update UserAccount set honest = '%lf' where account = '%s'", \
	          *(double *)info,account);
	  break;
	case USER_IP:      //传入的属性值是ip
	  sprintf(sql_str,"update UserAccount set ip = '%d' where account = '%s'", \
	          *(int *)info,account);
	  break;
	case USER_STATUS:      //传入的属性值是ip
	  sprintf(sql_str,"update UserAccount set status = '%c' where account = '%s'", \
	          *(char *)info,account);
	  break;
	default:   //其他传入的属性值，pwd，city
		sprintf(sql_str,"update UserAccount set '%s' = '%s' where account = '%s'", \
	         UserAttr[type],(char *)info,account);
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

//根据用户的城市获取用户的账号和ip
User* getUserInfoByCity(char *city)
{
  if((city == NULL))
	return NULL;
  MYSQL *conn = getIdleConn();
  MYSQL_RES *res;      //查询的result
  MYSQL_ROW row;       //result的row组，被定义为typedef char** MYSQL_ROW,可看出，mysql查询的返回结果都是char *形式的
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  	//设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"select account,ip from UserAccount where city = '%s'", \
	         city);
		//执行查询
	if(mysql_query(conn,sql_str))
	{
		perror("select User error");
		recycleConn(conn);
		free(sql_str);
		return NULL;
	}
	//获取查询结果
	res = mysql_use_result(conn);
	int bytesize = sizeof(User);
	//新建第一个节点
	User *user = (User *)malloc(bytesize);
	memset(user,0,bytesize);
	User *curUser = user, *preUser = NULL;
	//如果查询结果不为为空
	while((row = mysql_fetch_row(res)) != NULL)
    {
  	 memcpy(curUser->account,row[0],strlen(row[0]));
	 curUser->ip = atoi(row[1]);
	 //新建立节点
	 curUser->next = (User *)malloc(bytesize);
	 preUser = curUser;
	 curUser = preUser->next;  //移动到下一个节点
	 memset(curUser,0,bytesize);
    }
	mysql_free_result(res);
	recycleConn(conn);
	free(sql_str);
    //释放掉多申请的一个空间
	free(curUser);
	if(user == curUser)//表明未查询到数据
			user = NULL;
	preUser->next = NULL;  //前一个节点的next置为NULL
	return user;
}

//释放用户信息列表
void freeUser(User *user)
{
	User *preUser = NULL,curUser = user;
	while(curUser != NULL)
	{
			//删除上一个节点并释放
			preUser = curUser;
			curUser = preUser->next;
			free(preUser);
	}
			
}

//创建交通数据，返回event_id
unsigned long long addTrafficEvent(char event_type,int time,double lat,double lng,char *street,char *city,char *account,char status)
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
  sprintf(sql_str,"insert into TrafficEvent(event_type,time,lat,lng,street,city,founder,status) values('%c','%d','%lf','%lf','%s','%s','%s','%c')", \
	  event_type,time,lat,lng,street,city,account,status);
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

//获取交通事件
int getTrafficEvent(unsigned long long event_id,TrafficEvent *te)
{
	if((event_id == 0) || (te == NULL))
		return -1;
	MYSQL *conn = getIdleConn();
  MYSQL_RES *res;      //查询的result
  MYSQL_ROW row;       //result的row组，被定义为typedef char** MYSQL_ROW,可看出，mysql查询的返回结果都是char *形式的
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  	//设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"select event_type,time,lat,lng,street,city from TrafficEvent where event_id = '%ld'", \
	         event_id);
		//执行查询
	if(mysql_query(conn,sql_str))
	{
		perror("select traffic event error");
		recycleConn(conn);
		free(sql_str);
		return -1;
	}
	//获取查询结果
	res = mysql_use_result(conn);
	//如果查询结果不为为空
	if((row = mysql_fetch_row(res)) != NULL)
  {
  	 te->event_type = *((char *)row[0]); //获取事件类型
  	 te->time = atoi(row[1]);
  	 te->lat = atof(row[2]);
  	 te->lng = atof(row[3]);
  	 memcpy(te->street,row[4],strlen(row[4]));
  	 memcpy(te->city,row[5],strlen(row[5]));
	   
	   mysql_free_result(res);
	   recycleConn(conn);
	   free(sql_str);
	   return 0;
  }
  //未查到数据
  mysql_free_result(res);
	recycleConn(conn);
	free(sql_str);
	return -1;
}

/*
//获取事件状态
double getEventStatus(unsigned long long event_id)
{
  if(event_id < START_NUM)
	return -1.0;
  MYSQL *conn = getIdleConn();
  MYSQL_RES *res;      //查询的result
  MYSQL_ROW row;       //result的row组，被定义为typedef char** MYSQL_ROW,可看出，mysql查询的返回结果都是char *形式的
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  	//设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"select status from TrafficEvent where event_id = '%ld'", \
	         event_id);
		//执行查询
	if(mysql_query(conn,sql_str))
	{
		perror("select status error");
		recycleConn(conn);
		free(sql_str);
		return -1.0;
	}
	//获取查询结果
	res = mysql_use_result(conn);
	 double ret = .0d;
	//如果查询结果不为为空
	if((row = mysql_fetch_row(res)) != NULL)
  {
  	   ret = atof(row[0]); //获取status
	   
	   mysql_free_result(res);
	   recycleConn(conn);
	   free(sql_str);
	   return ret;
  }
  //未查到数据
  mysql_free_result(res);
	recycleConn(conn);
	free(sql_str);
	return ret;
}

//获取事件发生地
int getEventCity(unsigned long long event_id,char *city)
{
  if(event_id < START_NUM)
	return -1;
  MYSQL *conn = getIdleConn();
  MYSQL_RES *res;      //查询的result
  MYSQL_ROW row;       //result的row组，被定义为typedef char** MYSQL_ROW,可看出，mysql查询的返回结果都是char *形式的
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  	//设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"select city from TrafficEvent where event_id = '%ld'", \
	         event_id);
		//执行查询
	if(mysql_query(conn,sql_str))
	{
		perror("select city error");
		recycleConn(conn);
		free(sql_str);
		return -1;
	}
	//获取查询结果
	res = mysql_use_result(conn);
	//如果查询结果不为为空
	if((row = mysql_fetch_row(res)) != NULL)
  {
  	 memcpy(city,row[0],strlen(row[0])); //获取城市
	   
	 mysql_free_result(res);
	 recycleConn(conn);
	 free(sql_str);
     return 0;
  }
  //未查到数据
  mysql_free_result(res);
  recycleConn(conn);
  free(sql_str);
  return -1;
}
*/
//更新事件的确认人数
int incrementEventAck(unsigned long long event_id)
{
  MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"update TrafficEvent set ack_num = ack_num + 1 where event_id = '%ld'", \
	         event_id);
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

int addEventAck(char *account,unsigned long long event_id,int time)
{
  MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"insert into EventAck(event_id,account,time) values('%ld','%s','%d')", \
	         event_id,account,time);
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
unsigned long long addDescription(unsigned long long event_id,char *account,char description_type,char *description,int time)
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
  sprintf(sql_str,"insert into EventDescription(event_id,account,description_type,description,time) values('%ld','%s','%c','%s','%d')", \
	  event_id,account,description_type,description,time);
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

//通过event_id获取事件详细信息结构列表
Description* getDescription(unsigned long long event_id)
{
  if(event_id < START_NUM)
	return NULL;
  MYSQL *conn = getIdleConn();
  MYSQL_RES *res;      //查询的result
  MYSQL_ROW row;       //result的row组，被定义为typedef char** MYSQL_ROW,可看出，mysql查询的返回结果都是char *形式的
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  	//设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"select description_type,description from EventDescription where event_id = '%ld'", \
	         event_id);
		//执行查询
	if(mysql_query(conn,sql_str))
	{
		perror("select description error");
		recycleConn(conn);
		free(sql_str);
		return NULL;
	}
	//获取查询结果
	res = mysql_use_result(conn);
	int bytesize = sizeof(Description);
	//新建第一个节点
	Description *description = (Description *)malloc(bytesize);
	memset(description,0,bytesize);
	Description *curDes = description, *preDes = NULL;
	//如果查询结果不为为空
	while((row = mysql_fetch_row(res)) != NULL)
    {
	 curDes->description_type = *((char *)(row[0]));
  	 memcpy(curDes->description,row[1],strlen(row[1]));
	 //新建立节点
	 curDes->next = (Description *)malloc(bytesize);
	 preDes = curDes;
	 curDes = preDes->next;  //移动到下一个节点
	 memset(curDes,0,bytesize);
    }
	mysql_free_result(res);
	recycleConn(conn);
	free(sql_str);
    //释放掉多申请的一个空间
	free(curDes);
	if(description == curDes)//表明未查询到数据
			description = NULL;
	preDes->next = NULL;  //前一个节点的next置为NULL
	return description;
}

//释放事件详细信息结构列表
void freeDescription(Description *des)
{
	Description *preDes = NULL,curDes = description;
	while(curDes != NULL)
	{
			//删除上一个节点并释放
			preDes = curDes;
			curDes = preDes->next;
			free(preDes);
	}
			
}

//添加用户取消交通信息数据
int addEventCancellation(unsigned long long event_id,char *account,int time,char type)
{
  MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置插入语句
  sql_str = (char *)malloc(sizeof(char) * 200);
  memset(sql_str,0,200);
  sprintf(sql_str,"insert into EventCancellation(event_id,account,time,type) values('%ld','%s','%d','%c')", \
	  event_id,account,time,type);
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

//添加nck_num值
int incrementEventCancel(unsigned long long event_id)
{
  MYSQL *conn = getIdleConn();
  unsigned long affected_rows = 0;   //改变的语句数目
  char *sql_str = NULL;   //sql语句
  
  //设置字符编码为utf8
  mysql_setUTF8(conn);
  //设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"update TrafficEvent set nck_num = nck_num + 1 where event_id = '%ld'", \
	         event_id);
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

//添加车队信息,返回车队id
int addTeam(char num,char status,int time)
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
  sprintf(sql_str,"insert into VehicleTeam(num,status,time) values('%c','%c','%d')", \
                                  num,status,time);
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

//判断某个用户是否是一个车队的
int queryTeamMember(int team_id,char *account)
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
        sprintf(sql_str,"select * from TeamMember where team_id = '%d' and account = '%s'", \
                 team_id,account);
                //执行查询
        if(mysql_query(conn,sql_str))
        {
                perror("query team member error");
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

