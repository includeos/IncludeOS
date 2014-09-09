#include <os>

static int calls=0;

class new_obj{
public:
  static int calls;
  new_obj(){
    calls++;
    ::calls++;
    //OS::rsprint("Constructor called\n");
  }  
};

int new_obj::calls=0;

void test_new(){
  test_print_hdr("new");
  //Dynamic memory allocation, new
  new_obj* obj1=new new_obj();
  new_obj* obj2=new new_obj();
  new_obj* obj3=new new_obj();
  
  test_print_result("Constructor should be called 3 times", 
		    new_obj::calls==3);
  test_print_result("New objects are in OK locations", 
		    &obj2 >= &obj1+sizeof(new_obj) && 
		    &obj3 >= &obj2+sizeof(new_obj));
  
  /*
  char buf[100];
  for(int i=0;i<100;i++) buf[i]='#';
  buf[99]=0;
  //sprintf(buf,"(new_obj::calls %i glob::calls %i )\n",new_obj::calls,calls);
  OS::rsprint(buf);
  */
}
