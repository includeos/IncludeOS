
#include <stdio.h>
#include <string.h>

void test_string(){
  test_print_hdr("string.h");
  const char* s1="We have working cstrings";
  test_print_result("strcmp",strcmp(s1,"We have working cstrings")==0);
  test_print_result("strlen",strlen("We have working cstrings")==24);
  
  test_print_result("strerror",strcmp(strerror(10),"No children")==0);
  
  char* buf=(char*)malloc(100);
  memset((void*)buf,0,100);
  memcpy((void*)buf,(void*)s1,strlen(s1));
  test_print_result("memcpy",strcmp(s1,buf)==0);
  
  memset((void*)buf,'!',100);
  test_print_result("memset",buf[50]=='!');
  
  char* cat=(char*)"Strcat combines nine eggplants";
  strncpy(buf,cat,16);
  *(buf+16)=0;
  
  test_print_result("strcat",strcmp(strcat(buf,"two strings"),
				    "Strcat combines two strings")==0);
  test_print_result("strncat",
		    strcmp(strncat(buf," into one ugly mess",9),
			   "Strcat combines two strings into one")==0);

}

void test_sprintf(){

  //Setup
  test_print_hdr("sprintf");
  char sprintbuf[100];
  memset((void*)sprintbuf,0,100);
  char* adjective=(char*)"just dandy";
  sprintf(sprintbuf,"Beef is 0x%x and that's %s",0xbeef,adjective);

  //Hex & string
  test_print_result("sprintf hex and string",
		    strcmp(sprintbuf,
			   "Beef is 0xbeef and that's just dandy")==0);
  //Reset
  memset((void*)sprintbuf,0,100);
  //Int 
  sprintf(sprintbuf,"Beef is also %i",0xbeef);
  test_print_result("sprintf decimal",strcmp(sprintbuf,"Beef is also 48879")==0);

  
}

  
void test_printf(){
  test_print_hdr("printf");
  OS::rsprint("   (these has to be verified externally)\n");
  printf("\t >> printf: This is printf speaking \n");
  OS::rsprint("\t >> expect: This is printf speaking \n");
  int i=48879;
  printf("\t >> printf: Beef is 0x%x \n",i);
  OS::rsprint("\t >> expect: Beef is 0xbeef \n");
};  


