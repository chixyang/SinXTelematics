//存储数据库创建、表创建、约束以及触发器等语句

/*创建数据库，默认自己编码utf-8*/
create database telematics default character set 'utf8' collate 'utf8_general_ci';

//用户账户数据库
create table UserAccount
(
	account varchar(20) not null primary key,
	pwd     varchar(20) not null,
	license char(8),
	city	varchar(20) not null,
	phone	bigint not null,
	status	char(1) not null,
	honest	bigint not null,     //四字节四字节一存，前四字节表示除以过2的个数，后四字节表示未除以2之前的个数
	ip		int unsigned not null            //直接存储大端法的网络地址
)engine=InnoDB default charset=utf8;

//交通事件数据库
create table TrafficEvent
(
	event_id bigint auto_increment primary key,
	event_type varchar(15) enum('accident','congestion','administration','disaster','others') not null,  //五类事件
	time	timestamp(14) default current_timestamp,     //新建记录的时候该值设置为当前时间戳
	lat		decimal(15,12) not null,  //小数点前3位，小数点后12位
	lng		decimal(15,12) not null,
	street	varchar(20) not null,
	city	varchar(10)	not null,
	status	tinyint	not null
)engine=InnoDB default charset=utf8;

//设置auto_increment值从10000开始，10000以内的留给数据结构使用
alter table TrafficEvent auto_increment = 10000;
