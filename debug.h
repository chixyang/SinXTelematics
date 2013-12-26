/**
 * 此文件用于debug宏的定义
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>


#define DEBUG


#ifdef DEBUG
#define Debug(...) do{                                                      		\
	           printf("debug info: ***");                                     	\
                   printf(__VA_ARGS__);                                     		\
	           printf("***debug line:%d, debug file:%s\n",__LINE__,__FILE__); 	\
                   }while(0)
#else 
#define Debug(...) do{}while(0)
#endif











#endif
