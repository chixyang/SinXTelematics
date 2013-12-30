/**
 * 数据库操作函数定义
 */
 
 #ifndef DBIO_H
 #define DBIO_H
 
 #include <math.h>
 #include "debug.h"
 
 //浮点数运算误差范围内的最小值
 #define eps 1e-10
 
 //地球半径，单位m
 #define EARTH_RADIUS 63781370
 
 //设置字符编码为utf8
 #define setUTF8(x)    do{                                               \
                             if(mysql_query(x,"set names \'utf8\'"))     \
	                                 {                                      \
		                                    perror("set utf8 error");          \
		                                    recycleConn(x);                    \
		                                    return -1;                         \
	                                 }                                      \
	                          while(0)
 
 
 
 #endif
 
