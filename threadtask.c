/**
 * 线程任务函数实现文件
 */
#include "threadtask.h"

//车队编号,从1开始
int teamID;

//车队id访问锁
pthread_mutex_t id_lock;

//车队临时结构体
struct team{
	int id;
	pthread_mutex_t res_lock;  //修改res_num时需要加锁
	char req_num;  //所请求的车数（总车数）
	char res_num;  //所回复确认加入的车数
	char timer; //当前剩余存在时间，最开始默认3分钟
	struct vehicle *VehicleList;  //车队成员
	struct team *next;
};

//单个车结构体
struct vehicle{
	char *account;   //账户，最好不要占用栈，数据过多时栈会出问题
	int ip;          //用户ip
	char label;      //表示是否收到确认加入的回复
	struct vehicle *next;
};

//初始化汽车列表结构体
int team_list_init()
{
	//初始时车队列表为空
	TeamList = NULL;
	teamID = 0;
	id_lock = PTHREAD_MUTEX_INITIALIZER;
}

//添加新车队列表,Vehicle链表的创建是由单线程完成的，所以不用使用锁，不用担心安全性
Vehicle* addVehicles(Vehicle *head,char *account)
{
	//获取ip
	char* ipstr = getUserInfo(account,IP);
	if(ipstr == NULL)
	{
		perror("add Vehicles error");
		return NULL;
	}
	//字符串转换为ip
	int ip = atoi(ipstr);
	free(ipstr); //需要释放空间
			//新建结点并设置
	Vehicle *vhc = (Vehicle *)malloc(sizeof(Vehicle));
	memset(vhc,0,sizeof(Vehicle));
	vhc->account = account;
	vhc->ip = ip;
	vhc->label = 0;
	//判断是否所需添加节点将作为头结点
	if(head == NULL)
		return vhc;
	//所需添加结点不为头结点
	Vehicle *cur = head;
	//找到最后一个结点
	while(cur->next)
		cur = cur->next;
	cur->next = vhc;  //添加作为最后一个结点
	
	//返回头指针
	return head;
}
//添加新项到结构体中
int addTeamList(char req_num,Vehicle *vehicles)
{
	if((req_num == 0) || (vehicles == NULL))
		return -1;
	
	VehicleTeam *vt = (VehicleTeam *)malloc(sizeof(VehicleTeam)); //新建一个结点
	memset(vt,0,sizeof(VehicleTeam));	
	//初始化结点各属性信息
	vt->res_lock = PTHREAD_MUTEX_INITIALIZER;
	vt->req_num = req_num;
	vt->timer = 3; //表示存活时间为三分钟
	
	pthread_mutex_lock(&id_lock);
	//设置id号
	vt->id = teamID % MAX_LIST_NUM + 1;   //保证在1-10000之间
	/*其他属性都为0*/
	
	//TeamList是否为空
	if(TeamList == NULL)
	{
		TeamList = vt;
		pthread_mutex_unlock(&id_lock);
		return 0;
	}
	//TeamList不为空
	VehicleTeam *tmp = TeamList;
	while(tmp->next)
		tmp = tmp->next;
	tmp->next = vt;
	pthread_mutex_unlock(&id_lock);
	
	return 0;
}

//修改res_num和Vehicle链表项
int setVehicleLabel(int team_id,char *account)
{
	if((team_id == 0) || (account == NULL))
		return -1;
		
	VehicleTeam *vt = TeamList;
	//查询列表
	pthread_mutex_lock(&id_lock);
	while(vt)
	{
		if(vt->id == team_id)
			break;
		vt = vt->next;
	}
	//如果未查询到
	if(vt == NULL)
	{
		pthread_mutex_unlock(&id_lock);
		return -1;
	}
	//查询到了
	if(vt->label == 0)
		
	
	
}

//删除teamlist的一个结点并加入数据库

//删除整个teamlist结构
