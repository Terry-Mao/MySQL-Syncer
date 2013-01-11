#MySQL Syncer

##概述

MySQL Syncer Linux操作系统下是一套异构数据源复制的框架，基于解析MySQL binlog获取MySQL 操作数 据同步到任意数据源的服务.

##环境需求

1.  客户端必须与Mysql服务器端位于同一台服务器
2.  Mysql服务器binlog_format必须设置为row才可以使用该服务.查看Mysql服务器binlog格式命令      

        show global variables like '%binlog%';
1.   服务器必须安装redis的c客户端。如果未安装请参考如下安装步骤

		Hiredis客户端下载地址：https://github.com/antirez/hiredis/zipball/master
		[root@TEST ~]# tar zxvf antirez-hiredis-v0.10.1-0-g3cc6a7f.zip
		[root@TEST ~]# cd /data/programfiles/antirez-hiredis-857b269/
		[root@TEST ~]# make && make install
		[root@TEST ~]# echo /usr/local/lib > /etc/ld.so.conf.d/redis.conf
		[root@TEST ~]# ldconfig
		

##安装

1.  下载最新版本的MySQL Syncer

		Git 源码仓库下载: git clone https://github.com/Terry-Mao/MySQL-Syncer.git
		源码zip压缩包下载:https://github.com/Terry-Mao/MySQL-Syncer下载ZIP压缩包

1.  解压安装包  

		[root@TEST ~]# unzip MySQL-Syncer-master.zip

1.  执行安装脚本

		[root@TEST ~]# cd MySQL-Syncer-master
		64位系统执行:[root@TEST ~]# make master_64
		32位系统执行:[root@TEST ~]# make master_32

1. 配置master.cf,slave.cf,slave.info信息  
 
	4.1   主服务配置参数
	
		[配置master.cf]
		注意: $prefix为源码解压位置
		[root@TEST ~]# vim $prefix/etc/master.cf

		[配置信息]
		[core]
		# execute 主服务执行用户和用户组
		user root root

		# 主服务pid,file的路径
		pid /tmp/rs_master.pid
		
		# 守护进程,0为前台执行，1为后台执行一般设置为1
		daemon 1
		
		# 主服务的日志文件地址
		log /tmp/rs_master.log
		
		# 错误日志记录级别(3 DEBUG : 2 INFO : 1 ERR : 0)
		log.level 3
		
		# 日志调试级别 (CORE|ALLOC)
		debug.level BINLOG
		
		[master]
		
		# 监听地址
		listen.addr 127.0.0.1
		
		# 监听端口
		listen.port 1919

		# Mysql的binlog索引路径
		binlog.index /var/log/mysql/mysql-bin.index
		
		# 初始化申请内存大小
		pool.initsize 100
		
		# 服务申请使用最大系统内存 (10MB)
		pool.memsize 10485760
		
		# 内存slab class增长因子
		pool.factor 1.5
		
		# ring buffer num
		ringbuf.num 8000
		
		# send buf size
		sendbuf.size 1024000
		
		# io buf size (128MB)
		iobuf.size 10485760
		#iobuf.size 1024000
		
		# current mysql server_id
		server.id 1
		
		# max dump thread 
		dump.thread 36
			
		
	4.2   从服务配置参数

		[core] 

		# 启动用户组
		user root root
		
		# pid
		pid /tmp/rs_slave.pid
		
		# daemon
		daemon 1
		
		# log path
		log /tmp/rs_slave.log
		
		# log debug level (3 DEBUG : 2, INFO : 1, ERR : 0)
		log.level 3
		
		# debug level
		debug.level BINLOG|ALLOC
		
		[slave]
		# listen addr
		listen.addr 127.0.0.1
		
		# listen port
		listen.port 1919
		
		# slave.info path
		slave.info /tmp/slave.info
		
		# redis addr
		redis.addr 127.0.0.1
		
		# redis port
		redis.port 6379
		
		# slab init size
		pool.initsize 100
		
		# slab max memory size (10MB)
		pool.memsize 10485760
		
		# slab slab class grow factor
		pool.factor 1.5
		
		# ring buffer num
		ringbuf.num 10000
		
		# binlog save
		binlog.save 10
		
		# binlog savesec
		binlog.savesec 30
		
		# 需要处理的表
		filter.tables test.test
		
		# recv buffer length
		recvbuf.size 10485760
		
		# server ring buffer empty sleep usec
		svr.ringbuf.esusec 10000
		
		# client ring buffer empty sleep usec
		cli.ringbuf.esusec 10000

	4.3 slave.info配置参数

		binlog路径地址[通过mysql> show master status;命令查看当前binlog文件和Position]
		/var/log/mysql/mysql-bin.000001,620


1.  开启master，slave服务

	>	注意事项:  
1.在运行master slave服务之前请确定配置文件里面的目录都存在,如果不存在请执行mkdir -p 命令创建相关目录  
2.运行之前，请检查Redis的服务器IP和端口连接正常   


	    [root@TEST ~]# $prefix/rs.sh master start
	    [root@TEST ~]# $prefix/rs.sh slave start
	

1.  检查获取服务是否启动是否正常，并查看主从日志

	    tail -f /var/log/rs/rs_slave.log
		tail -f /var/log/rs/rs_master.log

		查看服务进程运行日志,如果出现相关error信息,请检查master.cf,slave.cf，slave.info的配置信息

7.  创建第一次Test测试

		进入Mysql命令行
		mysql -uroot -p
		show variables like '%log%';
		查看binlog的状态
		log_bin    | off
		如果状态为off,需要到mysql my.cnf配置文件里面去开启
		[mysqld]
		# bin log file path
		log-bin          = /var/lib/mysql/mysql-bin
		
		# bin log index filei
		log-bin-index    = /var/lib/mysql/mysql-bin.index
		修改之后重启Mysql服务,插入测试数据
		修改之后重启Mysql服务,插入测试数据
		mysql> use test
		mysql> CREATE TABLE test(id int PRIMARY KEY AUTO_INCREMENT, col varchar(10)) ENGINE = InnoDB;
		mysql> FLUSH MASTER;
		mysql> SET SESSION BINLOG_FORMAT = 'ROW';
		mysql> INSERT INTO test(col) VALUES('test');
		
		查看master,slaver进程日志
		[root@TEST ~]#tail -f /var/log/rs/rs_slave.log
		[root@TEST ~]#tail -f /var/log/rs/rs_master.log
		如果有错误信息返回,请停止slave，master进程，执行停止命令
		[root@TEST ~]# $prefix/rs.sh master stop
		[root@TEST ~]# $prefix/rs.sh slave  stop
		根据错误信息修改相关信息,然后再次启动进程
		[root@TEST ~]# $prefix/rs.sh master start
		[root@TEST ~]# $prefix/rs.sh slave  start
		进入redis-cli控制台
		redis-cli
		redis 127.0.0.1:6379> get test_1
		"test"
		如果看到这个,那恭喜你安装成功了。（：

1.  创建其他表结构的数据复制 
 
		1.请参考$prefix/src/slave/rs_mysql_test_test.c文件，你可以根据其格式编写你自己的逻辑处理代码.
		2.请去$prfix/src/slave/rs_register_tables.h 头文件中注册刚才添加表
		/* MYSQL : test.test */
		int rs_dm_test_test(rs_slave_info_t *si, char *r, uint32_t rl, char t);
		//下面为新添加的表结构
		int rs_dm_test_user(rs_slave_info_t *si, char *r, uint32_t rl, char t);
		3.修改$prefix/etc/slave.cf配置信息
		# filter tables
		filter.tables test.test 
		添加新表
		# filter tables
		filter.tables test.test test.user

1.  关闭slave服务，重新编译slave服务,启动slave服务

		[root@TEST ~]# cd $prefix 
		[root@TEST ~]# rs.sh slave  stop    
		[root@TEST ~]# make master_32 or make master_64
		[root@TEST ~]# rs.sh slave  start     

##反馈信息
如果安装,使用过程中出现问题可以将问题提交到<https://github.com/Terry-Mao/MySQL-Syncer/issues>。我们会在第一时间做出答复。最后希望大家能多多推广：-）


		
