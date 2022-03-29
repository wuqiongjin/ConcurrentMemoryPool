#include <iostream>
#include <unistd.h>
#include <thread>
using namespace std;

static __thread int* p = nullptr;

int main()
{
  thread t1([]{
      p = new int(10);
      while(1)
      {
        cout << this_thread::get_id() << ":" << *p << endl;
        sleep(1);  
      }
      });
  
  thread t2([]{
      p = new int(20);
      while(1)
      {
        cout << this_thread::get_id() << ":" << *p << endl;
        sleep(1);
      }
      });

  thread t3([]{
      p = new int(30);
      while(1)
      {
        cout << this_thread::get_id() << ":" << *p << endl;
        sleep(1);
      }
      });

  t1.join();
  t2.join();
  t3.join();
  return 0;
}
