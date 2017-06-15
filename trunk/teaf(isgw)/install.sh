#!/bin/bash
# 脚本写的比较简单，在当前目录进行安装 

# 指定teaf(isgw)框架的源码根目录，根据实际环境修改 
export ISGW_HOME=/usr/local/isgw/

if [ $# -lt 1 ]
then
{
    echo "Usage:$0 <svrd_name>"
    exit 1
}
fi

SVRD_NAME=$1

mkdir bin cfg log

ln -s $ISGW_HOME/svr/$SVRD_NAME bin/
ln -s $ISGW_HOME/svr/$SVRD_NAME.ini cfg/
ln -s $ISGW_HOME/svr/svrs.ini cfg/

if [ "`pwd`" != "$ISGW_HOME" ]
then
{
	ln -s $ISGW_HOME/bin/start.sh bin/
	ln -s $ISGW_HOME/bin/keeper.sh bin/
}
fi
