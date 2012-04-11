

default : build


build:
	make -f objs/Makefile master_x86_64

clean:
	make -f objs/Makefile clean

master_x86_64:
	/bin/mkdir -p objs/src/core
	/bin/mkdir -p objs/src/master
	make -f objs/Makefile master

master_x86_32:
	/bin/mkdir -p objs/src/core
	/bin/mkdir -p objs/src/master
	make -f objs/Makefile master CPUBIT='x86_32'

slave_x86_64:
	/bin/mkdir -p objs/src/core
	/bin/mkdir -p objs/src/slave
	make -f objs/Makefile slave

slave_x86_32:
	/bin/mkdir -p objs/src/core
	/bin/mkdir -p objs/src/slave
	make -f objs/Makefile slave CPUBIT='x86_32'
