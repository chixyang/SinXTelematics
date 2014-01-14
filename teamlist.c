/**
 * 线程任务函数实现文件
 */
#include "teamlist.h"

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
	char *account;   //账户
	int ip;          //用户ip
	char label;      //表示是否收到确认加入的回复
	struct vehicle *next;
};

//初始化汽车列表结构体
void team_list_init()
{
	//初始时车队列表为空
	TeamList = NULL;
	teamID = 0;
	id_lock = PTHREAD_MUTEX_INITIALIZER;
}

//释放汽车节点
void freeVehicleNode(Vehicle *vh)
{
	free(vh->account);
	free(vh);
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
	//给account分配空间
	int bytesize = strlen(account) + 1;
	vhc->account = (char *)malloc(bytesize);
	memcpy(vhc->account,account,bytesize);  //把最后的\0也复制了
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
		
	//查询列表
	pthread_mutex_lock(&id_lock);
	VehicleTeam *vt = TeamList;
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
	//查询到了team,释放team锁
	pthread_mutex_unlock(&id_lock);
	
	//vehicle列表的操作不需要加锁
	Vehicle *vh = vt->VehicleList;
	while(vh && (strcmp(vh->account,account)))
		vh = vh->next;
	if(vh == NULL)  //未找到
		return -1;
	//找到了,加锁保护label和res_num
	pthread_mutex_lock(&res_lock);
	if(vh->label == 0)  //判断一下，以防重复响应
		res_num++;
	pthread_mutex_unlock(&res_lock);
	
	vh->label = 1; //最终label都有设为1
	return 0;
}

//删除teamlist的一个结点并加入数据库,参数表示要删除节点的前一个节点
int teamInDB(VehicleTeam *preVT)
{
	VehicleTeam *vt;
	if(preVT == NULL) //前一个节点为NULL，表示该节点为第一个节点
	{
		//删除头结点
		pthread_mutex_lock(&id_lock);
		vt = TeamList;
		TeamList = TeamList->next;
		pthread_mutex_unlock(&id_lock);
	}
	else 
	{
		pthread_mutex_lock(&id_lock);
		vt = preVT->next;
		preVT->next = vt->next;
		pthread_mutex_unlock(&id_lock);
	}
	//将头结点数据加入数据库中
	int team_id = addTeam(vt->res_num,1,getCurrentTime());
	if(team_id <= 0)  //添加失败
		return -1;
	//添加车队成员表,该节点已从列表上删除，不需要加锁了
	Vehicle *vh = vt->VehicleList,tmp = NULL;
	while(vh)
	{
		if(vh->label == 1) //确认加入车队的车辆
			addTeamMember(team_id,vh->account);
		//删除该节点
		tmp = vh;
		vh = vh->next;
		freeVehicleNode(tmp);
	}
	//释放车队节点
	pthread_mutex_destory(vt->res_lock);
	free(vt);
	return 0;
}

//删除整个teamlist结构
