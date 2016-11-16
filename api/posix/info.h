// -*-C++-*-
// This file is a part of the IncludeOS unikernel - www.includeos.org
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

#pragma once
#ifndef API_POSIX_INFO_H
#define API_POSIX_INFO_H

#define LINEWIDTH 80

#ifndef NO_INFO
#define INFO(FROM, TEXT, ...) printf("%13s ] " TEXT "\n", "[ " FROM, ##__VA_ARGS__)
#define INFO2(TEXT, ...) printf("%16s" TEXT "\n"," ", ##__VA_ARGS__)
#define CAPTION(TEXT) printf("\n%*s%*s\n",LINEWIDTH/2 + strlen(TEXT)/2,TEXT,LINEWIDTH/2-strlen(TEXT)/2,"")
#define CHECK(TEST, TEXT, ...) printf("%16s[%s] " TEXT "\n","", TEST ? "x" : " ",  ##__VA_ARGS__)
#define CHECKSERT(TEST, TEXT, ...) assert(TEST), CHECK(TEST, TEXT, ##__VA_ARGS__)

#else
#define INFO(X,...)
#define INFO2(X,...)
#define CAPTION(TEXT,...)
#define FILLINE(CHAR)
#define CHECK(X,...)
#endif

#endif //< ___API_INFO___
