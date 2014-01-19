//存储数据库创建、表创建、约束以及触发器等语句

/*创建数据库，默认自己编码utf-8*/
create database telematics default character set 'utf8' collate 'utf8_general_ci';

//用户账户表
create table UserAccount
(
	account varchar(20) not null primary key,
	pwd     varchar(20) not null,
	city	varchar(15) not null,    //汉字在utf8编码中占据三个字节
	phone	bigint not null,
	status	tinyint not null default 1,  //1: login 表示录入，0：logout 表示录出，当用户注册的时候处于登录状态
	honest	double not null default 0.5,     //所有人默认新注册时都是0.5
	ip	int unsigned not null            //直接存储大端法的网络地址
)engine=InnoDB default charset=utf8;

//建立account索引
create index idx_account on UserAccount(account);
//建立city索引
create index idx_city on UserAccount(city);

//交通事件表
create table TrafficEvent
(
	event_id bigint auto_increment primary key,
	event_type tinyint not null,  //'accident','congestion','administration','disaster','others'五类事件
	time	int not null,     //时间保存为int形式，记录秒数
	lat	double(15,12) not null,  //小数点前3位，小数点后12位
	lng	double(15,12) not null,
	street	varchar(30) not null,
	city	varchar(15)	not null,
	founder varchar(20) not null,
	ack_num int not null default 1,
	canceller varchar(20),
	nck_num int default 0,
	status	tinyint	not null
)engine=InnoDB default charset=utf8;

//设置auto_increment值从10000开始，10000以内的备用
alter table TrafficEvent auto_increment = 10000;

//建立event_id索引
create index idx_id_onEvent on TrafficEvent(event_id);

//交通事件确认表
create table EventAck
(
	event_id bigint not null,
	account varchar(20) not null,
	time int not null,
	//主键，外键
	primary key (event_id,account),
	foreign key (event_id) references TrafficEvent(event_id),
	foreign key (account) references UserAccount(account)
)engine=InnoDB default charset=utf8;

//建立事件ack_num自加触发器
create trigger trg_AckAdd after insert on EventAck
for each row 
begin 
update TrafficEvent set ack_num = ack_num + 1 where event_id = new.event_id
end;

//建立event_id索引
create index idx_id_onEventAck on EventAck(event_id);

//交通事件的详细描述信息表
create table EventDescription
(
	description_id bigint auto_increment primary key,
	event_id bigint not null,
	account varchar(20) not null,
	description_type tinyint not null,  //'nothing','text','image','audio','vedio'五种类型
	description varchar(100) not null, //50个字或者图片，音频，视频的绝对地址
	time int not null,
	//外键约束
	foreign key (event_id) references TrafficEvent(event_id),
	foreign key (account) references UserAccount(account)
)engine=InnoDB default charset=utf8;

//为统一，也从10000开始
alter table EventDescription auto_increment = 10000;

//建立event_id索引
create index idx_id_onDescription on EventDescription(event_id);

//交通事件撤销表
create table EventCancellation
(
	event_id bigint not null,
	account varchar(20) not null,
	time int not null,
	type tinyint not null,   //0: relieve,事件已解除 or 1: fakeness,虚假信息
	//主键，外键
	primary key (event_id,account),
	foreign key (event_id) references TrafficEvent(event_id),
	foreign key (account) references UserAccount(account)
)engine=InnoDB default charset=utf8;

//建立event_id索引
create index idx_id_onCancell on EventCancellation(event_id);

//建立事件nck_num自加触发器
create trigger trg_nckAdd after insert on EventCancellation
for each row 
begin 
update TrafficEvent set nck_num = nck_num + 1 where event_id = new.event_id
end;

//车队信息表
create table VehicleTeam
(
	team_id	int auto_increment primery key,
	num	tinyint not null,   //车队的车总数
	status	tinyint ,   //车队状态（这个状态可以不要）
	time	int not null   //车队组建时间
)engine=InnoDB default charset=utf8;

//设置自增值从10000开始，低于10000的留给数据结构使用
alter table VehicleTeam auto_increment = 10000;

//车队成员表
create table TeamMember
(
	team_id int not null,
	account varchar(20) not null,
	//主键，外键
	primary key(team_id,account),
	foreign key (team_id) references VehicleTeam(team_id),
	foreign key (account) references UserAccount(account)
)engine=InnoDB default charset=utf8;

//建立team_id索引
create index idx_id_onTeam on TeamMemeber(team_id);

//建立车队num自加触发器
create trigger trg_numAdd after insert on TeamMember
for each row 
begin 
update VehicleTeam set num = num + 1 where team_id = new.team_id
end;

//建立车队num自减触发器,减为0时要删除VehicleTeam的一个记录，这个可以吗？？？？？
create trigger trg_numMinus after delete on TeamMember
for each row 
begin 
if VehicleTeam.num = 1 where team_id = new.team_id then
	delete from VehicleTeam where team_id = new.team_id
else 
	update VehicleTeam set num = num - 1 where team_id = new.team_id
end if;
end;

