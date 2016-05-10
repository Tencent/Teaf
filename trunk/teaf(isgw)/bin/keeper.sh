#!/bin/bash
###############################################################################
# function:监控指定的进程
# trait	  :
# author:away
# history: 
# init 2005-09-02
#      2006-07-26 同时可以监控多个进程
#      2008-05-23 如果有命令行参数，以命令行参数为准
# 
###############################################################################
#监控进程数
KEEP_COUNT=2 

#第一项进程信息，如果有其他的，依次填写
#进程名和启动路径及监控的时间间隔
PROCESS_NAME[0]="isgw_svrd"
START_PAHT[0]="/usr/local/isgw/bin/start.sh isgw_svrd"
INTERVAL[0]="10"
LOG_PATH[0]="/usr/local/isgw/bin/keeper.log"

PROCESS_NAME[1]="isgw_svrd"
START_PAHT[1]="/usr/local/isgw/bin/start.sh isgw_svrd"
INTERVAL[1]="10"
LOG_PATH[1]="/usr/local/isgw/bin/keeper.log"

# 命令行参数只支持监控一个进程
if [ $# -gt 0 ]
then
{
    KEEP_COUNT=1
    PROCESS_NAME[0]="$1"
    START_PAHT[0]="$2"
    INTERVAL[0]="$3"
    LOG_PATH[0]="$4"
}
fi

###############################################################################
# desc		:
# input		:
# output	:
###############################################################################
main()
{
    typeset i=0
    while [ $i -lt ${KEEP_COUNT} ]
    do    
	    keep_proc "${PROCESS_NAME[$i]}" "${START_PAHT[$i]}" "${INTERVAL[$i]}" "${LOG_PATH[$i]}"&
	    i=`expr $i + 1`
    done

    return $?
}

###############################################################################
. /usr/local/shcom/tools.inc

cd ${0%/*}

#echo `pwd`

main "$@"
exit $?
