if [ $# -lt 1 ]
then
{
    echo "Usage:$0 <svrd_name>"
    exit 1
}
fi

SVRD_NAME=$1
export ISGW_HOME=../
export ISGW_BIN=${ISGW_HOME}/bin/
export ISGW_CFG=${ISGW_HOME}/cfg/
export LD_LIBRARY_PATH=./

# for itc oper
export SYS_CONF_PATH=${ISGW_CFG}
export SYS_SOCKET_TIMEOUT=2

ulimit -c unlimited           #产生core文件，不限制core文件大小
ulimit -n 20480               #修改连接句柄数 
ulimit -s 2048                #修改线程的堆栈大小，避免单个线程数使用内存太多

#valgrind --tool=memcheck --leak-check=full --track-origins=yes --show-reachable=yes 
${ISGW_BIN}/${SVRD_NAME} 1>>${ISGW_BIN}/start.log 2>&1 &
