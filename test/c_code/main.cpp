#include <iostream>

using namespace std;

extern "C" {
  int func1(int i);
}

int main(){
  if(func1(5)==10)
    cout << "C++ compiles here. So does C - and they link together." << endl;
  else
    cout << "ERROR: unexpected result from C-function" << endl;
}
