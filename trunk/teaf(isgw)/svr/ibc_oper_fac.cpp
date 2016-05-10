#include "ibc_oper_fac.h"
#include "ibc_oper_base.h"
#include "stat.h"
#include "pdb_prot.h"

#ifdef IBC_SVRD
#include "ibc_get_news.h" 
#include "ibc_get_glist.h"
#endif

IBCOperBase* IBCOperFac::create_oper(int cmd)
{
    IBCOperBase* oper = NULL;
    switch (cmd)
    {
    default:
        oper = new IBCOperBase;
        Stat::instance()->incre_stat(CMD_IBC_TEST);
    }
    return oper;
}
