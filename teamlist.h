/**
 * �߳������������ļ�
 */

#ifndef TEAMLIST_H
#define TEAMLIST_H

#include <stdlib.h>
#include "dbio.h"
#include "debug.h"

//���ӽṹ��
typedef struct team VehicleTeam;

//�����б�
VehicleTeam TeamList;

//�������ṹ��
typedef struct vehicle Vehicle;

//�б����󳤶ȣ����������9999����ӵ����ͬʱ����
#define MAX_LIST_NUM 9999

/**
 * ��ʼ�������б�ṹ�� 
 */
void team_list_init();

/**
 * �ͷŵ��������ڵ�
 * @param vh �����ڵ�
 */
void freeVehicleNode(Vehicle *vh);

/**
 * ����³����б�
 * @param head ����ͷ�б�
 * @param account �û��˻�
 * @return ����ӳɹ��ĳ����б�NULL��ʾδ��ӳɹ�����headΪNULL���򷵻�ֵΪ����ͷ�ڵ�
 */
Vehicle* addVehicles(Vehicle *head,char *account);

/**
 * ����µĳ�����Ϣ
 * @param req_num ������ĳ���
 * @param vehicles ���ӳ�Ա�б�
 * @return 0��ʾ��ӳɹ�������ʧ��
 */
int addTeamList(char req_num,Vehicle *vehicles);

/**
 * ��ȡ�û�ȷ�ϻظ����޸ĳ����б���
 * @param team_id ���ӱ��
 * @param account �û��˻�
 * @return 0 ��ʾ�޸ĳɹ���������ʾ�޸�ʧ��
 */
int setVehicleLabel(int team_id,char *account);

/**
 * ɾ��teamlist��һ���ڵ㲢�������ݿ���
 * @param preVT Ҫɾ���ڵ��ǰһ���ڵ�
 * @return 0 ��ʾɾ���ɹ�������ʧ��
 */
int teamInDB(VehicleTeam *preVT);





#endif
