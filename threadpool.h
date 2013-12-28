/**
 * 线程池函数定义文件
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include "debug.h"


//线程任务结构
typedef struct Task  threadtask;

//线程池结构
typedef struct ThreadPool threadpool;

//线程池指针
static threadpool *tdpool;  //最后开始时初始化为NULL

/**
 * 线程池初始化函数
 * @param max_thread_num：要产生的最大线程数量
 * @return 实际创建的线程数目,-1表示初始化失败
 */
int tdpool_init (int max_thread_num);

/**
 * 向线程池中添加任务函数
 * @param process ：任务（函数）指针
 * @param arg ： 任务（函数）参数指针
 * @return 0 ：成功
 *         -1：失败
 */
int tdpool_add_task(void *(*process) (void *arg), void *arg);

/**
 * 线程池清除函数，清除任务队列，释放所有线程并回收资源，未执行完的线程等待其执行完
 * @return 0 成功
 *        -1 失败，也表示之前已被销毁
 */
int tdpool_destroy();

/**
 * 线程日常工作函数
 * @param arg：参数
 * @return void* 一般不返回
 */
void *thread_routine (void *arg);

/**
 * 给线程池中新加线程
 * @param add_num 新加线程的数目
 * @return 加成功线程的数目，-1表示线程池存在问题
 */
int tdpool_add_thread(int add_num);






#endif
