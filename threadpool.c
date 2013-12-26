/**
 * 线程池函数实现文件
 */
 
#include "threadpool.h"

/*
*线程池中所有运行和等待的任务都是一个threadtask
*/

struct Task
{
    /*任务函数*/
    void *(*process) (void *arg);
    void *arg;/*任务函数的参数*/
    struct task *next;
};



/*线程池结构*/
struct ThreadPool
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

};


//线程池初始化
int pool_init (int max_thread_num)
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
    int i = 0 , err = 0;
    for (; i < max_thread_num; i++)
    { 
        err = pthread_create (&(pool->tid[i]), NULL, thread_routine,NULL);
        if(err != 0) //创建错误
            break;
    }
    
    pool->max_thread_num = i;   //i为实际创建的线程数目
    
    return pool->max_thread_num;
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
    /*唤醒至少一个等待线程*/
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

//线程任务函数
void *thread_routine(void *arg)
{
    Debug("starting thread 0x%x\n", pthread_self());
    while (1)
    {
        pthread_mutex_lock (&(pool->queue_lock));
        /*如果等待队列为0并且不销毁线程池，则处于阻塞状态; pthread_cond_wait是一个原子操作，等待前和等待时会解锁，唤醒后会加锁*/
        
        /*
         * pthread_cond_wait使用while循环的两个原因：
         * 1.防止假唤醒，因为唤醒线程的不一定是signal或者broadcast，有可能是系统的一些信号，循环判断可以防止这类情况
         * 2.signal一次至少可以唤醒一个线程，但是加锁成功的却只有一个，也就是，除了一个线程能运行出while循环以外，其他
         * 线程还是卡在pthread_cond_wait上等待锁，但却不需要条件唤醒了，待到获得锁的线程执行完程序后，上次被唤醒并卡在
         * wait上的线程获得锁，如果没有while循环，就直接运行下面代码了，这时如果任务队列size==0，就会出错。
         */
        
        while(pool->cur_queue_size == 0 && !pool->shutdown)   //任务队列等于0的时候才会wait，否则线程执行队列中的任务
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

        /*判断是否调度出现问题，任务队列是否当前为空，能进来的队列一定不为空*/
        if((pool->cur_queue_size == 0) || (pool->queue_head == NULL));
        {
          perror("任务队列为空，调度出现问题！");
          exit(1);  //异常退出，资源都被回收，线程调用会终止进程及其他线程
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
