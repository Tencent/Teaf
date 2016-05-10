#include "sys_comm.h"

int EASY_UTIL::format_match(const char *key_str, const char* value)
{
    if (NULL == key_str || NULL == value)
    {
        return -1;
    }
    const char *p = key_str, *n = value;
    char c;
    while ((c = *p++) != '\0')
    {
      switch (c)
      {
        case '%':
            {
                //%d，需过滤掉数字
                if (*(p) == 'u' || *(p) == 'd')
                {
                    p++;
                    for (; *n != '\0'; ++n)
                    {
                        if (*n >= '0' && *n <= '9')
                        {
                            continue;
                        }
                        break;
                    }
                }
            }
            break;
        default:
        {
            if ((c) !=  (*n))
            {
                return 1;
            }
            break;
        }
      }
      if (*n != '\0')
    {
          ++n;    
    }
    }

    if (*n == '\0')
    {
        return 0;
    }
    return 1;
}

/*
功能:
    将格式为elem11,elem12|elem21,elem22|....的字符串，转为map输出，第一字段为
key，第二个字段为value
输入参数:
    input: 需要转化的字符串
    form_data: 转换后的map
    flag: 排序方法，0是以第一个字段为排序依据，1是以第二个字段为排序依据
    sep1: name和value之间的分割符
    sep2: 记录间的分割符
输出参数:
    result:成功返回0，其他表示失败
*/
int EASY_UTIL::string_to_map(const string &input, NAME_VALUE &form_data, int flag, char sep1, char sep2)
{
    //std::string input = contentLength;
    std::string name, value;
    std::string::size_type pos;
    std::string::size_type oldPos = 0;

    // Parse the input in one fell swoop for efficiency
    while(true) 
    {
        // Find the '=' separating the name from its value
        pos = input.find_first_of(sep1, oldPos);

        // If no '=', we're finished
        if(std::string::npos == pos)
        {
            break;
        }

        // Decode the name
        name = input.substr(oldPos, pos - oldPos);
        oldPos = ++pos;
        
        // Find the '&' separating subsequent name/value pairs
        pos = input.find_first_of(sep2, oldPos);

        // Even if an '&' wasn't found the rest of the string is a value
        value = input.substr(oldPos, pos - oldPos);

        //生成的map按照value排序
        if(1==flag)
        {
            string tmp = value;
            tmp += sep1;
            tmp += name;
            value = name;
            name = tmp;
        }

        // Store the pair
        form_data[name] = value;
        if(std::string::npos == pos)
        {
            break;
        }

        // Update parse position
        oldPos = ++pos;
    }
    
    return 0;
}

int EASY_UTIL::string_to_map(const string &input, map<unsigned,unsigned> &form_data)
{
    //std::string input = contentLength;
    std::string name, value;
    std::string::size_type pos;
    std::string::size_type oldPos = 0;

    // Parse the input in one fell swoop for efficiency
    while(true) 
    {
        // Find the '=' separating the name from its value
        pos = input.find_first_of(',', oldPos);

        // If no '=', we're finished
        if(std::string::npos == pos)
        {
            break;
        }

        // Decode the name
        name = input.substr(oldPos, pos - oldPos);
        oldPos = ++pos;
        
        // Find the '&' separating subsequent name/value pairs
        pos = input.find_first_of('|', oldPos);

        // Even if an '&' wasn't found the rest of the string is a value
        value = input.substr(oldPos, pos - oldPos);

        // Store the pair
        unsigned u_key = atoll(name.c_str());
        unsigned u_val = atoll(value.c_str());
        if(u_key==0&&name.compare("0")!=0) continue;

        form_data[u_key] = u_val;
        
        if(std::string::npos == pos)
        {
            break;
        }

        // Update parse position
        oldPos = ++pos;
    }
    
    return 0;
}

/*
功能:
    将map中的元素格式化输出到一个字符串中，输出的格式为elem11,elem12|elem21,elem22|....
输入参数:
    output: 转化后的字符串
    form_data: 需要转换的map
    flag: 排序方法，0是以第一个字段为排序依据，1是以第二个字段为排序依据
输出参数:
    result:成功返回0，其他表示失败
*/
int EASY_UTIL::map_to_string(string &output, NAME_VALUE &form_data, int flag)
{
    //std::string input = contentLength;
    std::string name, value;

    if(0==flag)
    {
        for(NAME_VALUE::iterator it = form_data.begin(); it != form_data.end(); it++)
        {
            output += it->first;
            output += ",";
            output += it->second;
            output += "|";
        }
    }
    else if(1==flag)
    {
        for(NAME_VALUE::reverse_iterator  it = form_data.rbegin(); it != form_data.rend(); it++)
        {
            int pos = it->first.find_first_of(",");
            output += it->second;
            output += ",";
            output += it->first.substr(0, pos);
            output += "|";
        }
    }

    return 0;
}

int EASY_UTIL::map_to_string(string &output, map<unsigned,unsigned> &form_data)
{
    //std::string input = contentLength;
    std::string name, value;
    std::ostringstream outstream;

    for(map<unsigned,unsigned>::iterator it = form_data.begin(); it != form_data.end(); it++)
    {
        outstream << it->first;
        outstream << ",";
        outstream << it->second;
        outstream << "|";
    }

    output = outstream.str();

    return 0;
}

int EASY_UTIL::format_time(time_t nowtime, string &time_string
                           ,const string& format  /*= "%Y-%m-%d %H:%M:%S"*/)
{    
    // 获得当前时间
    time_t curr_time = nowtime;
    
    char time_str_buf[32] = {0};
    struct tm local_tm ;
    localtime_r(&curr_time, &local_tm);
    strftime(time_str_buf, sizeof(time_str_buf),format.c_str(), &local_tm);

    time_string = time_str_buf;

    return 0;
}

// 将字符串转为二进制的buffer数据
 int EASY_UTIL::StringToHex(char *buffer, int &len)
 {    
    len = strlen(buffer);
    int i =0;
    
    if( len %2 != 0 ) 
    {
        buffer = NULL;
        len =0;
        return -1;
    }

    char hex[128] = {0};
    for( i = 0; i*2 < len&& i<sizeof(hex); i++)
    {
        int c = 0;
        sscanf(buffer+i*2, "%02X",  &c);
        hex[i] = c;
    }

    if(i>=sizeof(hex)) 
    {
        len = 0;
        return -1;
    }

    len = i;
    for(i=0; i<len; i++)
    {
        buffer[i] = hex[i];
    }

    return 0;
 }
 //将一段内存二进制数据转为ASCII码输出
 //buffer的长度至少为257个字节
 int EASY_UTIL::HexToString(char *buffer, int &len)
 {    
    char str[257] = {0};
    int i=0;

    for( i=0; i<len && 2*i<sizeof(str)-1; i++ )
    {
        unsigned char c = buffer[i];
        sprintf(str+i*2, "%02X", c);
    }

    if(i>=len) return -1;

    for(i=0; i<strlen(str); i++)
    {
        buffer[i] = str[i];
    }
    buffer[i] = 0;

    return 0;
 }

 std::string EASY_UTIL::CharToHex(char c)
 {
        std::string sValue;                
        static char MAPX[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', 
'9', 'A', 'B', 'C', 'D', 'E', 'F' };
        sValue += MAPX[(c >> 4) & 0x0F];
        sValue += MAPX[c & 0x0F];
        return sValue;
}

char EASY_UTIL::HexToChar(const std::string & sHex)
{
        unsigned char c=0;
        for ( unsigned int i=0; i<std::min<unsigned int>(sHex.size(), (
unsigned int)2); ++i ) { 
                unsigned char c1 = std::toupper(sHex[i]);
                unsigned char c2 = (c1 >= 'A') ? (c1 - ('A' - 10)) : (c1 - '0'
);
                (c <<= 4) += c2;
        }
        return c;
}
//url code 编码
std::string EASY_UTIL::Encode(const std::string & sSrc)
{
    std::string result;
    std::string::const_iterator pos;

    for(pos = sSrc.begin(); pos != sSrc.end(); ++pos) {

            if ( *pos >= '0' && *pos <= '9' || 
                    *pos >= 'A' && *pos <= 'Z' || 
                    *pos >= 'a' && *pos <= 'z'
            ) {
                    result.append(1, *pos);
           }
            else {
                    result.append("%" + CharToHex(*pos));
           }
    }
    return result;        
}
 
//url code 解码
std::string EASY_UTIL::Decode(const std::string & sSrc)
{
    std::string result;
    std::string::const_iterator pos;

    for(pos = sSrc.begin(); pos != sSrc.end(); ++pos) {

        if ( *pos == '%' && std::distance(pos, sSrc.end()) > 2 ) {
            if ( std::tolower(*(pos+1)) == 'u' && 
                std::distance(pos, sSrc.end()) > 5 &&
                std::isxdigit(*(pos + 2)) && 
                std::isxdigit(*(pos + 3)) &&
                std::isxdigit(*(pos + 4)) &&
                std::isxdigit(*(pos + 5))
            ) {
                // string like %uABCD                        
                pos = pos +2;

                unsigned short UnicodeWord;
                UnicodeWord = HexToChar(string(pos, pos+2));
                UnicodeWord <<= 8;
                UnicodeWord += (unsigned char)HexToChar(string(pos+2, pos+4));
                //result.append(ChrsetTools::USC2GB(string((char *)&UnicodeWord, 2)));
                result.append(string((char *)&UnicodeWord, 2));

                pos = pos + 3;
            }
            else if ( std::distance(pos, sSrc.end()) > 2 &&
                std::isxdigit(*(pos + 1)) &&
                std::isxdigit(*(pos + 2))
            ) {
                // string like %AB
                ++pos;
                result.append(1, HexToChar(string(pos,pos+2)));
                ++pos;
            }
            else {
                result.append(1, '%');
            }
        }
        else {
            result.append(1, *pos);
        }
    }

    return result;        
}

// 新版的编解码函数 请大家统一使用这两个函数 
std::string EASY_UTIL::Encode_ex(const std::string& src)
{
    std::string result;
    size_t len = src.length();
    char encode_buf[4] = {0};
    for(size_t i = 0; i < len; ++i)
    {
        switch(src[i])
        {
        case '~':
        case '`':
        case '!':
        case '@':
        case '#':
        case '$':
        case '%':
        case '^':
        case '&':
        case '*':
        case '(':
        case ')':
        case '-':
        case '_':
        case '+':
        case '=':
        case '{':
        case '}':
        case '[':
        case ']':
        case '|':
        case '\\':
        case ';':
        case ':':
        case '\"':
        case '\'':
        case ',':
        case '<':
        case '>':
        case '.':
        case '?':
        case '/':
        case ' ':
        case '\n':
        case '\r':
            snprintf(encode_buf, 4, "%%%02X", src[i]);
            result.append(encode_buf);
            break;
        default:
            result.push_back(src[i]);
            break;
        }
    }

    return result;
}
// 新版的编解码函数 请大家统一使用这两个函数 
std::string EASY_UTIL::Decode_ex(const std::string& src)
{
    std::string result;
    size_t len = src.length();
    char ch;
    for(size_t i = 0; i < len; ++i)
    {
        if(src[i] == '%' && i < len - 2
            && std::isxdigit(src[i + 1]) && std::isxdigit(src[i + 2]))
        {
            sscanf(src.c_str() + i + 1, "%02hhX", &ch);
            result.push_back(ch);
            i += 2;
        }
        else
        {
            result.push_back(src[i]);
        }
    }
    
    return result;
}

 //base64编码
std::string EASY_UTIL::Base64Encode(const std::string str)
{
    std::string strRet;
    char buff[8] = {0};

    for( size_t i = 0; i < str.length(); i++ )
    {
        unsigned char c = str[i];
        sprintf(buff, "%02X", c);
        strRet += buff;
    }

    return strRet;
}
 
 //base64解码
std::string EASY_UTIL::Base64Decode(const std::string str)
{
    if( str.length() %2 != 0 ) { return "";}

    std::string strRet;

    for( size_t i = 0; i < str.length(); i+=2)

    {
        int c = 0;
        sscanf(str.substr(i,2).c_str(), "%02X",  &c);
        strRet += (unsigned char)c;
    }

    return strRet;
}

//将UTF-8编码转成GB2312
char *EASY_UTIL::Utf8ToGb2312(char *dest,int dest_len,const char*src)
{
	if(0 !=code_convert("UTF-8", "GB2312",(char*)src, strlen(src), dest, dest_len
))
	{
		return NULL;
	}
	return dest;

}

//将GB2312编码转成UTF-8:
char *EASY_UTIL::Gb2312ToUtf8(char *dest,int dest_len,const char*src)
{
	//if(0 !=code_convert("utf-8","gb2312", (char*)src, strlen(src), dest, dest_len))
	if(0 !=code_convert("GB2312","UTF-8", (char*)src, strlen(src), dest, dest_len
))
	{
		return NULL;
	}
	return dest;
}

//将UTF-8编码转成GB2312
char *EASY_UTIL::Utf8ToGb18030(char *dest,int dest_len,const char*src)
{
	if(0 !=code_convert("UTF-8", "GB18030",(char*)src, strlen(src), dest, 
dest_len))
	{
		return NULL;
	}
	return dest;

}

//将GB18030编码转成UTF-8:
char *EASY_UTIL::Gb18030ToUtf8(char *dest,int dest_len,const char*src)
{
	//if(0 !=code_convert("utf-8","gb2312", (char*)src, strlen(src), dest, dest_len))
	if(0 !=code_convert("GB18030","UTF-8", (char*)src, strlen(src), dest, 
dest_len))
	{
		return NULL;
	}
	return dest;
}

//将UTF-8的字符集转成GB2312,然后做Urlcode的字符集
string EASY_UTIL::Utf8ToUrlcode(string src)
{
    if(src.length() >= 1024)
    {
        return "";
    }
    
    char dest[1024]={0};
    string str_dest;
    if(Utf8ToGb18030(dest, sizeof(dest), src.c_str()) !=NULL)
        str_dest = dest;
    else
        return src;
    
    return Encode(str_dest);

}

//将Urlcode解码为gbk，然后转为utf的字符集
string EASY_UTIL::UrlcodeToUtf8(string src)
{
    string str_gb = Decode(src);
    
    if(str_gb.length() >= 1024)
    {
        return "";
    }
    
    char dest[1024]={0};
    Gb18030ToUtf8(dest, sizeof(dest), str_gb.c_str());
    string str_dest = dest;
    
    return str_dest;

}
int  EASY_UTIL::code_convert(const char * from_charset, const char * to_charset,char * inbuf,int inlen,char * outbuf,int outlen)
{
	iconv_t cd;
	int     rc;
	char ** pin = &inbuf;
	char ** pout = &outbuf;

	if ((cd = iconv_open(to_charset,from_charset)) ==(iconv_t)-1)
	{
		//iconv_close(cd);
	    char err_buf[256] = {0};
		//ACE_DEBUG((LM_ERROR,"[%D] iconv_open failed,errno=%d,err=%s\n", errno, strerror_r(errno, err_buf, sizeof(err_buf))));
		return  -1;
	}

    size_t s_inlen = inlen;
    size_t s_outlen = outlen;
	memset(outbuf,0,outlen);
	if (iconv(cd,pin,&s_inlen,pout,&s_outlen)==-1)
	{
		iconv_close(cd);
		//ACE_DEBUG((LM_ERROR,"[%D] iconv failed errno=%d,outlen=%d,inlen=%d,inbuf=%s\n", errno, outlen, inlen, inbuf));
		return -1;
	}

    outlen = s_outlen;
	iconv_close(cd);
    
	return 0;

}

//取模式匹配里的值，如m%d，取%d的值
int EASY_UTIL::get_format_value(const char *key_str, const char* value, std::
string &patternvalue)
{
    if (NULL == key_str || NULL == value)
    {
        return -1;
    }
    const char *p = key_str, *n = value;
    char c;
    while ((c = *p++) != '\0')
    {
      switch (c)
      {
        case '%':
            {
                //%d，需过滤掉数字
                if (*(p) == 'u' || *(p) == 'd')
                {
                    p++;
                    for (; *n != '\0'; ++n)
                    {
                        if (*n >= '0' && *n <= '9')
                        {
                            patternvalue += *n;
                            continue;
                        }
                        break;
                    }
                    n--;
                }
            }
            break;
        default:
        {
            if ((c) !=  (*n))
            {
                return 1;
            }
            break;
        }
      }
      ++n;
    }

    if (*n == '\0')
    {
        return 0;
    }
    return 0;
}

unsigned int EASY_UTIL::get_time_from_str(const char* time_str)
{
    //"%Y-%m-%d %H:%M:%S"
    const int FORMAT_LEN = 19;
    if (strlen(time_str) != FORMAT_LEN)
    {
        return 0;
    }
    
    struct tm ltm;
    memset(&ltm, 0x0, sizeof(ltm));
    
    char szTmp[16];
    memset(szTmp, 0x0, sizeof(szTmp));
    strncpy(szTmp, time_str, 4);
    ltm.tm_year = atoi(szTmp) - 1900;
    
    memset(szTmp, 0x0, sizeof(szTmp));
    strncpy(szTmp, time_str + 5, 2);
    ltm.tm_mon = atoi(szTmp) - 1;
    
    memset(szTmp, 0x0, sizeof(szTmp));
    strncpy(szTmp, time_str + 8, 2);
    ltm.tm_mday = atoi(szTmp);
    
    memset(szTmp, 0x0, sizeof(szTmp));
    strncpy(szTmp, time_str + 11, 2);
    ltm.tm_hour = atoi(szTmp);
    
    memset(szTmp, 0x0, sizeof(szTmp));
    strncpy(szTmp, time_str + 14, 2);
    ltm.tm_min = atoi(szTmp);
    
    memset(szTmp, 0x0, sizeof(szTmp));
    strncpy(szTmp, time_str + 17, 2);
    ltm.tm_sec= atoi(szTmp);
    
    return mktime(&ltm);
}

// 返回days天前0点0分的 unix time 的秒数
unsigned int EASY_UTIL::get_time_days_ago(int days)
{
    return ((((unsigned int)time(0))-days*SECONDS_ONE_DAY)/SECONDS_ONE_DAY)*SECONDS_ONE_DAY-28800;
}

//格式 YYYYMMDD
unsigned int EASY_UTIL::get_date_from_time(time_t  time)
{
    struct tm local_tm;

    localtime_r(&time, &local_tm);

    return ((local_tm.tm_year+1900)*10000+(local_tm.tm_mon+1)*100+local_tm.
tm_mday);
}

//根据被查看的次数、赞的次数、回复数以及时间，计算热点值
unsigned int EASY_UTIL::get_hot_value(int time, int checkNum, int upNum, int 
replyNum )
{
    int threshTime = get_time_days_ago(10);

	if(time <= threshTime )
		return 0;
	
	int day = (time-threshTime)/SECONDS_ONE_DAY;
    return ((checkNum + 2*upNum + 5*replyNum)*day)/10;
}
unsigned int EASY_UTIL::get_month_from_time(time_t nowtime)
{
    struct tm local_tm ;
    localtime_r(&nowtime, &local_tm);
    //year * 12 + current month
    return local_tm.tm_year * 12 + local_tm.tm_mon + 1;
}

/*
function:
 replace_tag 标签替换
in para:
 const std::string &input          //处理文本
 const std::string &startTag     //开始标签
 const std::string &endTag       //结束标签
 const std::string &replaceTag //替换的新标识，追加到文字头
out para:
 std::string &output                //返回文本
*/
int EASY_UTIL::replace_tag(const std::string &input
                                                , std::string &output 
                                                , const std::string &startTag 
                                                , const std::string &endTag   
                                                , const std::string &replaceTag
                                                )
{
    //std::string input = contentLength;
    std::string name, value;
    std::string::size_type pos;
    std::string::size_type oldPos = 0;

    std::string::size_type rightpos;
    std::string::size_type rightoldPos = 0;

    //[img]18_804720618_1269239725[/img]
    // Parse the input in one fell swoop for efficiency
    while(true) 
    {
        // Find the "[tag]"
        pos = input.find(startTag, oldPos);

        // If no found, we're finished
        if(std::string::npos == pos)
        {
            break;
        }

        // Find the "[/tag]"
        rightoldPos = pos + startTag.length();
        rightpos = input.find(endTag, rightoldPos);

        // If no found, we're finished
        if(std::string::npos == rightpos)
        {
            break;
        }

        rightoldPos = rightpos + endTag.length();
        // transfer the tag
        output = replaceTag;
        output += input.substr(0, pos);
        output += input.substr(rightoldPos);
        break;
    }

    //删除其它图片(标签)
    oldPos = 0;
    while(true) 
    {
        // Find the "[tag]"
        pos = output.find(startTag, oldPos);

        // If no found, we're finished
        if(std::string::npos == pos)
        {
            break;
        }

        // Find the "[/tag]"
        rightoldPos = pos + startTag.length();
        rightpos = output.find(endTag, rightoldPos);

        // If no found, we're finished
        if(std::string::npos == rightpos)
        {
            break;
        }

        rightoldPos = rightpos + endTag.length();
        output.replace(pos, rightoldPos, "");
        oldPos = 0;
    }
    if (output.length() <= 0)
    {
        output = input;
    }
    return 0;
}

int EASY_UTIL::get_charc_num(const string & src, const char c, int start)
{
    if (src.length() <= start)
    {
        return 0;
    }
    
    int num = 0;
    for(const char *p = src.c_str()+start; *p != '\0'; p++)
    {
        if(*p == c)
        {
            num++;
        }
    }
    return num;
}

int EASY_UTIL::get_charc_idxlist(const string & src, const char c
    , string & idxlist, const string& delim, int start)
{
    idxlist = "";
    if (src.length() <= start)
    {
        return 0;
    }
    
    int idx = 1; //返回的下标目前是从 1 开始的
    char buff[16];
    for(const char *p = src.c_str()+start; *p != '\0'; p++, idx++)
    {
        if(*p == c)
        {
            snprintf(buff, sizeof(buff), "%d", idx);            
            idxlist += buff;
            idxlist += delim;
        }
    }
    return 0;
}

/**********************************************************************
function : 从字符串中解析出ip和port
in/out para  :
    src 输入的字符串，格式为172.23.16.79:35588,172.23.16.79:35588,172.23.16.79
:35588
    host 返回的ip和port对列表
    delim src 中每个地址间的分割符，但是ip和port之间的分割符必须为:
return   :
**********************************************************************/
void EASY_UTIL::parse_host_addr(const string src, HOSTS &hosts, const string 
delim)
{
    vector<string> addr;
    
    split_ign(src, delim, addr);
    for(int i=0; i<addr.size(); i++)
    {
        vector<string> vec_str_addr;
        ADDRESS pair_addr;
        split_ign(addr[i], ":", vec_str_addr);
        if(vec_str_addr.size()==2)
        {
            pair_addr.first = vec_str_addr[0];
            pair_addr.second = atoi(vec_str_addr[1].c_str());
            hosts.push_back(pair_addr);
        }
    }
    
}

//获取nowtime中的年份值
unsigned int EASY_UTIL::get_year_from_time(time_t nowtime)
{
    struct tm local_tm ;
    localtime_r(&nowtime, &local_tm);
    return local_tm.tm_year;
}

//将字符串str_depose中的old_char替换为new_char
void EASY_UTIL::replace_char(char *str_depose, char old_char, char new_char)
{
    for(int idx=0;idx <strlen(str_depose); idx++)
    {
        if(old_char==str_depose[idx]) 
        {
            str_depose[idx] = new_char;
        }
    }
    return ;
}

//将字符串str_depose中的old_char替换为new_char
string EASY_UTIL::intos(unsigned num)
{
    char str[16];
    snprintf(str, sizeof(str), "%u", num);
    string result = str;
    return result;
}

void EASY_UTIL::Replace(string &value, string src, string dest)
{
    string ret="";
    int pos1 = 0, pos2=0;
    while(1)
    {
        pos2 = value.find(src, pos1);
        if(string::npos==pos2) 
        {
            ret += value.substr(pos1);
            break;
        }
        else
        {
            ret += value.substr(pos1, pos2-pos1);
            ret += dest;
            pos1 = pos2+src.length();
        }
    }
    value = ret;
}

//截取中文字符, 支持GB18030字符集
void EASY_UTIL::w_substr(string &src, int len, char*dst)
{
    if(src.length()<len)
    {
        snprintf(dst, len, "%s", src.c_str());
        return ;
    }
    //标识gbk字符是否闭合
    //flag=0闭合;flag>0非闭合;
    int flag=0, idx=0, pos=0;
    unsigned char t_char = 0;
    for(; pos<len&&idx<src.length(); idx++)
    {
        //空格的过滤
        if(' '==src[idx]||'\t'==src[idx]||'\n'==src[idx])     
            continue;
        //去掉全角空格,全角空格为两个连续的-95
        if(-95==src[idx])
        {
            if(idx+1<src.length()&&-95==src[idx+1]) 
            {
                idx++;
                continue;
            }
        }

        dst[pos] = src[idx];
        t_char = (unsigned char)dst[pos];
        pos++;
        if(0==flag)
        {
            if(t_char>=0x81&&t_char<=0xFE)
                flag = 1;
        }
        else if(1==flag)
        {
            if(t_char>=0x40&&t_char<=0xFE)
                flag = 0;   //GBK字符闭合
            //超出GBK的范围,可能是GB18030
            else if(t_char>=0x30&&t_char<=0x39)
                flag=2;
            else
                break;
        }
        else if(2==flag)
        {
            if(t_char>=0x40&&t_char<=0xFE)
                flag = 3;
            else 
                break;
        }
        else if(3==flag)
        {
            if(t_char>=0x30&&t_char<=0x39)
                flag=0; //GB18030闭合
            else
                break;
        }
    }
    
    if(flag) 
    {
        if(pos<len) dst[pos-flag-1] = '\0';
        else dst[pos-flag] = '\0';
    }
    else dst[pos] = '\0';
}

void EASY_UTIL::inet_ntoa_r(unsigned u_ip, char*cp, int size)
{
    unsigned char *pc_ip = (unsigned char*)&u_ip;
    snprintf(cp, size, "%u.%u.%u.%u"
        , pc_ip[3]
        , pc_ip[2]
        , pc_ip[1]
        , pc_ip[0]
        );
    return;
}

unsigned EASY_UTIL::get_roleidx_from_role(const unsigned area, const unsigned 
idx)
{
    return area*1000+(idx+1);
}

unsigned EASY_UTIL::get_area_from_roleidx(const unsigned roleidx, unsigned& 
idx)
{
    idx = roleidx%1000-1;

    return roleidx/1000;
}

// 常用hash函数
unsigned int EASY_UTIL::SDBMHash(const std::string& str)
{
    unsigned int hash = 0;
    for(std::size_t i = 0; i < str.length(); i++)
    {
        hash = str[i] + (hash << 6) + (hash << 16) - hash;
    }
    
    return hash;
}

unsigned int EASY_UTIL::DJBHash(const std::string& str)
{
    unsigned int hash = 5381;
    for(std::size_t i = 0; i < str.length(); i++)
    {
        hash = ((hash << 5) + hash) + str[i];
    }

    return hash;
}

unsigned EASY_UTIL::hash_uid(const std::string& uid)
{
    unsigned id = 0;
    if(!uid.empty() && ::isdigit(uid[0]))
    {
        id = strtoul(uid.c_str(), NULL, 10);
    }
    else
    {
        id = SDBMHash(uid);
    }

    return id;
}

// 获取指定时间的unixtime
uint32_t EASY_UTIL::get_timed_ut(int h, int m, int s, time_t cur_time)
{
    if(cur_time == 0) cur_time = time(0);
    
    struct tm ptm;
    memset(&ptm, 0x0, sizeof(struct tm));
    localtime_r((time_t*)&cur_time, &ptm);
    ptm.tm_hour = h;
    ptm.tm_min = m;
    ptm.tm_sec = s;
    return static_cast<uint32_t>(mktime(&ptm));
}

// 去掉字符串中指定字符
std::string EASY_UTIL::remove_charset(const std::string &src, char* charset)
{
    std::string result(src);
    for(size_t i = 0;i < strlen(charset);i++)
    {
        result.erase(std::remove(result.begin(), result.end(), charset[i]), result.end());
    }
    return result;
}

std::string EASY_UTIL::bin2hex(const char* data, int32_t len)
{
    if(len <= 0) return "";
    
    static char hexmap[] = {
        '0', '1', '2', '3'
        , '4', '5', '6', '7'
        , '8', '9', 'a', 'b'
        , 'c', 'd', 'e', 'f'
        };
    
    std::string result(len * 2, ' ');
    for(int32_t i = 0; i < len; ++i)
    {
        result[2*i] = hexmap[(data[i] & 0xF0) >> 4];
        result[2*i+1] = hexmap[data[i] & 0x0F];
    }
    
    return result;
}

int32_t EASY_UTIL::hex2bin(const std::string& src, char* data, int32_t& len)
{
    size_t str_len = src.size();
    if(str_len % 2 != 0 || str_len <= (2 * len)) return -1;
    
    char c, b;
    bool first = true;
    for(size_t i = 0;i < str_len;i += 2)
    {
        c = src[i];
        if((c>47) && (c<58)) c -= 48;
        else if((c>64) && (c<71)) c -= (65-10);
        else if((c>96) && (c<103)) c -= (97-10);
        else continue;
        
        if(first) b = c<<4;
        else data[i/2] = static_cast<uint8_t>(b+c);
        
        first=!first;
    }

    return 0;
}

// 获取随机字符串，默认随机长度5~9的字符串
std::string EASY_UTIL::get_random_str(int32_t num)
{
    static char base_str[65] = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz?$";
    std::string str_value = "";

    // 以当前微秒为随机种子
    struct timeval tv;
    struct timezone tz;
    ::gettimeofday(&tv, &tz);
    srand((uint32_t)tv.tv_usec);

    // 默认返回长度位于5~9之间
    if(num <= 0) num = (rand() & 0x5) + 5;
    
    for(int i = 0;i < num;i++)
    {
        str_value += base_str[rand() & 0x3f];
    }
    
    return str_value;
}

#define DIG2CHR(dig) (((dig) <= 0x09) ? ('0' + (dig)) : ('a' + (dig) - 0x0a))
#define CHR2DIG(chr) (((chr) >= '0' && (chr) <= '9') ? ((chr) - '0') : (((chr) >= 'a' && (chr) <= 'f') ? ((chr) - 'a' + 0x0a) : ((chr) - 'A' +0x0a)))

void EASY_UTIL::str2hex(const char* str, unsigned char* hex, int len)
{
    int i = 0;
    for (; i < len; i ++)
    {
        hex[i] = ((CHR2DIG(str[i * 2]) << 4) & 0xf0) + CHR2DIG(str[i * 2 + 1]);
    }
}

void EASY_UTIL::hex2str(unsigned char* data, int len, char* str)
{
    int i = 0;
    for ( ; i < len; i++)
    {
        str[i * 2] = DIG2CHR((data[i] >> 4) & 0x0f);
        str[i * 2 + 1] = DIG2CHR((data[i]) & 0x0f);
    }
    str[len * 2] = '\0';
}

void EASY_UTIL::str2upper(const char* src, char* dest)
{
    while (*src)
    {
        if (*src >= 'a' && *src <= 'z')
        {
            *dest++ = *src - 32;
        }
        else
        {
            *dest++ = *src;
        }

        src ++;
    }

    *dest = '\0';
}

void EASY_UTIL::str2lower(const char* src, char* dest)
{
    while (*src)
    {
        if (*src >= 'A' && *src <= 'Z')
        {
            *dest++ = *src + 32;
        }
        else
        {
            *dest++ = *src;
        }

        src ++;
    }

    *dest = '\0';
}

int EASY_UTIL::parse(char* str, CGI_PARAM_MAP& dest, const char minor
    , const char* major)
{
    char name[256];
    char value[4096];

    char* ptr = NULL;
    char* p = strtok_r(str, major, &ptr);
    while (p != NULL) 
    {
        memset(name, 0x0, sizeof(name));
        memset(value, 0x0, sizeof(value));

        char* s = strchr(p, minor);
        if (s != NULL)
        {
            int name_len = s - p > sizeof(name)-1 ? sizeof(name)-1 : s - p;
            strncpy(name, p, name_len);
            strcpy(value, s + 1);
            dest[name] = value;
        }
        p = strtok_r(NULL, major, &ptr);
    }

    return 0;
}

int EASY_UTIL::parse(const string &src, CGI_PARAM_MAP &dest
    , const char minor, const char major)
{
    //std::string input = contentLength;
    std::string name, value;
    std::string::size_type pos;
    std::string::size_type oldPos = 0;

    // Parse the input in one fell swoop for efficiency
    while(true) 
    {
        // Find the '=' separating the name from its value
        pos = src.find_first_of(minor, oldPos);

        // If no '=', we're finished
        if(std::string::npos == pos)
        {
            break;
        }

        // Decode the name
        name = src.substr(oldPos, pos - oldPos);
        oldPos = ++pos;
        
        // Find the '&' separating subsequent name/value pairs
        pos = src.find_first_of(major, oldPos);

        // Even if an '&' wasn't found the rest of the string is a value
        value = src.substr(oldPos, pos - oldPos);

        // Store the pair
        dest[name] = value;
        if(std::string::npos == pos)
        {
            break;
        }

        // Update parse position
        oldPos = ++pos;
    }
    
    return 0;
}

int EASY_UTIL::parse(const string &src, map<int, int> &dest
    , const char minor, const char major)
{
    //std::string input = contentLength;
    std::string name, value;
    std::string::size_type pos;
    std::string::size_type oldPos = 0;

    // Parse the input in one fell swoop for efficiency
    while(true) 
    {
        // Find the '=' separating the name from its value
        pos = src.find_first_of(minor, oldPos);

        // If no '=', we're finished
        if(std::string::npos == pos)
        {
            break;
        }

        // Decode the name
        name = src.substr(oldPos, pos - oldPos);
        oldPos = ++pos;
        
        // Find the '&' separating subsequent name/value pairs
        pos = src.find_first_of(major, oldPos);

        // Even if an '&' wasn't found the rest of the string is a value
        value = src.substr(oldPos, pos - oldPos);

        // Store the pair
        dest[atoi(name.c_str())] = atoi(value.c_str());
        if(std::string::npos == pos)
        {
            break;
        }

        // Update parse position
        oldPos = ++pos;
    }
    
    return 0;
}


///分割字符串
void EASY_UTIL::split(const string& src, const string& delim, vector<string>& dest)
{
    int pos1 = 0, pos2 = 0;
    while (true)
    {
        pos2 = src.find_first_of(delim, pos1);
        if (pos2 == -1)
        {
            dest.push_back(src.substr(pos1));
            break;
        }
        else
        {
            dest.push_back(src.substr(pos1, pos2-pos1));
        }
        pos1 = pos2 + 1;
    }
}

///分割字符串
void EASY_UTIL::split(const string& src, const string& delim, vector<unsigned int>& dest)
{
    int pos1 = 0, pos2 = 0;
    int len = src.length();
    
    while (true&&pos1<len)
    {
        pos2 = src.find_first_of(delim, pos1);
        if (pos2 == -1)
        {
            dest.push_back(atoll(src.substr(pos1).c_str()));
            break;
        }
        else if(pos2>pos1)
        {
            dest.push_back(atoll(src.substr(pos1, pos2-pos1).c_str()));
        }
        pos1 = pos2 + 1;
    }
}

/// 分割并去重 
void EASY_UTIL::split_ign(const string& src, const string& delim, set<string>& dest)
{
    int pos1 = 0, pos2 = 0;
    while (true)
    {
        pos1 = src.find_first_not_of(delim, pos2);
        if (pos1 == -1) break;
        
        pos2 = src.find_first_of(delim, pos1);
        if (pos2 == -1)
        {
            dest.insert(src.substr(pos1));
            break;
        }
        else
        {
            dest.insert(src.substr(pos1, pos2-pos1));
        }
    }
}

void EASY_UTIL::split_ign(const string& src, const string& delim, set<unsigned int>& dest)
{
    int pos1 = 0, pos2 = 0;
    while (true)
    {
        pos1 = src.find_first_not_of(delim, pos2);
        if (pos1 == -1) break;
        
        pos2 = src.find_first_of(delim, pos1);
        if (pos2 == -1)
        {
            dest.insert(atoll(src.substr(pos1).c_str()));
            break;
        }
        else
        {
            dest.insert(atoll(src.substr(pos1, pos2-pos1).c_str()));
        }
    }
}

/// 分割并去重 
void EASY_UTIL::split_ign(const string& src, const string& delim, set<int>& dest)
{
    int pos1 = 0, pos2 = 0;
    while (true)
    {
        pos1 = src.find_first_not_of(delim, pos2);
        if (pos1 == -1) break;
        
        pos2 = src.find_first_of(delim, pos1);
        if (pos2 == -1)
        {
            dest.insert(atoi(src.substr(pos1).c_str()));
            break;
        }
        else
        {
            dest.insert(atoi(src.substr(pos1, pos2-pos1).c_str()));
        }
    }
}


///分割字符串
void EASY_UTIL::split_ign(const string& src, const string& delim, vector<string>& dest)
{
    int pos1 = 0, pos2 = 0;
    while (true)
    {
        pos1 = src.find_first_not_of(delim, pos2);
        if (pos1 == -1) break;
        
        pos2 = src.find_first_of(delim, pos1);
        if (pos2 == -1)
        {
            dest.push_back(src.substr(pos1));
            break;
        }
        else
        {
            dest.push_back(src.substr(pos1, pos2-pos1));
        }
    }
}

///分割字符串
void EASY_UTIL::split_ign(const string& src, const string& delim, vector<unsigned int>& dest)
{
    int pos1 = 0, pos2 = 0;
    while (true)
    {
        pos1 = src.find_first_not_of(delim, pos2);
        if (pos1 == -1) break;
        
        pos2 = src.find_first_of(delim, pos1);
        if (pos2 == -1)
        {
            dest.push_back(atoll(src.substr(pos1).c_str()));
            break;
        }
        else
        {
            dest.push_back(atoll(src.substr(pos1, pos2-pos1).c_str()));
        }
    }
}

int EASY_UTIL::days(unsigned int unix_time)
{
    if(unix_time == 0)
    {
        unix_time = time(0); // 没传则获取当前时间
    }
    
    return unix_time/(24*3600);
}

char* EASY_UTIL::get_time_str(char* time_str)
{
    time_t t;
    t = time(0);
    tm ltm;
    memset(&ltm, 0x0, sizeof(ltm));
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", localtime_r(&t, &ltm));
    return time_str;
}

string EASY_UTIL::get_local_ip(const string& if_name)
{
    IPAddrMap ipaddr_map = get_local_ip_map();
    if ( ipaddr_map.count(if_name) == 0 )
    {
        return string("");
    }
    return ipaddr_map[if_name];
}

EASY_UTIL::IPAddrMap EASY_UTIL::get_local_ip_map()
{
    static IPAddrMap ipaddr_map;
    if ( ipaddr_map.empty() ) 
    {
        const int IP_ADDR_MAX_LEN = 128;
        struct ifconf ifc;
        char ifcbuf[8 * sizeof(struct ifreq)]={0};
        ifc.ifc_len = sizeof(ifcbuf);
        ifc.ifc_buf = ifcbuf;

        int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
        int ret = ioctl(sock_fd, SIOCGIFCONF, (void *)&ifc);
        close(sock_fd);
        
        if(ret == 0)
        {
            unsigned int j;
            struct ifreq * ifr;
            struct sockaddr_in * paddr;
            char ip[IP_ADDR_MAX_LEN];

            for (ifr=ifc.ifc_req, j=0; j<ifc.ifc_len/sizeof(struct ifreq); j++, ifr++) 
            {
                paddr = (struct sockaddr_in*)&(ifr->ifr_addr);
                if(inet_ntop(AF_INET,(void *)&paddr->sin_addr, ip, sizeof(ip)-1)<0)
                {
                    break;
                }
                ipaddr_map.insert(make_pair(string(ifr->ifr_name), string(ip)));
            }
        }
    }
    
    return ipaddr_map;
}


