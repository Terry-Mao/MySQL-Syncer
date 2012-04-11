
CC =      gcc
CPUBIT = x86_64
CFLAGS =  -pipe  -O -W -Wall -Wpointer-arith -Wno-unused-parameter -Wunused-function -Wunused-variable -Wunused-value -Werror -g \
		  -D$(CPUBIT)
LINK =	$(CC)


CORE_DEPS = src/core/rs.h \
			src/core/rs_config.h \
			src/core/rs_core.h \
			src/core/rs_errno.h \
			src/core/rs_string.h \
			src/core/rs_io.h \
			src/core/rs_log.h \
			src/core/rs_ring_buffer.h \
			src/core/rs_process.h \
			src/core/rs_conf.h \
			src/core/rs_core_info.h

CORE_INCS = -I src/core

MASTER_DEPS =  src/master/rs_master.h \
			   src/master/rs_master_info.h \
			   src/master/rs_dump_listen.h \
			   src/master/rs_request_dump.h \
			   src/master/rs_read_binlog.h \
			   src/master/rs_parse_binlog.h \
			   src/master/rs_filter_binlog.h 

MASTER_INCS = -I src/master

MASTER_DEFINE = -DMASTER -D$(CPUBIT)

SLAVE_DEPS =   src/slave/rs_slave.h \
			   src/slave/common.pb-c.h \
			   src/slave/redis.pb-c.h \
			   src/slave/rs_slave_info.h \
			   src/slave/rs_io_thread.h \
			   src/slave/rs_redis_thread.h

SLAVE_INCS = -I src/slave -I /usr/local/include -I /usr/local/include/hiredis

SLAVE_LIBS = -lhiredis -lprotobuf-c

SLAVE_DEFINE = -DSLAVE -D$(CPUBIT)

objs/rs_master:	objs/src/core/rs_master.o \
	objs/src/core/rs_errno.o \
	objs/src/core/rs_string.o \
	objs/src/core/rs_io.o \
	objs/src/core/rs_log.o \
	objs/src/core/rs_ring_buffer.o \
	objs/src/core/rs_process.o \
	objs/src/core/rs_conf.o \
	objs/src/master/rs_master.o \
	objs/src/master/rs_master_info.o \
	objs/src/master/rs_dump_listen.o \
	objs/src/master/rs_request_dump.o \
	objs/src/master/rs_read_binlog.o \
	objs/src/master/rs_parse_binlog.o \
	objs/src/master/rs_filter_binlog.o

objs/rs_slave:	objs/src/core/rs_slave.o \
	objs/src/core/rs_errno.o \
	objs/src/core/rs_string.o \
	objs/src/core/rs_io.o \
	objs/src/core/rs_log.o \
	objs/src/core/rs_ring_buffer.o \
	objs/src/core/rs_process.o \
	objs/src/core/rs_conf.o \
	objs/src/slave/rs_slave.o \
	objs/src/slave/common.pb-c.o \
	objs/src/slave/redis.pb-c.o \
	objs/src/slave/rs_slave_info.o \
	objs/src/slave/rs_io_thread.o \
	objs/src/slave/rs_redis_thread.o

master: objs/rs_master
	$(LINK) -o objs/rs_master \
		objs/src/core/rs_master.o \
		objs/src/core/rs_errno.o \
		objs/src/core/rs_string.o \
		objs/src/core/rs_io.o \
		objs/src/core/rs_log.o \
		objs/src/core/rs_ring_buffer.o \
		objs/src/core/rs_process.o \
		objs/src/core/rs_conf.o \
		objs/src/master/rs_master.o \
		objs/src/master/rs_master_info.o \
		objs/src/master/rs_dump_listen.o \
		objs/src/master/rs_request_dump.o \
		objs/src/master/rs_read_binlog.o \
		objs/src/master/rs_parse_binlog.o \
		objs/src/master/rs_filter_binlog.o \
		-lpthread -lm $(MASTER_DEFINE)

slave: objs/rs_slave
	$(LINK) -o objs/rs_slave \
		objs/src/core/rs_slave.o \
		objs/src/core/rs_errno.o \
		objs/src/core/rs_string.o \
		objs/src/core/rs_io.o \
		objs/src/core/rs_log.o \
		objs/src/core/rs_ring_buffer.o \
		objs/src/core/rs_process.o \
		objs/src/core/rs_conf.o \
		objs/src/slave/rs_slave.o \
		objs/src/slave/common.pb-c.o \
		objs/src/slave/redis.pb-c.o \
		objs/src/slave/rs_slave_info.o \
		objs/src/slave/rs_io_thread.o \
		objs/src/slave/rs_redis_thread.o \
		-lpthread -lm $(SLAVE_LIBS) $(SLAVE_DEFINE)

clean: 
	/bin/rm -rvf objs/src/* objs/rs_master


# core
objs/src/core/rs_master.o: $(CORE_DEPS) \
	src/core/rs.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o objs/src/core/rs_master.o \
		src/core/rs.c $(MASTER_DEFINE)

objs/src/core/rs_slave.o: $(CORE_DEPS) \
	src/core/rs.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o objs/src/core/rs_slave.o \
		src/core/rs.c $(SLAVE_DEFINE)

objs/src/core/rs_errno.o: $(CORE_DEPS) \
	src/core/rs_errno.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o objs/src/core/rs_errno.o \
		src/core/rs_errno.c

objs/src/core/rs_string.o: $(CORE_DEPS) \
	src/core/rs_string.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o objs/src/core/rs_string.o \
		src/core/rs_string.c

objs/src/core/rs_io.o: $(CORE_DEPS) \
	src/core/rs_io.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o objs/src/core/rs_io.o \
		src/core/rs_io.c

objs/src/core/rs_log.o: $(CORE_DEPS) \
	src/core/rs_log.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o objs/src/core/rs_log.o \
		src/core/rs_log.c

objs/src/core/rs_ring_buffer.o: $(CORE_DEPS) \
	src/core/rs_ring_buffer.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o objs/src/core/rs_ring_buffer.o \
		src/core/rs_ring_buffer.c

objs/src/core/rs_process.o: $(CORE_DEPS) \
	src/core/rs_process.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o objs/src/core/rs_process.o \
		src/core/rs_process.c

objs/src/core/rs_conf.o: $(CORE_DEPS) \
	src/core/rs_conf.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o objs/src/core/rs_conf.o \
		src/core/rs_conf.c

# master
objs/src/master/rs_master.o: $(CORE_DEPS) $(MASTER_DEPS) \
	src/master/rs_master.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(MASTER_INCS) \
		-o objs/src/master/rs_master.o \
		src/master/rs_master.c $(MASTER_DEFINE)

objs/src/master/rs_master_info.o: $(CORE_DEPS) $(MASTER_DEPS) \
	src/master/rs_master_info.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(MASTER_INCS) \
		-o objs/src/master/rs_master_info.o \
		src/master/rs_master_info.c $(MASTER_DEFINE)

objs/src/master/rs_read_binlog.o: $(CORE_DEPS) $(MASTER_DEPS) \
	src/master/rs_read_binlog.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(MASTER_INCS) \
		-o objs/src/master/rs_read_binlog.o \
		src/master/rs_read_binlog.c $(MASTER_DEFINE)

objs/src/master/rs_parse_binlog.o: $(CORE_DEPS) $(MASTER_DEPS) \
	src/master/rs_parse_binlog.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(MASTER_INCS) \
		-o objs/src/master/rs_parse_binlog.o \
		src/master/rs_parse_binlog.c $(MASTER_DEFINE)

objs/src/master/rs_filter_binlog.o: $(CORE_DEPS) $(MASTER_DEPS) \
	src/master/rs_filter_binlog.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(MASTER_INCS) \
		-o objs/src/master/rs_filter_binlog.o \
		src/master/rs_filter_binlog.c $(MASTER_DEFINE)

objs/src/master/rs_dump_listen.o: $(CORE_DEPS) $(MASTER_DEPS) \
	src/master/rs_dump_listen.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(MASTER_INCS) \
		-o objs/src/master/rs_dump_listen.o \
		src/master/rs_dump_listen.c $(MASTER_DEFINE)

objs/src/master/rs_request_dump.o: $(CORE_DEPS) $(MASTER_DEPS) \
	src/master/rs_request_dump.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(MASTER_INCS) \
		-o objs/src/master/rs_request_dump.o \
		src/master/rs_request_dump.c $(MASTER_DEFINE)

# slave
objs/src/slave/rs_slave.o: $(CORE_DEPS) $(SLAVE_DEPS) \
	src/slave/rs_slave.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(SLAVE_INCS) \
		-o objs/src/slave/rs_slave.o \
		src/slave/rs_slave.c $(SLAVE_DEFINE)

objs/src/slave/common.pb-c.o: $(CORE_DEPS) $(SLAVE_DEPS) \
	src/slave/common.pb-c.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(SLAVE_INCS) \
		-o objs/src/slave/common.pb-c.o \
		src/slave/common.pb-c.c $(SLAVE_DEFINE)

objs/src/slave/redis.pb-c.o: $(CORE_DEPS) $(SLAVE_DEPS) \
	src/slave/redis.pb-c.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(SLAVE_INCS) \
		-o objs/src/slave/redis.pb-c.o \
		src/slave/redis.pb-c.c $(SLAVE_DEFINE)


objs/src/slave/rs_slave_info.o: $(CORE_DEPS) $(SLAVE_DEPS) \
	src/slave/rs_slave_info.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(SLAVE_INCS) \
		-o objs/src/slave/rs_slave_info.o \
		src/slave/rs_slave_info.c $(SLAVE_DEFINE)

objs/src/slave/rs_io_thread.o: $(CORE_DEPS) $(SLAVE_DEPS) \
	src/slave/rs_io_thread.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(SLAVE_INCS) \
		-o objs/src/slave/rs_io_thread.o \
		src/slave/rs_io_thread.c $(SLAVE_DEFINE)

objs/src/slave/rs_redis_thread.o: $(CORE_DEPS) $(SLAVE_DEPS) \
	src/slave/rs_redis_thread.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(SLAVE_INCS) \
		-o objs/src/slave/rs_redis_thread.o \
		src/slave/rs_redis_thread.c $(SLAVE_DEFINE)

