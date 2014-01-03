/**
 * 线程任务函数实现文件
 */
#include "threadtask.h"

//车队临时结构体
struct team{
	int listID;
	char req_num;  //所请求的车数（总车数）
	char res_num;  //所回复确认加入的车数
	char timer; //当前剩余存在时间
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
int team_init()
{
	//初始时车队列表为空
	TeamList = NULL;
}

//添加新车队列表
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
int addTeamList()
