#include <os>
#include <class_dev.hpp>

void sse_testing();

void Service::start(){
  printf("\n *** Service is up - with OS Included! *** \n");
  
  printf("\n Running some SSE Examples \n");
  // SSE test
  // results in general protection fault if not enabled
  sse_testing();
  
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
	#define SSE  __attribute__ (( aligned (16) ))
	#define SSE_VALIDATE(buffer)  (assert(((intptr_t) sse_buffer & 15) == 0))
	
	const int BUFFER_SIZE = 16;
	float sse_buffer[BUFFER_SIZE] SSE;
	
	SSE_VALIDATE(sse_buffer);
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
