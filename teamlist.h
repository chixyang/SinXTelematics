/**
 * 线程任务函数定义文件
 */

#ifndef TEAMLIST_H
#define TEAMLIST_H

#include <stdlib.h>
#include "dbio.h"
#include "debug.h"

//车队结构体
typedef struct team VehicleTeam;

//车队列表
VehicleTeam TeamList;

//单个车结构体
typedef struct vehicle Vehicle;

//列表的最大长度，即最多允许9999个组队的情况同时发生
#define MAX_LIST_NUM 9999

/**
 * 初始化汽车列表结构体 
 */
void team_list_init();

/**
 * 释放单个汽车节点
 * @param vh 汽车节点
 */
void freeVehicleNode(Vehicle *vh);

/**
 * 添加新车队列表
 * @param head 车队头列表
 * @param account 用户账户
 * @return 已添加成功的车队列表，NULL表示未添加成功，若head为NULL，则返回值为车队头节点
 */
Vehicle* addVehicles(Vehicle *head,char *account);

/**
 * 添加新的车队信息
 * @param req_num 所请求的车数
 * @param vehicles 车队成员列表
 * @return 0表示添加成功，否则失败
 */
int addTeamList(char req_num,Vehicle *vehicles);

/**
 * 获取用户确认回复后修改车队列表项
 * @param team_id 车队编号
 * @param account 用户账户
 * @return 0 表示修改成功，其他表示修改失败
 */
int setVehicleLabel(int team_id,char *account);

/**
 * 删除teamlist的一个节点并加入数据库中
 * @param preVT 要删除节点的前一个节点
 * @return 0 表示删除成功，否则失败
 */
int teamInDB(VehicleTeam *preVT);





#endif
