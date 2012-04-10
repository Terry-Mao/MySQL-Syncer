

default : build


build:
	make -f objs/Makefile master

master:
	make -f objs/Makefile master

master_x86_64:
	make -f objs/Makefile master

master_x86_32:
	make -f objs/Makefile master CPUBIT='x86_32'

slave:
	make -f objs/Makefile slave

slave_x86_64:
	make -f objs/Makefile slave

slave_x86_32:
	make -f objs/Makefile slave CPUBIT='x86_32'
