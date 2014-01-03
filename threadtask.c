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
}

//单个车结构体
struct vehicle{
	char *account;   //账户，最好不要占用栈，数据过多时栈会出问题
	int ip;          //用户ip
	char label;      //表示是否收到确认加入的回复
	struct vehicle *next;
}

//初始化汽车列表结构体
int team_init()
{
	//初始时车队列表为空
	TeamList = NULL;
}
