// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
  
  std::cout << "\nTesting aligned allocation" << std::endl;
  const int BUFSIZE = 1610;
  
  void* FIRST = aligned_alloc(BUFSIZE, 128);
  aligned_free(FIRST);
  
  for (int i = 0; i < 1000; i++)
  {
    // allocate
    void* test = sse_alloc(BUFSIZE);
    
    // set to 0xFF
    streamset((char*) test, 0xFF, BUFSIZE);
    // verify
    for (int j = 0; j < BUFSIZE; j++)
      assert( ((unsigned char*) test)[j] == 0xFF);
    
    // allocate 2
    void* test2 = sse_alloc(BUFSIZE);
    
    // set to 0xFF
    streamcpy((char*) test2, (char*) test, BUFSIZE);
    // verify again
    for (int j = 0; j < BUFSIZE; j++)
      assert( ((unsigned char*) test2)[j] == 0xFF);
    
    // free
    aligned_free(test);
    aligned_free(test2);
  }
  std::cout << "* Alignment verification success!" << std::endl;
  
  void* LAST = aligned_alloc(BUFSIZE, 128);
  aligned_free(LAST);
  
  std::cout << "First " << FIRST << " vs Last " << LAST << std::endl;
  assert(FIRST == LAST);
  std::cout << "* Pointers matched, no missing memory space" << std::endl;
  
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
