#include "isgw_comm.h"
#include "isgw_ack.h"
using namespace EASY_UTIL;

///
char *EASY_UTIL::prot_encode(char *dest, const char *src)
{
    unsigned int i;
	dest[0] = '\0';
	for (i = 0; i < strlen(src); i++) 
       {
            switch (src[i]) 
            {
                case '&': strncat(dest, "%26", 3);  break;
                case '=': strncat(dest, "%3D", 3);  break;
                case '|': strncat(dest, "%7C", 3);  break; //特别加上 url里面一般不处理
                case ' ': strncat(dest, "%20", 3);  break;
                case ',': strncat(dest, "%2C", 3);  break;
                case ':': strncat(dest, "%3A", 3);  break;                
                case '%': strncat(dest, "%25", 3);  break; 
                case '~': strncat(dest, "%7E", 3);  break;
                //根据需要可以增加其他常用分割符
                
                default: strncat(dest, src+i, 1); //sprintf(dest, "%s%c", dest, src[i]);
            }
	}
	return dest;
}

///过滤特殊字符
char *EASY_UTIL::prot_strim(char *dest, const char *src)
{
    unsigned int i;

    dest[0] = '\0';
    for (i = 0; i < strlen(src); i++) 
    {
        switch (src[i]) 
        {
            case '&': break; //strcat(dest, "");
            case '=': break;
            case '|': break; //特别加上 url里面一般不处理
            case '%': break; 
            case ' ': break;
            case ':': break;
            default: strncat(dest, src+i, 1);
        }
    }
    return dest;
}

int EASY_UTIL::is_valid_uin(unsigned int uin)
{
    if ( uin < 10000 )
    {
        return -1;
    }
    return 0;
}

unsigned int EASY_UTIL::get_time()
{
    return ISGWAck::instance()->get_time();
}
//返回的值单位是0.1ms
unsigned int EASY_UTIL::get_span(struct timeval *tv1, struct timeval *tv2)
{
    return ((tv2->tv_sec - tv1->tv_sec)*1000000 + tv2->tv_usec - tv1->tv_usec)/100;
}


