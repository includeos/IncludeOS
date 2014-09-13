#include "tests/tests.h"

void test_print_hdr(const char* testname){
  OS::rsprint("\n** TESTING: [");
  OS::rsprint(testname);
  OS::rsprint("] **\n");
}

const char* str_intro="\t >> ";
const char* str_pass="[PASS]";
const char* str_fail="[FAIL]";
const int BUFSIZE=70;

void test_print_result(const char* teststep, const bool passed){
  char buf[BUFSIZE];
  for(int i=0;i<BUFSIZE;i++)
    buf[i]='.';
  buf[BUFSIZE-1]=0;
  buf[BUFSIZE-2]='\n';

  //OS::rsprint(buf);


  char* loc=buf;
  
  char* sptr=(char*)str_intro;  
  while(*sptr && loc-buf<20)
    *(loc++)=*(sptr++);
  
  sptr=(char*)teststep;  
  while(*sptr && loc-buf<70)
    *(loc++)=*(sptr++);
  
  if(passed)
    sptr=(char*)str_pass;
  else
    sptr=(char*)str_fail;

  loc=buf+BUFSIZE-8;
  while(*sptr )
    *(loc++)=*(sptr++);

  OS::rsprint(buf);
  //OS::rsprint("\n");
}

#include "tests/test_malloc.cpp"
#include "tests/test_new.cpp"
#include "tests/test_stdio_string.cpp"
//#include "tests/test_stdio.cpp"

