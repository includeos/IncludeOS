// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#define GSL_THROW_ON_CONTRACT_VIOLATION
#include <common.cxx>
#include <cstring>

#define SIZE 1000000

/**
 * This case will only run once
 **/
CASE ("0")
{
  printf("CASE 0 start\n");
  GIVEN(" 0 ")
    {
      printf("\tGIVEN 0.1 start\n");
      char* buf = (char*) malloc(SIZE);
      memset(buf, '0', SIZE);
      EXPECT(buf[SIZE-1] == '0');
      free(buf);
      printf("\tGIVEN 0 end\n");
    }
  printf("CASE 0 end\n");
}

CASE ("1: Understanding LEST scopes")
{
  printf("CASE 1 start\n");

  GIVEN (" 1 ")
    {
      /**
       * This 'GIVEN' will run once for each of the following "THEN"/"AND_THEN"/"WHEN"'s
       **/
      printf("\tGIVEN 1 start\n");
      char* buf = (char*) malloc(SIZE);
      memset(buf, '1', SIZE);
      EXPECT(buf[SIZE/2] == '1');

      THEN("THEN 1")
        {
          printf("\t\t THEN 1 start\n");
          char c = '2';
          memset(buf, c, SIZE);
          EXPECT(buf[SIZE/2] == c);
          printf("\t\t THEN 1 end\n");

          AND_THEN("You can check if any address is within that range")
            {
              printf("\t\t\t AND_THEN 1.1 begin\n");
              char c = 'a';
              memset(buf, c, SIZE);
              EXPECT(buf[SIZE/2] == c);
              printf("\t\t\t AND_THEN 1.1 end\n");
            }
        }

      AND_THEN("You can check if any address is within that range")
        {
          printf("\t\t AND_THEN 1 start\n");
          char c = '3';
          memset(buf, c, SIZE);
          EXPECT(buf[SIZE/2] == c);
          printf("\t\t AND_THEN 1 end\n");
        }

      AND_THEN("You can resize that range (but you should let the memory map do that)")
        {
          printf("\t\t AND_THEN 2 start\n");
          char c = '4';
          memset(buf, c, SIZE);
          EXPECT(buf[SIZE/2] == c);
          printf("\t\t AND_THEN 2 start\n");
        }

      /**
       * The remainder of this GIVEN will also run once after each sub-case (THEN/WHEN etc.)
       **/
      printf("\tGIVEN 1 end\n");
      free(buf);
    }

  /**
   * This will run once at the end of the previous "THEN"'s
   **/
  printf("CASE 1 end\n");
}

// A new case is a brand new test and new scope, nothing kept from the last one
CASE ("2: Understanding LEST scopes")
{
  printf("CASE 2 start\n");

  GIVEN (" 2 ")
    {
      /**
       * This 'GIVEN' will run once for each of the following "THEN"'s
       **/
      printf("\tGIVEN 1 start\n");
      char* buf = (char*) malloc(SIZE);
      memset(buf, '1', SIZE);
      EXPECT(buf[SIZE/2] == '1');

      THEN(" THEN 2.1")
        {
          printf("\t\t THEN 2.1 start\n");
          char c = '2';
          memset(buf, c, SIZE);
          EXPECT(buf[SIZE/2] == c);
          printf("\t\t THEN 2.1 end\n");

          AND_THEN("AND_THEN 2.1.1")
            {
              printf("\t\t\t AND_THEN 2.1.1 begin\n");
              char c = 'a';
              memset(buf, c, SIZE);
              EXPECT(buf[SIZE/2] == c);
              printf("\t\t\t AND_THEN 2.1.1 end\n");
            }
        }

      AND_THEN("AND_THEN 2.1")
        {
          printf("\t\t AND_THEN 2.1 start\n");
          char c = '3';
          memset(buf, c, SIZE);
          EXPECT(buf[SIZE/2] == c);
          printf("\t\t AND_THEN 2.1 end\n");
        }

      AND_THEN("AND_THEN 2.2")
        {
          printf("\t\t AND_THEN 2.2 start\n");
          char c = '4';
          memset(buf, c, SIZE);
          EXPECT(buf[SIZE/2] == c);
          printf("\t\t AND_THEN 2.2 start\n");
        }

      /**
       * The remainder of this GIVEN will also run once after each sub-case (THEN/WHEN etc.)
       **/
      printf("\tGIVEN 2 end\n");
      free(buf);
    }

  /**
   * This will run once at the end of the previous "THEN"'s
   **/
  printf("CASE 2 end\n");
}
