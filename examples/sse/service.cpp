#include <os>
#include <iostream>
#include <memstream>

void sse_testing();

void Service::start()
{
  printf("\n *** Service is up - with OS Included! *** \n");
  
  printf("\nRunning some SSE tests\n");
  // SSE test
  // results in general protection fault if not enabled
  sse_testing();
  
  std::cout << "Testing SSE allocation" << std::endl;
  const int BUFSIZE = 1610;
  
  void* FIRST = stream_alloc(BUFSIZE);
  std::cout << "First pointer: " << FIRST << std::endl;
  stream_free(FIRST);
  
  for (int i = 0; i < 1000; i++)
  {
    // allocate
    void* test = stream_alloc(BUFSIZE);
    
    // set to 0xFF
    streamset((char*) test, 0xFF, BUFSIZE);
    // verify
    for (int j = 0; j < BUFSIZE; j++)
      assert( ((unsigned char*) test)[j] == 0xFF);
    
    // allocate 2
    void* test2 = stream_alloc(BUFSIZE);
    
    // set to 0xFF
    streamcpy((char*) test2, (char*) test, BUFSIZE);
    // verify again
    for (int j = 0; j < BUFSIZE; j++)
      assert( ((unsigned char*) test2)[j] == 0xFF);
    
    // free
    stream_free(test);
    stream_free(test2);
  }
  std::cout << "SSE allocation done" << std::endl;
  
  void* LAST = stream_alloc(BUFSIZE);
  std::cout << "Last pointer: " << LAST << std::endl;
  stream_free(LAST);
  
  printf("Service out! \n");
}

#include <assert.h>
#include <x86intrin.h>

void print_row4f(float* buffer)
{
	printf("[ROW] ");
	for (int i = 0; i < 4; i++)
	{
		printf("%f ", buffer[i]);
	}
	printf("\n");
}

void sse_testing()
{
	const int BUFFER_SIZE = 16;
	float sse_buffer[BUFFER_SIZE] SSE_ALIGNED;
	
	assert(SSE_VALIDATE(sse_buffer));
	printf("assertion OK: SSE buffer was aligned!\n");
	
	for (int i = 0; i < BUFFER_SIZE; i++)
		sse_buffer[i] = i;
	
	for (int i = 0; i < BUFFER_SIZE; i += 4)
		print_row4f(sse_buffer + i);
	
	__m128 xmm0 = _mm_setzero_ps();
	
	for (int i = 0; i < BUFFER_SIZE; i += 4)
	{
		__m128 xmm1 = _mm_loadu_ps(sse_buffer + i);
		xmm0 = _mm_add_ps(xmm0, xmm1);
	}
	// store result in sse_buffer[0-3]
	_mm_store_ps(sse_buffer, xmm0);
	
	// print resulting vector
	printf("SSE add result:\n");
	print_row4f(sse_buffer);
}
