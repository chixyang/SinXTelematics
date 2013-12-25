
/**
 * 数据库操作函数的实现
 */

#include "dbio.h"

//增加用户可信度
double honestIncerment(double *honest)
{
  if(((*honest)-0.5d) < -eps)  //小于0.5
    return ((*honest) *= 2); 
  else  //大于等于0.5
    return ((*honest) = (*honest)/2 + 0.5d);
}

//减少用户可信度
double honestDecrement(double *honest)
{
  return ((*honest) /= 2);
}

//计算事件可信度,简单的相加
double calEventHonest(double *honest1,double *honest2)
{
  return ((*honest1) + (*honest2));
}

//经纬度转换为距离
