/**
 * 数据库操作函数定义
 */
 
 #ifndef DBIO_H
 #define DBIO_H
 
 #include <math.h>
 #include <string.h>
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
 
 #endif
 
