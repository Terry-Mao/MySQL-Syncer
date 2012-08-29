#!/bin/bash

master_pid=/var/run/rs/rs_master.pid
slave_pid=/var/run/rs/rs_slave.pid

cur_path=$(pwd)

case $1 in
    master)
    if test "$2" == "start"
    then
        ${cur_path}/objs/rs_master -c ${cur_path}/etc/master.cf 
    elif test "$2" == "stop"
    then
        kill $(cat ${master_pid})
    else
        echo "arguments error (master|slave) (start|stop)"
    fi
        ;;
    slave)
    if test "$2" == "start"
    then
        ${cur_path}/objs/rs_slave -c ${cur_path}/etc/slave.cf 
    elif test "$2" == "stop"
    then
        kill $(cat ${slave_pid})
    else
        echo "arguments error (master|slave) (start|stop)"
    fi
        ;;
    *)
        echo "arguments error (master|slave) (start|stop)" ;;
esac

