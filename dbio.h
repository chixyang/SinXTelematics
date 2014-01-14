/**
 * 数据库操作函数定义
 */
 
 #ifndef DBIO_H
 #define DBIO_H
 
 #include <math.h>
 #include <string.h>
 #include <stdlib.h>
 #include "threadpool.h"
 #include "debug.h"
 
 //浮点数运算误差范围内的最小值
 #define eps 1e-10
 
 //地球半径，单位m
 #define EARTH_RADIUS 63781370
 
 //数据库中序号的起始值
 #define START_NUM 10000
 
 //设置字符编码为utf8
 #define mysql_setUTF8(x)    do{                                          \
                              if(mysql_query(x,"set names \'utf8\'"))     \
	                                 {                                      \
		                                    perror("set utf8 error");          \
		                                    recycleConn(x);                    \
		                                    return -1;                         \
	                                 }                                      \
	                           while(0)
 
 //用户登录登出信息
 enum LogType{
 	LOGIN,
 	LOGOUT
 	};
 
 char **UserAttr;   //用户在数据库中的属性
 //用户属性所对应的枚举数值
 enum userattr{
 	PWD,
 	LICENSE,
 	CITY,
 	PHONE,
 	STATUS,
 	HONEST,
 	IP
 	};
 	
 	//交通事件类型
 enum EventType{
 	ACCIDENT,
 	CONGESTION,
 	ADMINISTRATION,
 	DISASTER,
 	OTHERS
 };
 
 //事件的具体信息描述类型
 enum DescriptionType{
 	NOTHING,
 	TEXT,
 	IMAGE,
 	AUDIO,
 	VEDIO
 };
 
 //事件取消类型
 enum QuitType{
 	RELIEVE,
 	FAKENESS
 };
 
 //交通事件结构
 typedef struct traffic_event TrafficEvent;
 //事件详细信息结构
 typedef struct event_description Description;
 //用户结构
 typedef struct user_info User;
 
 /**
  * 增加用户的可信度
  * @param honest 原来的可信度
  * @return 增加后的可信度
  */
 static double honestIncerment(double honest);
 
 /**
  * 减少用户可信度
  * @param honest 原来的可信度
  * @return 增加后的可信度
  */
 static double honestDecrement(double honest);
 
 /**
  * 计算事件可信度
  * @param honest1 事件1的可信度
  * @param honest2 事件2的可信度
  * @return 事件的可信度
  */
 static double calEventHonest(double honest1,double honest2);
 
 /**
  * 根据经纬度计算距离
  * @param lat1 第一个点的纬度
  * @param lng1 第一个点的精度
  * @param lat2 第二个点的纬度
  * @param lng2 第二个点的精度
  * @return 计算出来的距离
  */
 static double getDistance(double lat1,double lng1,double lat2,double lng2);
 
 /**
  * 数据库的初始化
  * @param link_num 初始化时打开的链接数目
  * @return 0 ：初始化成功，其他：初始化失败
  */
 int db_init(int link_num);
 
 /**
  * 关闭数据库
  * @return 0：关闭成功，其他：关闭失败
  */
 int db_close();
 
 /**
  * 获取当前时间
  * @return 返回的是当前时间的秒数，从1970年开始算起，time_t 也是int类型
  */
 time_t getCurrentTime();

 /**
  * 将系统时间转化为字符串形式
  * @param tt 采用getCurrentTime获得的当前时间的秒数指针
  * @return char *返回当前时间的字符串形式，形如：Sat Oct 28 02:10:06 2000
  */
 char *timeToString(time_t *tt);

 /**
  * 添加新用户
  * @param account 用户账户
  * @param pwd 用户密码
  * @param license 用户车牌
  * @param city 用户所在城市
  * @param phone 用户手机号
  * @param ip 用户手机ip
  * @return 0：表示成功，其他：插入失败
  */
 int addUser(char *account,char *pwd,char *license,char *city,unsigned long long phone,unsigned int ip);
 
 /**
  * 获取用户信息：city，ip，status
  * @param account 用户账户
  * @param type 所获取的信息类型：CITY，IP，STATUS
  * @return NULL 获取失败，其他：获取成功,返回的数据空间需要用free显示释放
  */
 char* getUserInfo(char *account,char type);
 
 /**
  * 查询用户是否存在
  * @param account 用户账户
  * @param pwd 用户密码
  * @return 0：表示存在，其他不存在
  */
 int queryUser(char *account, char *pwd);
 
 /**
  * 更新用户信息
  * @param account 用户账户
  * @param info 要改为的信息
  * @param type 要更改的属性，如IP，PWD等
  * @return 0：表示更改成功，其他：失败
  */
 int updateUser(char *account,void *info,char type);
 
 /**
  * 根据用户的城市获取用户的账号和ip列表
  * @param city 用户的城市
  * @return User* 用户列表，未获取成功则返回NULL，返回数据需要采用freeUser显式释放
  */
 User* getUserInfoByCity(char *city);

 /**
  * 释放用户信息列表User
  * @param user 用户信息列表
  */
 void freeUser(User *user);
 
 /**
  * 添加交通事件
  * @param event_type 事件类型
  * @param lat 事件纬度
  * @param lng 事件精度
  * @param sreet 事件发生的街道
  * @param city 事件发生的城市
  * @param status 事件当前的状态（填入当前发布用户的可信度）
  * @return 0:表示添加失败，其他表示当前添加事件的event_id
  */
 unsigned long long addTrafficEvent(char event_type,int time,double lat,double lng,char *street,char *city,double status);
 
 /**
  * 获取交通事件信息
  * @param event_id 事件id
  * @param te 事件结构体
  * @return 0 表示获取成功，否则失败
  */
 int getTrafficEvent(unsigned long long event_id,TrafficEvent *te);

 /**
  * 获取事件的状态值
  * @param event_id 事件id
  * @return 事件状态值,小于0表示获取失败，大于0为当前状态值
  */
 double getEventStatus(unsigned long long event_id);

 /**
  * 获取事件发生地
  * @param event_id 事件编号
  * @param city 要赋值的事件发生地
  * @return 0 表示获取成功，否则失败
  */
 int getEventCity(unsigned long long event_id,char *city);

 /**
  * 更新交通事件的状态
  * @param event_id 事件号
  * @param status 事件的状态
  * @return 0:表示成功，其他：表示失败
  */
 int updateEventStatus(unsigned long long event_id, double status);
 
 /**
  * 添加事件详细信息
  * @param event_id 事件编号
  * @param account 用户账户
  * @param description_type 事件描述类型
  * @param description 描述具体的内容或者其链接
  * @return 0：表示添加失败，其他表示当前添加的具体信息的description_id
  */
 unsigned long long addDescription(unsigned long long event_id,char *account,char description_type,char *description);

 /**
  * 通过事件id获取事件详细信息结构列表
  * @param event_id 事件id
  * @return Description *事件详细信息结构列表。NULL表示未获取成功,返回内容需要采用freeDescription显式释放
  */
 Description* getDescription(unsigned long long event_id);

 /**
  * 释放事件详细信息列表
  * @param des 事件详细信息列表
  */
 void freeDescription(Description *des);

 /**
  * 添加事件的取消信息
  * @param event_id 事件编号
  * @param account 账户
  * @param time 当前时间
  * @param type 取消类型（原因）\
  * @return 0：表示成功，其他：失败
  */
 int addEventCancellation(unsigned long long event_id,char *account,int time,char type);
 
 /**
  * 添加车队信息
  * @param num 当前车辆数量
  * @param status 当前组队状况（备用参数）
  * @param time 当前时间
  * @return 0：添加失败，其他：表示当前添加的车队编号
  */
 int addTeam(char num,char status,int time);
 
 /**
  * 添加车队成员信息
  * @param team_id 车队编号
  * @param account 用户账户
  * @return 0:添加成功，其他：添加失败
  */
 int addTeamMember(int team_id,char *account);
 
 /**
  * 删除车队成员记录
  * @param team_id 车队编号
  * @param account 用户账户
  * @return 0：表示删除成功，其他：表示删除失败
  */
 int delTeamMember(int team_id,char *account);
 
 /**
  * 获取车队中汽车数目
  * @param team_id 车队编号
  * @return 车辆数目，0表示错误
  */
 char getTeamNum(int team_id);
 
 /**
  * 删除车队
  * @param team_id 车队编号
  * @return 0:表示删除成功，其他表示删除失败
  */
 int delTeam(int team_id);

 /**
  * 查询某个用户是否属于某个车队
  * @param team_id 车队编号
  * @param account 用户账户
  * @return 0 属于，其他表示不属于
  */
 int queryTeamMember(int team_id,char *account);

 
 
 #endif
 
