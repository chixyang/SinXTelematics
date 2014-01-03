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
  * @param type 要更改的属性，如UserAttr[IP];
  * @return 0：表示更改成功，其他：失败
  */
 int updateUser(char *account,void *info,char *type);
 
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
 unsigned long long addTrafficEvent(char event_type,double lat,double lng,char *street,char *city,double status);
 
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
  * 添加事件的取消信息
  * @param event_id 事件编号
  * @param account 账户
  * @param type 取消类型（原因）\
  * @return 0：表示成功，其他：失败
  */
 int addEventCancellation(unsigned long long event_id,char *account,char type);
 
 /**
  * 添加车队信息
  * @param num 当前车辆数量
  * @param status 当前组队状况（备用参数）
  * @return 0：添加失败，其他：表示当前添加的车队编号
  */
 int addTeam(char num,char status);
 
 /**
  * 添加车队成员信息
  * @param team_id 车队编号
  * @param account 用户账户
  * @return 0:添加成功，其他：添加失败
  */
 int addTeamMember(int team_id,char *account);
 
 
 
 
 
 #endif
 
