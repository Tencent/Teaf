#include "ibc_oper_base.h"

int IBCOperBase::process(QModeMsg& req, char* ack, int& ack_len)
{
    ACE_DEBUG((LM_DEBUG, "[%D] IBCOperBase start to process msg\n"));
    snprintf(ack, ack_len, "%s|", (*(req.get_map()))["info"].c_str());
    
    return 0;
}

int IBCOperBase::merge(IBCRValue& rvalue, const char* new_item)
{
    ACE_DEBUG((LM_DEBUG, "[%D] IBCOperBase start to merge msg\n"));
    if (new_item == NULL || strlen(new_item) == 0)
    {
        return -1;
    }
    
    strncat(rvalue.msg, new_item, sizeof(rvalue.msg)-strlen(rvalue.msg));

    ACE_DEBUG((LM_DEBUG, "[%D] IBCOperBase merge msg succ"
        ",cnum=%d"
        "\n"
        , rvalue.cnum
        ));
    return 0;
}

int IBCOperBase::end(IBCRValue& rvalue)
{
    ACE_DEBUG((LM_DEBUG, "[%D] IBCOperBase start to end\n"));

    //to do what you need 
    
    
    ACE_DEBUG((LM_DEBUG, "[%D] IBCOperBase end succ"
        ",cnum=%d"
        "\n"
        , rvalue.cnum
        ));
    return 0;
}


