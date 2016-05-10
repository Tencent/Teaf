// g++ dll_test.cpp -I./ -I/usr/local/include/ace -I../../easyace/ -I../comm/ -L. -lACE -lperson -o dll_test
#include "ace/Log_Msg.h"
#include "ace/DLL.h" 
#include "ace/ACE.h"
 
#include "dll_impl.h"

int main()
{
    while (1) 
    {
    ACE_DLL dll;
    int retval = dll.open("libperson.so", ACE_DEFAULT_SHLIB_MODE, 1);
    if (retval != 0) 
    { 
        ACE_TCHAR *dll_error = dll.error (); 
        ACE_ERROR_RETURN ((LM_ERROR, 
             ACE_TEXT ("Error in DLL Open: %s\n"), 
             dll_error ? dll_error : ACE_TEXT ("unknown error")), 
            -1); 
    }
    else
    {
        ACE_DEBUG((LM_DEBUG, "open the dll successfully!!!\n"));
    }

    //声明一个函数指针类型
    typedef person (*person_factory)(const string& name, unsigned int age);
    person_factory fac = (person_factory)dll.symbol("create_person");
    if(fac == NULL)
    {
        cout << "get symbol failed!" << endl;
        return -1;
    }
    person per = fac("ecy", 24);
    cout << "get name:" <<per.get_person_name() << endl;
    dll.close();

    sleep(5);
    
    }
    
    return 0;
}
