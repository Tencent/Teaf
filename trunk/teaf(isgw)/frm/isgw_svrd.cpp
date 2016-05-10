#include "isgw_app.h"

int ACE_TMAIN (int argc, ACE_TCHAR* argv[])
{
    ISGWApp the_app;
    if (the_app.init(argc, argv) != 0)
    {
        return -1;
    }

    return 0;
}
