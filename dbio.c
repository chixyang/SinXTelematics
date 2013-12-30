/**
 * 数据库操作函数的实现
 */

#include "dbio.h"

//增加用户可信度
static double honestIncerment(double *honest)
{
  if(((*honest)-0.5d) < -eps)  //小于0.5
    return ((*honest) *= 2.0d); 
  else  //大于等于0.5
    return ((*honest) = (*honest)/2.0d + 0.5d);
}

//减少用户可信度
static double honestDecrement(double *honest)
{
  return ((*honest) /= 2.0d);
}

//计算事件可信度,简单的相加
static double calEventHonest(double *honest1,double *honest2)
{
  return ((*honest1) + (*honest2));
}

//角度转化为弧度
static double rad(double d)
{
  return (d * M_PI / 180.0d);
}

//经纬度转换为距离,lat表示纬度，lng表示经度，把地球当做椭圆来算
static double getDistance(double lat1,double lng1,double lat2,double lng2)
{
  double f = rad((lat1 + lat2)/2);
  double g = rad((lat1 - lat2)/2);
  double l = rad((lng1 - lng2)/2);
        
  double sg = sin(g);
  double sl = sin(l);
  double sf = sin(f);
        
  double s,c,w,r,d,h1,h2;
  double a = EARTH_RADIUS;
  double fl = 1/298.257;
        
  sg = sg*sg;
  sl = sl*sl;
  sf = sf*sf;
        
  s = sg*(1-sl) + (1-sf)*sl;
  c = (1-sg)*(1-sl) + sf*sl;
        
  w = atan(sqrt(s/c));
  r = sqrt(s*c)/w;
  d = 2*w*a;
  h1 = (3*r -1)/2/c;
  h2 = (3*r +1)/2/s;
        
  return d*(1 + fl*(h1*sf*(1-sg) - h2*(1-sf)*sg));
}

//打开数据库
int db_init(int link_num)
{
  int ret = dbpool_init(link_num);
  if(ret <= 0)
    return -1;
  
  return 0;
}

//用户新建账户








