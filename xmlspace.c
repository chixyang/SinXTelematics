

/**
 * xml解析函数实现文件
 */

#include "xmlspace.h"

//所返回的文件结构体
struct ret_file{
	int fd;  //文件描述符
	char type; //文件类型
	int size; //文件大小
};

//xml文件生成的类型，内部用，所以定义在.c文件里
enum XmlType{
	XML_SUCCESS,     //成功
	XML_FAIL,        //失败
	XML_ERROR,       //错误
	XML_LOGIN,       //登录
	XML_LOGOUT,      //登出
	XML_UPDATE,      //用户信息更新
	XML_REGISTER,    //注册
	XML_UPLOAD,      //交通事件上传
	XML_EVENTACK,    //交通事件确认
	XML_CANCEL,      //取消交通事件
	XML_TEAM,        //组队请求
	XML_TEAMACK,     //组队确认
	XML_TEAMQUIT,    //退出组队
	XML_TEAMADD      //邀请入队
};

/**
 * xml解析主方法，该方法作为xml解析方法入口，调用其他方法
 * @param charStream 服务器收到的字符型xml流
 * @param size 该流大小
 * @param rf 返回的文件结构指针，当返回值为NULL且rf->size不为0时才有效, 结构体必须在传递时存在
 * @return 返回要发送的xml
 */
xmlDocPtr parseXml(char *charStream,int size,RetFile *rf,int ip)
{
	char *xmlStream = charStream;
	rf->size = 0;  //先赋为0
	xmlDocPtr doc = NULL;     //xml文件指针
	xmlNodePtr cur = NULL;    //xml节点指针

	//解析内存中的xml流
	doc = xmlParseMemory(xmlStream,size);
	if(doc == NULL)
	{
		perror("xml parse error");
		return getServerResponseXmlStream(XML_FAIL,XML_ERROR,NULL);
	}
     //检测根节点是否为所需
	cur = xmlDocGetRootElement(doc);
	if(cur == NULL || xmlStrcmp(cur->name,(const xmlChar *)"version"))
	{
		perror("xml file error at version");
		xmlFreeDoc(doc);
		return getServerResponseXmlStream(XML_FAIL,XML_ERROR,NULL);
	}
     //获取当前请求类型
	cur = cur->next;
	if(cur == NULL || xmlStrcmp(cur->name,(const xmlChar *)"request"))
	{
		perror("xml file error at request");
		xmlFreeDoc(doc);
		return getServerResponseXmlStream(XML_FAIL,XML_ERROR,NULL);
	}
	else{   //判断请求类型并分别处理
		cur = cur->xmlChildrenNode;
		//登录请求
		if(!xmlStrcmp(cur->name,(const xmlChar *)"login"))
		{
			int ret = parseLoginXml(doc,cur); 
			xmlFreeDoc(doc);
			//登录成功
			if(!ret)
				return getServerResponseXmlStream(XML_SUCCESS,XML_LOGIN,NULL);			
			//登录失败
			return getServerResponseXmlStream(XML_FAIL,XML_LOGIN,NULL);
		}
		//定时汇报自己的ip和city
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"report"))
		{
			int ret = parseReportXml(doc,cur,ip); 
			xmlFreeDoc(doc);
			return NULL;
		}
		//登出请求
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"logout"))
		{
			int ret = parseLogoutXml(doc,cur); 
			xmlFreeDoc(doc);
			//登出成功
			if(!ret)
				return getServerResponseXmlStream(XML_SUCCESS,XML_LOGOUT,NULL);			
			//登出失败
			return getServerResponseXmlStream(XML_FAIL,XML_LOGOUT,NULL);
		}
		//信息修改请求
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"update"))
		{
			int ret = parseUpdateXml(doc,cur);
			xmlFreeDoc(doc);
			//修改成功
			if(!ret)
				return getServerResponseXmlStream(XML_SUCCESS,XML_UPDATE,NULL);			
			//修改失败
			return getServerResponseXmlStream(XML_FAIL,XML_UPDATE,NULL);
		}
		//注册请求
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"register"))
		{
			int ret = parseRegisterXml(doc,cur,ip);
			xmlFreeDoc(doc);
			//修改成功
			if(!ret)
				return getServerResponseXmlStream(XML_SUCCESS,XML_REGISTER,NULL);			
			//修改失败
			return getServerResponseXmlStream(XML_FAIL,XML_REGISTER,NULL);
		}
		//交通信息的接收
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"upload"))
		{
			unsigned long long event_id = parseEventUploadXml(doc,cur);
			xmlFreeDoc(doc);
			if(event_id)
				return getServerResponseXmlStream(XML_SUCCESS,XML_UPLOAD,&event_id);
			return getServerResponseXmlStream(XML_FAIL,XML_UPLOAD,&event_id);
		}
		//用户确认交通事件
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"event-ack"))
		{
			unsigned long long event_id = parseEventAckXml(doc,cur);
			xmlFreeDoc(doc);
			if(event_id)
				return getServerResponseXmlStream(XML_SUCCESS,XML_EVENTACK,&event_id);
			return getServerResponseXmlStream(XML_FAIL,XML_EVENTACK,&event_id);
		}
		//用户取消交通信息请求
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"event-cancellation"))
		{
			unsigned long long event_id = parseCancelXml(doc,cur);
			xmlFreeDoc(doc);
			//修改成功
			if(event_id)
				return getServerResponseXmlStream(XML_SUCCESS,XML_CANCEL,&event_id);			
			//修改失败
			return getServerResponseXmlStream(XML_FAIL,XML_CANCEL,&event_id);
		}
		//交通事件详细信息请求
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"event"))
		{
			if(rf != NULL)
			{
				parseEventRequestXml(doc,cur,rf);
				xmlFreeDoc(doc);
			}
			return NULL;
		}
		//组队请求协议
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"teammem"))
		{
			int team_id = parseTeamXml(doc,cur);  //这里不需要用ull类型，因为是链表发出的
			xmlFreeDoc(doc);
			//修改成功
			if(team_id)
				return getServerResponseXmlStream(XML_SUCCESS,XML_TEAM,&team_id);			
			//修改失败
			return getServerResponseXmlStream(XML_FAIL,XML_TEAM,&team_id);
		}
		//组队确认
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"teamack"))
		{
			int ret = parseTeamAckXml(doc,cur);
			xmlFreeDoc(doc);
			//修改成功
			if(!ret)
				return getServerResponseXmlStream(XML_SUCCESS,XML_TEAMACK,NULL);			
			//修改失败
			return getServerResponseXmlStream(XML_FAIL,XML_TEAMACK,NULL);
		}
		//退出组队申请
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"teamquit"))
		{
			unsigned long long team_id = parseTeamQuitXml(doc,cur);
			xmlFreeDoc(doc);
			//修改成功
			if(team_id)
				return getServerResponseXmlStream(XML_SUCCESS,XML_TEAMQUIT,&team_id);			
			//修改失败
			return getServerResponseXmlStream(XML_FAIL,XML_TEAMQUIT,&team_id);
		}
		//邀请加入车队
		else if(!xmlStrcmp(cur->name,(const xmlChar *)"teamadd"))
		{
			unsigned long long team_id = parseTeamAddXml(doc,cur);
			xmlFreeDoc(doc);
			//修改成功
			if(team_id)
				return getServerResponseXmlStream(XML_SUCCESS,XML_TEAMADD,&team_id);			
			//修改失败
			return getServerResponseXmlStream(XML_FAIL,XML_TEAMADD,&team_id);
		}
	
    	//其他情况不做处理
		xmlFreeDoc(doc);
	    return NULL;
	}
}
		
//解析登陆xml,成功返回0，失败返回-1
int parseLoginXml(xmlDocPtr doc, xmlNodePtr node)
{
	node = node->xmlChildrenNode; //进入子节点
	if(xmlStrcmp(node->name,(const xmlChar *)"account"))
		return -1;
	xmlChar *account = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取account
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"password"))
	{
		xmlFree(account);
		return -1;
	}
	xmlChar *pwd = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取password
	//查询用户是否存在
	if(queryUser(account,pwd))
	{
		xmlFree(account);
		xmlFree(pwd);
		return -1;
	}
	xmlFree(pwd);
	//将用户登录标志位设为1
	char label = 1;
	if(updateUser(account,&label,USER_STATUS))
	{
		xmlFree(account);
		return -1;
	}
	//登录成功
	xmlFree(account);
	return 0;
}

//解析汇报ip和city
int parseReportXml(xmlDocPtr doc, xmlNodePtr node,int ip)
{
	node = node->xmlChildrenNode; //进入子节点
	if(xmlStrcmp(node->name,(const xmlChar *)"account"))
		return -1;
	xmlChar *account = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取account
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"city"))
	{
		xmlFree(account);
		return -1;
	}
	xmlChar *city = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取city
	//更新city
	if(updateUser(account,city,USER_CITY))
	{
		xmlFree(account);
		xmlFree(city);
		return -1;
	}
	xmlFree(city);
	//更新ip
	if(updateUser(account,&ip,USER_IP))
	{
		xmlFree(account);
		return -1;
	}
	//登录成功
	xmlFree(account);
	return 0;
}

//解析退出xml
int parseLogoutXml(xmlDocPtr doc, xmlNodePtr node)
{
	node = node->xmlChildrenNode; //进入子节点
	if(xmlStrcmp(node->name,(const xmlChar *)"account"))
		return -1;
	xmlChar *account = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取account
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"password"))
	{
		xmlFree(account);
		return -1;
	}
	xmlChar *pwd = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取password
	//查询用户是否存在
	if(queryUser(account,pwd))
	{
		xmlFree(account);
		xmlFree(pwd);
		return -1;
	}
	xmlFree(pwd);
	//将用户登录标志位设为0
	char label = 0;
	if(updateUser(account,&label,USER_STATUS))
	{
		xmlFree(account);
		return -1;
	}
	//登出成功
	xmlFree(account);
	return 0;
}

//解析更新个人信息xml
int parseUpdateXml(xmlDocPtr doc, xmlNodePtr node)
{
	node = node->xmlChildrenNode; //进入子节点
	if(xmlStrcmp(node->name,(const xmlChar *)"account"))
		return -1;
	xmlChar *account = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取account
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"type"))
	{
		xmlFree(account);
		return -1;
	}
	xmlChar *type = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取type
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"info"))
	{
		xmlFree(account);
		xmlFree(type);
		return -1;
	}
	xmlChar *info = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取info
	
	//更新用户信息
	if(updateUser(account,info,*((char *)type)))
	{
		xmlFree(account);
		xmlFree(type);
		xmlFree(info);
		return -1;
	}
	//更新成功
	xmlFree(account);
	xmlFree(type);
	xmlFree(info);
	return 0;
}

//解析注册请求
int parseRegisterXml(xmlDocPtr doc, xmlNodePtr node, int ip)
{
	node = node->xmlChildrenNode; //进入子节点
	if(xmlStrcmp(node->name,(const xmlChar *)"account"))
		return -1;
	xmlChar *account = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取account
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"password"))
	{
		xmlFree(account);
		return -1;
	}
	xmlChar *password = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取password
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"phone"))
	{
		xmlFree(account);
		xmlFree(password);
		return -1;
	}
	xmlChar *phone = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取phone
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"city"))
	{
		xmlFree(account);
		xmlFree(password);
		xmlFree(phone);
		return -1;
	}
	xmlChar *city = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取city
	
	//添加新用户，注意phone为长整数，传输前后要字符顺序一致
	if(addUser(account,password,city,*((unsigned long long *)phone),ip))
	{
		xmlFree(account);
		xmlFree(password);
		xmlFree(phone);
		xmlFree(city);
		return -1;
	}
	//注册成功
	xmlFree(account);
	xmlFree(password);
	xmlFree(phone);
	xmlFree(city);
	return 0;
}

//解析交通事件接收xml
unsigned long long parseEventUploadXml(xmlDocPtr doc, xmlNodePtr node)
{
	node = node->xmlChildrenNode; //进入子节点
	if(xmlStrcmp(node->name,(const xmlChar *)"account"))
		return 0ull;
	xmlChar *account = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取account
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"lat"))
	{
		xmlFree(account);
		return 0ull;
	}
	xmlChar *lat = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取lat
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"lng"))
	{
		xmlFree(account);
		xmlFree(lat);
		return 0ull;
	}
	xmlChar *lng = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取lng
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"street"))
	{
		xmlFree(account);
		xmlFree(lat);
		xmlFree(lng);
		return 0ull;
	}
	xmlChar *street = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取street
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"city"))
	{
		xmlFree(account);
		xmlFree(lat);
		xmlFree(lng);
		xmlFree(street);
		return 0ull;
	}
	xmlChar *city = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取city
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"event-type"))
	{
		xmlFree(account);
		xmlFree(lat);
		xmlFree(lng);
		xmlFree(street);
		xmlFree(city);
		return 0ull;
	}
	xmlChar *type = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取event-type
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"status"))
	{
		xmlFree(account);
		xmlFree(lat);
		xmlFree(lng);
		xmlFree(street);
		xmlFree(city);
		xmlFree(type);
		return 0ull;
	}
	xmlChar *status = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取status
	
	//获取当前时间
	int time = getCurrentTime();
	//添加事件
	unsigned long long event_id = 0ull;
	event_id = addTrafficEvent(*((char *)type),time,*((double *)lat),*((double *)lng),street,city,account,*((char *)status));
	
	//添加事件成功
	free(status);
	xmlFree(account);
	xmlFree(lat);
	xmlFree(lng);
	xmlFree(street);
	xmlFree(city);
	xmlFree(type);
	xmlFree(status);
	return event_id;
}

//解析交通事件确认xml
unsigned long long parseEventAckXml(xmlDocPtr doc, xmlNodePtr node)
{
	node = node->xmlChildrenNode; //进入子节点
	if(xmlStrcmp(node->name,(const xmlChar *)"account"))
		return 0ull;
	xmlChar *account = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取account
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"event-id"))
	{
		xmlFree(account);
		return 0ull;
	}
	xmlChar *event_id_ptr = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取event_id
	unsigned long long event_id = *((unsigned long long *)event_id_ptr);
	//获取当前时间
	int time = getCurrentTime();
	//添加事件确认信息并更新ack_num值(线程安全？)
	if(addEventAck(account,event_id,time) || incrementEventAck(event_id))
	{
		xmlFree(account);
		xmlFree(event_id_ptr);
		return 0ull;
	}
	//添加成功
	xmlFree(account);
	xmlFree(event_id_ptr);
	return event_id;
}

//解析用户取消交通事件xml
unsigned long long parseCancelXml(xmlDocPtr doc, xmlNodePtr node)
{
	node = node->xmlChildrenNode; //进入子节点
	if(xmlStrcmp(node->name,(const xmlChar *)"account"))
		return 0ull;
	xmlChar *account = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取account
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"event-id"))
	{
		xmlFree(account);
		return 0ull;
	}
	xmlChar *event_id_ptr = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取event_id
	unsigned long long event_id = *((unsigned long long *)event_id_ptr);
	xmlFree(event_id_ptr);
	
	node = node->next;
	if(xmlStrcmp(node->name,(const xmlChar *)"cancel-type"))
	{
		xmlFree(account);
		return 0ull;
	}
	xmlChar *type = xmlNodeListGetString(doc,node->xmlChildrenNode,1);  //获取type
	//获取当前时间
	int time = getCurrentTime();
	//添加事件取消信息并更新nck_num值(线程安全？)
	if(addEventCancellation(event_id,account,time,*((char *)type)) || incrementEventCancel(event_id))
	{
		xmlFree(account);
		xmlFree(type);
		return 0ull;
	}
	//添加成功
	xmlFree(account);
	xmlFree(type);
	return event_id;
}

//解析交通事件详细请求xml
int parseEventRequestXml(xmlDocPtr doc, xmlNodePtr node,RetFile *rf)
{
}

//解析组队信息请求xml
int parseTeamXml(xmlDocPtr doc, xmlNodePtr node)
{
}

//解析组队确认xml
int parseTeamAckXml(xmlDocPtr doc, xmlNodePtr node)
{
}

//解析退出组队xml
unsigned long long parseTeamQuitXml(xmlDocPtr doc, xmlNodePtr node)
{
}

//解析邀请加入对xml
unsigned long long parseTeamAddXml(xmlDocPtr doc, xmlNodePtr node)
{
}

//获取服务器回复的xml流
xmlDocPtr getServerResponseXmlStream(char type,char type,void *argu)
{
}
















                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             


