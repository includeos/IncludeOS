#ifndef TESTS_H
#define TESTS_H

#include <os>

/*
  A little test framework 
  - I chose to include .cpp-files, since there's no point in having several implementations of these tests. 
*/


void test_print_hdr(const char* testname);
void test_print_result(const char* teststep, const bool passed);

void test_malloc();
void test_new();
void test_stdio();
#endif
