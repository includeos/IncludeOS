
#include <malloc.h>

extern char _BSS_START_, _BSS_END_;

void test_malloc(){
  test_print_hdr("malloc");
  //Dynamic memory allocation, malloc
  extern char _end;
  test_print_result("&_end <= 0x800000 (no stack collision)",&_end<(char*)0x800000);
  

  void* p0=0;
  p0=malloc(50);
  test_print_result("malloc(50) valid location  :",p0 > &_BSS_END_ && p0< &_BSS_END_ + 0x100000);
    
  
  int B1=1000;
  int* p1=(int*)malloc(B1);
  p1[500]=0xBEEF1;


  int B2=5000;
  int* p2=(int*)malloc(B2);
  p2[2500]=0xBEEF2;  
  test_print_result("malloc(5000) valid location :",p2-p1 <= B1);


  int B3=10000;
  int* p3=(int*)malloc(B3);
  p3[5000]=0xBEEF3;

  test_print_result("malloc(10000) valid location :",p3-p2 <= B2);

  int B4=50000;
  int* p4=(int*)malloc(B4);
  p4[25000]=0xBEEF4;

  test_print_result("malloc(50000) valid location :",p4-p3 <= B3);

  test_print_result("malloc(1000) read/write",p1[500]==0xBEEF1);
  test_print_result("malloc(5000) read/write:",p2[2500]==0xBEEF2);
  test_print_result("malloc(10000) read/write:",p3[5000]==0xBEEF3);
  test_print_result("malloc(50000) read/write:",p4[25000]==0xBEEF4);

}
