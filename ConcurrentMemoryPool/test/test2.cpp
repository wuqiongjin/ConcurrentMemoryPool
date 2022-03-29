#include <iostream>
#include <cxxabi.h>
#include <typeinfo>
using namespace std;
#if defined (_WIN64) //64位下_WIN64和_WIN32都有，32位下只有_WIN32
	typedef unsigned long long PAGE_ID;
#elif defined (_WIN32)
	typedef size_t PAGE_ID;
#elif defined(__x86_64)
	//linux
	typedef unsigned long long PAGE_ID;
#elif defined (__i386__)
	typedef size_t PAGE_ID;
#endif


int main()
{
  int status;
  cout << abi::__cxa_demangle(typeid(PAGE_ID).name(), 0, 0, &status) << endl;
  cout << abi::__cxa_demangle(typeid(int).name(), 0, 0, &status) << endl;
  //cout << typeid(int).name() << endl;
  return 0;
}
