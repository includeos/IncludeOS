
/**
 * Example from http://www.yolinux.com/TUTORIALS/LinuxTutorialPosixThreads.html
 * Fetched Apr. 25th. 2017
 * Modifications to compile w. C++ compiler
 * - added 'int' before main
 * - Added return value to print message function
 **/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *print_message_function( void *ptr );

int main()
{
  pthread_t thread1, thread2;
  const char *message1 = "Thread 1";
  const char *message2 = "Thread 2";
  int  iret1, iret2;

  /* Create independent threads each of which will execute function */

  iret1 = pthread_create( &thread1, NULL, print_message_function, (void*) message1);
  if(iret1)
    {
      fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
      exit(EXIT_FAILURE);
    }

  iret2 = pthread_create( &thread2, NULL, print_message_function, (void*) message2);
  if(iret2)
    {
      fprintf(stderr,"Error - pthread_create() return code: %d\n",iret2);
      exit(EXIT_FAILURE);
    }

  printf("pthread_create() for thread 1 returns: %d\n",iret1);
  printf("pthread_create() for thread 2 returns: %d\n",iret2);

  /* Wait till threads are complete before main continues. Unless we  */
  /* wait we run the risk of executing an exit which will terminate   */
  /* the process and all threads before the threads have completed.   */

  void* retval1 = NULL;
  void* retval2 = NULL;

  pthread_join( thread1, &retval1);
  pthread_join( thread2, &retval2);

  printf("Thread 1 returned: %li \n", (long)retval1);
  printf("Thread 2 returned: %li \n", (long)retval2);

  exit(EXIT_SUCCESS);
}

int ret = 0;

void *print_message_function( void *ptr )
{
  static int i = 41;
  i++;

  char *message;
  message = (char *) ptr;
  printf("Thread message: %s Returning: %i \n", message, i);

  return (void*)i;

}
