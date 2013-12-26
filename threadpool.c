
/**
 * 线程池函数实现文件
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>

/*
*线程池中所有运行和等待的任务都是一个threadtask
*/
typedef struct task
{
    /*任务函数*/
    void *(*process) (void *arg);
    void *arg;/*任务函数的参数*/
    struct task *next;
} threadtask;



/*线程池结构*/
typedef struct
{
    //任务队列互斥访问锁
    pthread_mutex_t queue_lock;
    //线程唤醒条件
    pthread_cond_t queue_ready;

    /*线程池中等待任务的头结点*/
    threadtask *queue_head;

    /*是否销毁线程池*/
    int shutdown;
    //线程池中线程编号
    pthread_t *tid;
    /*线程池中线程总数*/
    int max_thread_num;
    /*当前等待队列的任务数目*/
    int cur_queue_size;

} threadpool;



int pool_add_task (void *(*process) (void *arg), void *arg);

//线程未接到任务时的平常工作
void *thread_routine (void *arg);


//线程池指针
static threadpool *pool = NULL;
void pool_init (int max_thread_num)
{
    pool = (threadpool *) malloc (sizeof (threadpool));

    //静态初始化锁和条件变量
    pool->queue_lock = PTHREAD_MUTEX_INITIALIZER;
    pool->queue_ready = PTHREAD_COND_INITIALIZER;

    pool->queue_head = NULL;

    pool->max_thread_num = max_thread_num;
    pool->cur_queue_size = 0;

    pool->shutdown = 0;    //表示线程池正常工作，未被撤销

    pool->tid = (pthread_t *) malloc (max_thread_num * sizeof (pthread_t));
    int i = 0;
    for (i = 0; i < max_thread_num; i++)
    { 
        pthread_create (&(pool->tid[i]), NULL, thread_routine,NULL);
    }
}



/*加入任务到线程池中*/
int pool_add_task(void *(*process) (void *arg), void *arg)
{
    //判断任务队列是否已经被销毁
    if(pool->shutdown)
      return -1;
    /*建立一个新任务*/
    threadtask *newtask = (threadtask *) malloc (sizeof (threadtask));
    if(newtask == NULL)
    {
      strerror(errno);
      return -1;
    } 
    newtask->process = process;
    newtask->arg = arg;
    newtask->next = NULL;

    //将任务加到队列中
    pthread_mutex_lock (&(pool->queue_lock));
    
    if(pool->queue_head == NULL)  //链表开始为空
      pool->queue_head = newtask;
    else
    {
      threadtask *member = pool->queue_head; 
      while (member->next != NULL)   //找到最后一个指针的前一个指针
        member = member->next;
      member->next = newtask;
    }
    //检查队列插入是否成功
    if(pool->queue_head == NULL)
    {
      perror("线程任务队列出现问题");
      return -1;
    }

    pool->cur_queue_size++;
    
    pthread_mutex_unlock (&(pool->queue_lock));
    /*唤醒一个等待线程*/
    pthread_cond_signal (&(pool->queue_ready));
    
    return 0;
}



/*销毁线程池，等待队列中的任务不会再被执行，但是正在运行的线程会一直把任务运行完后再退出*/
int pool_destroy()
{
    if (pool->shutdown)
        return -1;/*已经被销毁，防止再次调用*/
    pool->shutdown = 1;

    /*唤醒所有等待线程，线程池要销毁了*/
    pthread_cond_broadcast (&(pool->queue_ready));

    /*首先销毁等待队列，防止线程释放时其他线程继续执行*/
    threadtask *head = NULL;
    while (pool->queue_head != NULL)
    {
        head = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free (head);
    }
    pool->cur_queue_size = 0;
    
    /*等待所有线程退出*/
    int i;
    for(i = 0; i < pool->max_thread_num; i++)
        pthread_join(pool->tid[i], NULL);
    free(pool->tid);
    pool->max_thread_num = 0;

    /*条件变量和互斥量也需要销毁*/
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_ready));
    
    //最后释放整个线程池数据结构
    free (pool);
    pool=NULL;
    
    return 0;
}



void *thread_routine(void *arg)
{
    Debug("starting thread 0x%x\n", pthread_self());
    while (1)
    {
        pthread_mutex_lock (&(pool->queue_lock));
        /*如果等待队列为0并且不销毁线程池，则处于阻塞状态; 
        pthread_cond_wait是一个原子操作，等待前和等待时会解锁，唤醒后会加锁*/
        while (pool->cur_queue_size == 0 && !pool->shutdown)   //任务队列等于0的时候才会wait，否则线程执行任务
        {
            Debug("thread 0x%x is waiting\n",pthread_self());
            pthread_cond_wait(&(pool->queue_ready), &(pool->queue_lock));
        }//之所以用while是因为担心broadcast的情况，signal不需要用while

        /*线程池销毁,线程退出*/
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&(pool->queue_lock));
            Debug("thread 0x%x will exit\n", pthread_self ());
            pthread_exit(NULL);
        }

        Debug("thread 0x%x is starting to work\n", pthread_self ());

        /*判断是否调度出现问题，任务队列是否当前为空*/
        if((pool->cur_queue_size == 0) || (pool->queue_head == NULL));
        {
          perror("任务队列为空，调度出现问题！");
          exit(1);  //异常退出，资源都被回收
        }
        
        /*等待队列长度减去1，并取出链表中的头元素*/
        pool->cur_queue_size--;
        threadtask *task = pool->queue_head;
        pool->queue_head = task->next;
        pthread_mutex_unlock (&(pool->queue_lock));

        /*调用回调函数执行任务*/
        (*(task->process)) (task->arg);
        free (task);
        task = NULL;
    }
    /*该句不可达的*/
    pthread_exit (NULL);
}

//    下面是测试代码

void *
myprocess (void *arg)
{
    printf ("threadid is 0x%x, working on task %d\n", pthread_self (),*(int *) arg);
    sleep (1);/*休息一秒，延长任务的执行时间*/
    return NULL;
}

int
main (int argc, char **argv)
{
    pool_init (3);/*线程池中最多三个活动线程*/
    
    /*连续向池中投入10个任务*/
    int *workingnum = (int *) malloc (sizeof (int) * 10);
    int i;
    for (i = 0; i < 10; i++)
    {
        workingnum[i] = i;
        pool_add_worker (myprocess, &workingnum[i]);
    }
    /*等待所有任务完成*/
    sleep (5);
    /*销毁线程池*/
    pool_destroy ();

    free (workingnum);
    return 0;
}
