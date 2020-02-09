#ifdef USE_IO
#include <iostream>
#endif
int main(){
    #ifdef USE_IO
    std::cout<<"test"<<std::endl;
    #endif
  return 1;  
}