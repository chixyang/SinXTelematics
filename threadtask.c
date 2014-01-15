
/*线程任务，主要是两类任务，一是循环车队列表，将超时的信息删除或者同时加入数据库中，二是循环数据库，将正发生的事件发送给同城所有用户，将超时的事件取消并通知所有用户*/

#include "threadtask.h"

//tcp写函数,不用加锁,write函数本身为原子的
int sinx_write(int fd,char *buf,int len)
{
	int readLen = 0;
	while(readLen < len)
		readLen += write(fd,buf + readLen,len - readLen);
	return readLen;
}

//tcp读函数
int sinx_read(int fd,char *buf,int len)
{
	//读函数首先判断读入的请求类型，如果是description，则继续读一字节，判断是否为image之类的，若是，则直接读入文件
}

//tcp发送普通数据函数,假定ip和port都已经转换为网络字节表示形式
int sinx_sendCommonInfo(int ip,short port,char *message,int len)
{
	int sockfd;
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = ip;
	server_addr.sin_port = port;
	//建立tcp链接
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{   
        perror("sinx_send socket error");   
        return -1;   
    }
	//发起链接
	 if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) 
	{   
        perror("sinx_send connect error");   
        return -1;    
    }
	//发送数据
	int sendLen = sinx_write(sockfd,message,len);
	close(sockfd);
	
	return sendLen;
}

//循环车队列表，超时信息加入数据库中
void pollTeamList()
{
	while(1)
	{
		//等待时间
		struct timeval tv;
		tv.tv_sec = 1;  //休眠一秒钟
		tv.tv_usec = 0;
		select(1,NULL,NULL,NULL,&tv);
		//休眠完后开始执行轮询
		pthread_mutex_lock(&team_list_lock);
		if(TeamList == NULL) //为空则跳过本次循环
		{
			pthread_mutex_unlock(&team_list_lock);
			continue;  
		}
		//轮询任务，减去timer
		VehicleTeam *vt  = TeamList, preVt = NULL;
		while(vt != NULL)
		{
			vt->timer--;  //定时器减时间
			if(vt->timer <= 0) //超时
			{
				if(vt->res_num > 1) //至少有一个确认加入即可
					teamInDB(preVt); //删除节点并将节点加入数据库中,//还需要发送通知消息
				else  //无确认加入的汽车，仅删除该节点即可,//还需要发送通知消息
					delTeam(preVt); //仅删除节点
			}
			//未超时的不管
			preVt = vt;
			vt = preVt->next;
		}
		pthread_mutex_unlock(&team_list_lock);
	}
}
