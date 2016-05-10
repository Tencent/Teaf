CXX=g++ -g
CFLAGS= -DPDB -DADMIN_OPER #-DISGW_USE_LUA #-DISGW_USE_ASY #-DISGW_USE_IBC  -DISGW_USE_DLL -DVIP_OPER -DWEBGAME_OPER 
INCLUDE= -I./ -I/usr/local/include/ace -I../easyace/ -I/usr/include/mysql/ -I../comm/ -I../frm -I../../comm/

#ISGW_USE_DLL = 1
ifdef ISGW_USE_DLL
LIBS= -L./ -L/usr/local/lib/mysql/ -L../../comm/\
      /usr/local/lib/libmysqlclient_r.a /usr/local/lib/libACE.a -lcomm -lnsl -lm -lz -lc -ldl -lpthread -lrt -lhiredis#-llua 
else
LIBS= -L./ -L/usr/local/lib/ -L../../comm/\
      -static -lmysqlclient_r -lACE -lcomm -lnsl -lm -lz -lc -ldl -lpthread -lrt -lhiredis#-llua
endif

BINARY = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
TARGET = isgw_svrd

all:$(TARGET)

$(TARGET):$(BINARY)
	$(CXX) $? -o $@ $(INCLUDE) $(CFLAGS) $(LIBS)
%.o:%.cpp
	$(CXX) -c $< -o $@ $(INCLUDE) $(CFLAGS)

conf:
	ln -s ../comm/sys_comm.cpp
	ln -s ../comm/isgw_comm.cpp
	ln -s ../comm/qmode_msg.cpp
	#ln -s ../comm/plat_db_access.cpp
	#ln -s ../comm/plat_db_conn.cpp	
	ln -s ../comm/plat_conn_mgr_ex.cpp
	ln -s ../comm/plat_conn_mgr_asy.cpp
	ln -s ../comm/stat.cpp
	ln -s ../comm/ibc_oper_base.cpp
	ln -s ../comm/isgw_oper_base.cpp
	ln -s ../comm/cmd_amount_contrl.cpp
	ln -s ../comm/isgw_task_base.cpp
	#ln -s ../comm/lua_oper.cpp
	ln -s ../comm/rds_access.cpp
	ln -s ../frm/isgw_svrd.cpp
	ln -s ../frm/isgw_app.cpp
	ln -s ../frm/isgw_intf.cpp
	ln -s ../frm/isgw_uintf.cpp
	ln -s ../frm/isgw_cintf.cpp
	ln -s ../frm/ace_sock_hdr_base.cpp
	ln -s ../frm/isgw_ack.cpp
	ln -s ../frm/isgw_mgr_svc.cpp
	ln -s ../frm/ibc_mgr_svc.cpp
	ln -s ../frm/isgw_sighdl.cpp
	ln -s ../frm/asy_task.cpp
	ln -s ../easyace/*.cpp ./

clean:
	@rm -rf ${TARGET} $(BINARY)
	
cleanconf:
	@rm -rf sys_comm.cpp isgw_uintf.cpp isgw_comm.cpp qmode_msg.cpp \
	isgw_svrd.cpp isgw_app.cpp isgw_intf.cpp ace_sock_hdr_base.cpp isgw_ack.cpp isgw_mgr_svc.cpp \
	stat.cpp ibc_oper_base.cpp ibc_mgr_svc.cpp plat_conn_mgr_ex.cpp plat_conn_mgr_asy.cpp \
	isgw_cintf.cpp isgw_oper_base.cpp cmd_amount_contrl.cpp isgw_sighdl.cpp asy_task.cpp isgw_task_base.cpp \
	ace_app.cpp ace_conf.cpp ace_object_que.cpp ace_svc.cpp lua_oper.cpp rds_access.cpp ace_object_array.cpp
