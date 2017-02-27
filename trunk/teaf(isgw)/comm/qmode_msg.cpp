#include "qmode_msg.h"
#include <cstdio>
#include <cstdlib>

QModeMsg::QModeMsg(const int len, const char* body, unsigned int sock_fd, unsigned int sock_seq
    , unsigned int msg_seq, unsigned int prot, unsigned int time, unsigned int sock_ip
    , unsigned short port)
    : sock_fd_(sock_fd), sock_seq_(sock_seq), msg_seq_(msg_seq), prot_(prot), time_(time)
    , sock_ip_(sock_ip), sock_port_(port)
{
    cmd_ = 0;
    uin_ = 0;
    rflag_ = 0;
    memcpy(body_, body, len);
    body_len_ = len;
}

QModeMsg::QModeMsg(const char* body, unsigned int sock_fd, unsigned int sock_seq
    , unsigned int msg_seq, unsigned int prot, unsigned int time, unsigned int sock_ip
    , unsigned short port)
    : sock_fd_(sock_fd), sock_seq_(sock_seq), msg_seq_(msg_seq), prot_(prot), time_(time)
    , sock_ip_(sock_ip), sock_port_(port)
{
    cmd_ = 0;
    uin_ = 0;
    rflag_ = 0;
    gettimeofday(&tvtime_, NULL);
    //memset(body_, 0x0, sizeof(body_));
    set_body(body);

}

QModeMsg::QModeMsg(void)
{
    body_len_ = 0;
    sock_fd_ = 0;
    sock_seq_ = 0;
    msg_seq_ = 0;
    prot_ = 0;
    time_ = 0;
    sock_ip_ = 0;
    sock_port_ = 0;
    cmd_ = 0;
    uin_ = 0;
    rflag_ = 0;
    gettimeofday(&tvtime_, NULL);
}

QModeMsg::~QModeMsg()
{
	msg_map_.clear();
}

QMSG_MAP*  QModeMsg::get_map()
{
    return &msg_map_;
}

const char* QModeMsg::get_body() const
{
    return body_;
}

void QModeMsg::set_body(const char* body)
{
    if (body != NULL)
    {
        body_len_ = snprintf(body_, QMSG_MAX_LEN, "%s", body);
        if(body_len_ > QMSG_MAX_LEN) body_len_ = QMSG_MAX_LEN;
        parse_msg();
    }
}

void QModeMsg::set(const char* body, unsigned int sock_fd, unsigned int sock_seq
                   , unsigned int msg_seq, unsigned int prot, unsigned int time
                   , unsigned int sock_ip, unsigned short port)
{
    set_body(body);
    
    sock_fd_ = sock_fd;
    sock_seq_ = sock_seq;
    msg_seq_ = msg_seq;
    prot_ = prot;
    time_ = time;
    sock_ip_ = sock_ip;
    sock_port_ = port;
}

unsigned int QModeMsg::get_cmd()
{
    return cmd_;
}

unsigned int QModeMsg::get_uin()
{
    return uin_;
}

unsigned int QModeMsg::get_rflag()
{
    return rflag_;
}

int QModeMsg::get_result()
{
    char* p = strstr(body_, FIELD_NAME_RESULT); 
    if (p == NULL)
    {
        return ERROR_NO_FIELD;
    }
    
	return atoi(((msg_map_)[FIELD_NAME_RESULT]).c_str());
}

int QModeMsg::parse_msg()
{
    msg_map_.clear();
    if(body_len_>0&&'\r'==body_[body_len_-1]) body_[body_len_-1] = '\0';
    char tmp_body[body_len_+1];
    snprintf(tmp_body, sizeof(tmp_body), "%s", body_);
    
    char name[QMSG_NAME_LEN];
    char* ptr = NULL;
    char* p = strtok_r(tmp_body, QMSG_SEP, &ptr);
    while (p != NULL) 
    {
        char* s = strchr(p, '=');
        if (s != NULL)
        {
            int name_len = s - p > sizeof(name)-1 ? sizeof(name)-1 : s - p;            
            snprintf(name, name_len+1, "%s", p);
            msg_map_[name] = s + 1;
        }
        p = strtok_r(NULL, QMSG_SEP, &ptr);
    }

    cmd_ = atoi(((msg_map_)[FIELD_NAME_CMD]).c_str());
    uin_ = strtoul(((msg_map_)[FIELD_NAME_UIN]).c_str(), NULL, 10);
    rflag_ = atoi(((msg_map_)["_rflag"]).c_str());
    
    return 0;
}

unsigned int QModeMsg::get_handle()
{
    return sock_fd_;
}

unsigned int QModeMsg::get_sock_seq()
{
    return sock_seq_;
}

unsigned int QModeMsg::get_msg_seq()
{
    return msg_seq_;
}

unsigned int QModeMsg::get_prot()
{
    return prot_;
}

unsigned int QModeMsg::get_time()
{
    return time_;
}

unsigned int QModeMsg::get_sock_ip()
{
    return sock_ip_;
}

unsigned short QModeMsg::get_sock_port()
{
    return sock_port_;
}

unsigned int QModeMsg::get_body_size()
{
	return body_len_;
}

int QModeMsg::is_have(const char * field_name)
{
    if (field_name == NULL)
    {
        return ERROR_PARA_ILL;
    }
    
    char* p = strstr(body_, field_name);
    if (p==NULL)
    {
        return -1;
    }
    
    return 0;
}

