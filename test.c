#ifdef USE_IO
#include <stdio.h>
#endif
int main(){
    #ifdef USE_IO
    printf("test\n");
    #endif
    
  return 1;  
}