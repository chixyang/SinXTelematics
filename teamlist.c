/**
 * �߳�������ʵ���ļ�
 */
#include "teamlist.h"


//������ʱ�ṹ��
struct team{
	int id;
	//pthread_mutex_t res_lock;  //�޸�res_numʱ��Ҫ����//������ʹ����Ҳ���ܹ�����ʹ�ã�������id_lock��ʹ�ã��Է�������ʹ�õ�ʱ��destroy��
	char req_num;  //������ĳ������ܳ�����
	char res_num;  //���ظ�ȷ�ϼ���ĳ���
	char timer; //��ǰʣ�����ʱ�䣬�ʼĬ��3����
	struct vehicle *VehicleList;  //���ӳ�Ա
	struct team *next;
};

//�������ṹ��
struct vehicle{
	char *account;   //�˻�
	int ip;          //�û�ip
	char label;      //��ʾ�Ƿ��յ�ȷ�ϼ���Ļظ�
	struct vehicle *next;
};

//��ʼ�������б��ṹ��
void team_list_init()
{
	//��ʼʱ�����б�Ϊ��
	TeamList = NULL;
	teamID = 0;
	id_lock = PTHREAD_MUTEX_INITIALIZER;
}

//�ͷ������ڵ�
void freeVehicleNode(Vehicle *vh)
{
	free(vh->account);
	free(vh);
}

//�����³����б�,Vehicle�����Ĵ������ɵ��߳���ɵģ����Բ���ʹ���������õ��İ�ȫ��
Vehicle* addVehicles(Vehicle *head,char *account)
{
	//��ȡip
	char* ipstr = getUserInfo(account,USER_IP);
	if(ipstr == NULL)
	{
		perror("add Vehicles error");
		return NULL;
	}
	//�ַ���ת��Ϊip
	int ip = atoi(ipstr);
	free(ipstr); //��Ҫ�ͷſռ�
			//�½���㲢����
	Vehicle *vhc = (Vehicle *)malloc(sizeof(Vehicle));
	memset(vhc,0,sizeof(Vehicle));
	//��account����ռ�
	int bytesize = strlen(account) + 1;
	vhc->account = (char *)malloc(bytesize);
	memcpy(vhc->account,account,bytesize);  //������\0Ҳ������
	vhc->ip = ip;
	vhc->label = 0;
	//�ж��Ƿ��������ӽڵ㽫��Ϊͷ���
	if(head == NULL)
		return vhc;
	//�������ӽ�㲻Ϊͷ���
	Vehicle *cur = head;
	//�ҵ����һ�����
	while(cur->next)
		cur = cur->next;
	cur->next = vhc;  //������Ϊ���һ�����
	
	//����ͷָ��
	return head;
}
//��������ṹ����,����team_id
int addTeamList(char req_num,Vehicle *vehicles)
{
	if((req_num == 0) || (vehicles == NULL))
		return -1;
	
	VehicleTeam *vt = (VehicleTeam *)malloc(sizeof(VehicleTeam)); //�½�һ�����
	memset(vt,0,sizeof(VehicleTeam));	
	//��ʼ������������Ϣ
	//vt->res_lock = PTHREAD_MUTEX_INITIALIZER;
	vt->req_num = req_num;
	vt->timer = 3; //��ʾ���ʱ��Ϊ������
	
	pthread_mutex_lock(&id_lock);
	//����id��
	vt->id = (teamID++) % MAX_LIST_NUM + 1;   //��֤��1-10000֮��
	/*�������Զ�Ϊ0*/
	
	//TeamList�Ƿ�Ϊ��
	if(TeamList == NULL)
	{
		TeamList = vt;
		pthread_mutex_unlock(&id_lock);
		return vt->id;
	}
	//TeamList��Ϊ��
	VehicleTeam *tmp = TeamList;
	while(tmp->next)
		tmp = tmp->next;
	tmp->next = vt;
	pthread_mutex_unlock(&id_lock);
	
	return vt->id;
}

//�޸�res_num��Vehicle������,����ֵΪ����ֵ����Ӧֵ�Ĳ�ֵ��-1
int setVehicleLabel(int team_id,char *account)
{
	if((team_id == 0) || (account == NULL))
		return -1;
		
	//��ѯ�б�
	pthread_mutex_lock(&id_lock);
	VehicleTeam *vt = TeamList;
	while(vt)
	{
		if(vt->id == team_id)
			break;
		vt = vt->next;
	}
	//���δ��ѯ��
	if(vt == NULL)
	{
		pthread_mutex_unlock(&id_lock);
		return -1;
	}
	
	//vehicle�б��Ĳ�������Ҫ����
	Vehicle *vh = vt->VehicleList;
	while(vh && (strcmp(vh->account,account)))
		vh = vh->next;
	if(vh == NULL)  //δ�ҵ�
	{
		pthread_mutex_unlock(&id_lock);
		return -1;
	}
	//�ҵ���,��������label��res_num
	//pthread_mutex_lock(&res_lock);
	if(vh->label == 0)  //�ж�һ�£��Է��ظ���Ӧ
		vt->res_num++;
	//pthread_mutex_unlock(&res_lock);
    //������ɣ��ͷ�team��
	int ret = vt->req_num - vt->res_num;
	pthread_mutex_unlock(&id_lock);
	vh->label = 1; //����label����Ϊ1
	return ret;
}

//ɾ��teamlist��һ����㲢�������ݿ�,������ʾҪɾ���ڵ��ǰһ���ڵ�,һ��Ҫ���������У��������
int teamInDB(VehicleTeam *preVT)
{
	VehicleTeam *vt;
	if(preVT == NULL) //ǰһ���ڵ�ΪNULL����ʾ�ýڵ�Ϊ��һ���ڵ�
	{
		//ɾ��ͷ���
		vt = TeamList;
		TeamList = TeamList->next;
	}
	else 
	{
		//ɾ����ͨ�ڵ�
		vt = preVT->next;
		preVT->next = vt->next;
	}
	//���µĲ�������Ҫ�������Գ����������Ӳ�����
	pthread_mutex_unlock(&id_lock);
	//��ͷ������ݼ������ݿ���
	int team_id = addTeam(vt->res_num,1,getCurrentTime());
	if(team_id <= 0)  //����ʧ��
	{
		pthread_mutex_lock(&id_lock);//����ټ�����������һ��
		return -1;
	}
	//���ӳ��ӳ�Ա��,�ýڵ��Ѵ��б���ɾ��������Ҫ������
	Vehicle *vh = vt->VehicleList,tmp = NULL;
	while(vh)
	{
		if(vh->label == 1) //ȷ�ϼ��복�ӵĳ���
			addTeamMember(team_id,vh->account);
		//ɾ���ýڵ�
		tmp = vh;
		vh = vh->next;
		freeVehicleNode(tmp);
	}
	//�ͷų��ӽڵ�
	free(vt);
	pthread_mutex_lock(&id_lock);  //����ټ�����������һ��
	return 0;
}

//ɾ��team�ڵ�,һ��Ҫ����������
int delTeam(VehicleTeam *preVT)
{
	VehicleTeam *vt;
	if(preVT == NULL) //ǰһ���ڵ�ΪNULL����ʾ�ýڵ�Ϊ��һ���ڵ�
	{
		//ɾ��ͷ���
		vt = TeamList;
		TeamList = TeamList->next;
	}
	else 
	{
		//ɾ����ͨ�ڵ�
		vt = preVT->next;
		preVT->next = vt->next;
	}
	pthread_mutex_unlock(&id_lock);
	//ɾ�����ӳ�Ա��,�ýڵ��Ѵ��б���ɾ��������Ҫ������
	Vehicle *vh = vt->VehicleList,tmp = NULL;
	while(vh)
	{
		//ɾ���ýڵ�
		tmp = vh;
		vh = vh->next;
		freeVehicleNode(tmp);
	}
	//�ͷų��ӽڵ�
	free(vt);
	pthread_mutex_lock(&id_lock);
	return 0;
}

//ɾ������teamlist�ṹ