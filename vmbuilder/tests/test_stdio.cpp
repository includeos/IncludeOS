
void test_sprintf(){
  char sprintbuf[100]{0};
  if(printf("%s Printf is working too... 55==%i\n",sprintf_ok,i) < 0 ){
    sprintf(sprintbuf,"Error: %i \n",errno);
    OS::rsprint(sprintbuf);
  }
  char str[100]{0};  
  sprintf(str,"%s sprintf-statement, with an int 55==%i \n",sprintf_ok,i);
  rsprint(str);
  
}

void test_printf(){

  printf("PRINTF ***** YEA \n");
  const char* sprintf_ok="[PASS]\t";
    int i=55;    
    
    printf("\n_end is 0x%x, *_end is 0x%x and _includeos is %x \n",_end,&_end,_includeos);
  
}

    


