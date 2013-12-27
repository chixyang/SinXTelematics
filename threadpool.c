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

    pool->cur_queue_size = 0;

    pool->shutdown = 0;    //表示线程池正常工作，未被撤销

    int bytesize = max_thread_num * sizeof (pthread_t);
    pool->tid = (pthread_t *) malloc (bytesize);
    
    if(pool->tid == NULL) //未分配成功
    {
     strerror(errno);
     return -1;
    }
    //初始化
    memset(pool->tid,0,bytesize);
    
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
      free(newtask);
      pthread_mutex_unlock (&(pool->queue_lock));
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
    
    //公共变量的修改都需要加锁
    pthread_mutex_lock (&(pool->queue_lock));
    pool->shutdown = 1;
    pthread_mutex_unlock (&(pool->queue_lock));

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
      if(pool->tid[i] != NULL)  //创建线程时可能创建失败，但失败的都被初始化为0，所以失败的不用join
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
        }

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

//给线程池中新加线程，单独子线程而非主线程完成的任务，所以要加锁
int pool_add_thread(int add_num)
{
    pthread_mutex_lock (&(pool->queue_lock));
    if((pool == NULL) || (pool -> shutdown == 1))  //线程池存在问题
    {
     pthread_mutex_unlock (&(pool->queue_lock));
     return -1;
    }
    
    //新建一个大的空间存储原来的线程和添加的线程号
    int bytesize = (pool->max_thread_num + add_num) * sizeof (pthread_t);
    pthread_t *tmp = (pthread_t *) malloc (bytesize);
    if(tmp == NULL)
    {
       strerror(errno);
       pthread_mutex_unlock (&(pool->queue_lock));
       return 0;
    }
    //初始化
    memset(tmp,0,bytesize);
    //原来数据复制到当前新区域
    memcpy(tmp,pool->tid,(pool->max_thread_num * sizeof (pthread_t)));
    //释放原空间并将tid设置为新空间
    free(pool->tid);
    pool->tid = tmp;
    tmp = NULL;
    
    //创建新线程
    int i = 0 , err = 0;
    for (; i < add_num; i++)
    { 
        err = pthread_create (&(pool->tid[pool->max_thread_num + i]), NULL, thread_routine,NULL);
        if(err != 0) //创建错误,不管创建错误后多分配的那部分空间
            break;
    }
    
    pool->max_thread_num += i;   //i为实际创建的线程数目
    
    pthread_mutex_unlock (&(pool->queue_lock));
    
    return i;
}







