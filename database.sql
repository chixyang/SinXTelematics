
//存储数据库创建、表创建、约束以及触发器等语句

/*创建数据库，默认自己编码utf-8*/
create database telematics default character set 'utf8' collate 'utf8_general_ci';

create table UserAccount
(
	account varchar(20) not null primary key,
	pwd     varchar(20) not null,
	license char(8),
	city	varchar(20) not null,
	phone	bigint not null,
	status	char(1) not null,
	honest	bigint not null,     //四字节四字节一存，前四字节表示除以过2的个数，后四字节表示未除以2之前的个数
	ip		int unsigned not null,             //直接存储大端法的网络地址
)engine=InnoDB default charset=utf8;

create table TrafficEvent
(
	event_id bigint auto_increment primary key,
)
