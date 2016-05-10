//g++ dll_impl.cpp -fPIC -shared -o libperson.so
#include "dll_impl.h"

person::person(const string& name, unsigned int age) : m_name(name), m_age(age)
{
    cout << m_name << " say hello!" << endl;
};

person::~person() 
{
    cout << m_name << " say byebye!" << endl;
};

string person::get_person_name() const {return m_name;}

person create_person(const string& name, unsigned int age)
{
    return person(name, age);
};
