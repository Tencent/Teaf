#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plat_db_conn.h"

PlatDbConn::PlatDbConn()
{
    conn_flag_ = 0;
    memset(db_ip_, 0x0, sizeof(db_ip_));
    memset(db_user_, 0x0, sizeof(db_user_));
    memset(db_pswd_, 0x0, sizeof(db_pswd_));
    
    memset(db_, 0x0, sizeof(db_));
    port_ = 0;
    memset(unix_socket_, 0x0, sizeof(unix_socket_));
    //client_flag_ = 0; // 默认支持多结果集查询  CLIENT_MULTI_STATEMENTS 
    client_flag_ = CLIENT_MULTI_STATEMENTS;
    time_out_ = 2; // 默认2秒 

	memset(char_set_, 0x0, sizeof(char_set_));
    
    memset(err_msg_, 0x0, sizeof(err_msg_));
}

PlatDbConn::~PlatDbConn()
{
    disconnect();
}

int PlatDbConn::set_character_set(const char* character_set)
{
    //save char set info
    strncpy(char_set_, character_set, sizeof(char_set_)-1);
	
    return set_character_set();
}

int PlatDbConn::set_character_set()
{
    //char set_character_string[512] = {0};
    //snprintf(set_character_string, sizeof(set_character_string)-1, "set names %s", char_set_);
    //ret = mysql_real_query(&mysql_, set_character_string, strlen(set_character_string));
    int ret = mysql_set_character_set(&mysql_, char_set_); //v5.0以上版本支持
    return ret;
}

unsigned long PlatDbConn::escape_string(char *to, const char *from, unsigned long length)
{
    return mysql_real_escape_string(&mysql_, to, from, length); //v5.0以上版本支持
}

int PlatDbConn::exec_query(const char* sql, MYSQL_RES*& result_set)
{
    if(conn_flag_ == 0)
    {
        int ret = connect();
        if(ret != 0)
        {
            return ret;
        }
    }

    if (mysql_real_query(&mysql_, sql, strlen(sql)) != 0)
    {
        snprintf(err_msg_, sizeof(err_msg_), "PlatDbConn mysql_real_query failed:%d,%s",
            mysql_errno(&mysql_), mysql_error(&mysql_));
        //svr gone or lost connection
        if(CR_SERVER_GONE_ERROR==mysql_errno(&mysql_)
            || CR_SERVER_LOST==mysql_errno(&mysql_)
        )
        {
            disconnect(); //清理，便于下次重连
            return -1;
        }
        return mysql_errno(&mysql_);
    }

    if ((result_set = mysql_store_result(&mysql_)) == NULL)
    {
        snprintf(err_msg_, sizeof(err_msg_), "PlatDbConn mysql_store_result failed:%d,%s",
            mysql_errno(&mysql_), mysql_error(&mysql_));
        //svr gone or lost connection
        if(CR_SERVER_GONE_ERROR==mysql_errno(&mysql_)
            || CR_SERVER_LOST==mysql_errno(&mysql_)
        )
        {
            disconnect(); //清理，便于下次重连
            return -1;
        }
        return mysql_errno(&mysql_);
    }

    return 0;
}

int PlatDbConn::exec_multi_query(const char* sql, vector<MYSQL_RES*> & result_set_list)
{
    if(conn_flag_ == 0)
    {
        int ret = connect();
        if(ret != 0)
        {
            return ret;
        }
    }

    if (mysql_real_query(&mysql_, sql, strlen(sql)) != 0)
    {
        snprintf(err_msg_, sizeof(err_msg_), "PlatDbConn mysql_real_query failed:%d,%s",
            mysql_errno(&mysql_), mysql_error(&mysql_));
        //svr gone or lost connection
        if(CR_SERVER_GONE_ERROR==mysql_errno(&mysql_)
            || CR_SERVER_LOST==mysql_errno(&mysql_)
        )
        {
            disconnect(); //清理，便于下次重连
            return -1;
        }
        return mysql_errno(&mysql_);
    }

    int keep_going = 1;
    do
    {
        MYSQL_RES* result_set = mysql_store_result(&mysql_);
        if(result_set==NULL)
        {
            snprintf(err_msg_, sizeof(err_msg_), "PlatDbConn mysql_store_result failed:%d,%s",
                mysql_errno(&mysql_), mysql_error(&mysql_));
            //svr gone or lost connection
            if(CR_SERVER_GONE_ERROR==mysql_errno(&mysql_)
                || CR_SERVER_LOST==mysql_errno(&mysql_)
            )
            {
                disconnect(); //清理，便于下次重连
                return -1;
            }
            return mysql_errno(&mysql_);
        }
        else
        {
            result_set_list.push_back(result_set);
        }
        int status = mysql_next_result(&mysql_);
        if(status !=0)
        {
            keep_going = 0;
            if(status>0)
            {
                snprintf(err_msg_, sizeof(err_msg_), "PlatDbConn mysql_store_result failed:%d,%s",
                    mysql_errno(&mysql_), mysql_error(&mysql_));
            }
        }
    }while(keep_going);

    return 0;
}

int PlatDbConn::exec_update(const char* sql,
                           int& last_insert_id, int& affected_rows)
{
    if(conn_flag_ == 0)
    {
        int ret = connect();
        if(ret != 0)
        {
            return ret;
        }
    }
    
    if (mysql_real_query(&mysql_, sql, strlen(sql)) != 0)
    {
        snprintf(err_msg_, sizeof(err_msg_), "PlatDbConn mysql_real_query failed:%d,%s",
            mysql_errno(&mysql_), mysql_error(&mysql_));
        //svr gone or lost connection
        if(CR_SERVER_GONE_ERROR==mysql_errno(&mysql_)
            || CR_SERVER_LOST==mysql_errno(&mysql_)
        )
        {
            disconnect(); //清理，便于下次重连
            return -1;
        }
        
        return mysql_errno(&mysql_);
    }
    
    last_insert_id = (int)mysql_insert_id(&mysql_);
    affected_rows = (int)mysql_affected_rows(&mysql_);

    return 0;
}

int PlatDbConn::exec_trans(const vector<string>& sqls, int& last_insert_id, int& affected_rows)
{
    if(conn_flag_ == 0)
    {
        int ret = connect();
        if(ret != 0)
        {
            return ret;
        }
    }
    
	if (mysql_query(&mysql_, "start transaction;") != 0)
	{
		return mysql_errno(&mysql_);
	}
    
	for(size_t i=0; i<sqls.size(); i++)
	{
		if (mysql_query(&mysql_, sqls[i].c_str()) != 0)
		{
		    int mysql_err = mysql_errno(&mysql_);
            snprintf(err_msg_, sizeof(err_msg_), "PlatDbConn exec_trans failed,"
    			"sql=%s,errno=%d,errmsg=%s", 
    			sqls[i].c_str(), mysql_err, mysql_error(&mysql_));
			
			mysql_query(&mysql_, "rollback;");
			
            //svr gone or lost connection
            if(CR_SERVER_GONE_ERROR==mysql_err || CR_SERVER_LOST==mysql_err)
            {
                disconnect(); //清理，便于下次重连
                return -1;
            }
        
		    return mysql_err;
		}
		else if(mysql_affected_rows(&mysql_) == 0)
		{
			mysql_query(&mysql_, "rollback;");
		    affected_rows = 0;
		    return 0;
		}
        
        affected_rows += mysql_affected_rows(&mysql_);
	}
    
	if (mysql_query(&mysql_, "commit;") != 0)
	{
		return mysql_errno(&mysql_);
	}
    
	return 0 ;

}

int PlatDbConn::connect(const char* db_ip, const char* db_user, const char* db_pswd
    , unsigned int port, int timeout, const char *db, const char *unix_socket, unsigned long client_flag)
{
    //save connect info    
    strncpy(db_ip_, db_ip, sizeof(db_ip_)-1);
    strncpy(db_user_, db_user, sizeof(db_user_)-1);
    strncpy(db_pswd_, db_pswd, sizeof(db_pswd_)-1);
    if (db != NULL)
    {
        strncpy(db_, db, sizeof(db_)-1);
    }
    port_ = port;
    if (unix_socket != NULL)
    {
        strncpy(unix_socket_, unix_socket, sizeof(unix_socket_)-1);
    }    
    
    if(timeout>0) time_out_= timeout;
    
    if(client_flag!=0)client_flag_ = client_flag;
    
    return connect();
}

void PlatDbConn::disconnect()
{
    if (conn_flag_ == 1)
    {
        mysql_close(&mysql_);
        conn_flag_ = 0;
    }
}

///
int PlatDbConn::connect()
{
    //初始化，并创建 mysql 连接
    if (mysql_init(&mysql_) == NULL)
    {
        snprintf(err_msg_, sizeof(err_msg_), "PlatDbConn mysql_init error");
        return -1;
    }
    
    //设置连接超时时间
    mysql_options(&mysql_, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&time_out_);
    //设置服务器读写超时时间
    mysql_options(&mysql_, MYSQL_OPT_READ_TIMEOUT, (const char *)&time_out_);
    mysql_options(&mysql_, MYSQL_OPT_WRITE_TIMEOUT, (const char *)&time_out_);
    
    if (mysql_real_connect(&mysql_, db_ip_, db_user_, db_pswd_,
        db_, port_, NULL, client_flag_) == NULL)
    {
        snprintf(err_msg_, sizeof(err_msg_), "PlatDbConn mysql_real_connect failed,"
			"%s/%s@%s,errno=%d,errmsg=%s"
			, db_user_, db_pswd_, db_ip_
			,mysql_errno(&mysql_), mysql_error(&mysql_));
        return -1;
    }
	
	if (strlen(char_set_) != 0)
	{
	    set_character_set(); //重连的情况要设置字符集
    }

    conn_flag_ = 1; //连接成功设置状态为 1 
	return 0;
}
