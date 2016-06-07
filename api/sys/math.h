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

#ifndef SYS_MATH_H
#define SYS_MATH_H


// Long double math stubs
long double cosl(long double);
long double sinl(long double);
long double tanl(long double);

long double acosl(long double);
long double asinl(long double);
long double atanl(long double);
long double atan2l(long double, long double);


long double coshl(long double);
long double sinhl(long double);
long double tanhl(long double);
long double acoshl(long double);
long double asinhl(long double);
long double atanhl(long double);


long double ceill(long double);
long double coshl(long double);
long double fabsl(long double);
long double floorl(long double);
long double fmodl(long double, long double);
long double logl(long double);
long double log10l(long double);
long double modfl(long double, long double*);
long double powl(long double,long double);
long double frexpl(long double, int*);

long double cbrtl(long double);
long double copysignl(long double, long double);
long double erfl(long double);
long double erfcl(long double);
long double exp2l(long double);
long double expm1l(long double);
long double fdiml(long double, long double);
long double fmal(long double, long double, long double);
long double fmaxl(long double, long double);
long double fminl(long double, long double);

long double expl(long double);
long double ldexpl(long double, long double);

long double ilogbl(long double);
long double lgammal(long double);
long double llroundl(long double);
long double log1pl(long double);
long double log2l(long double);
long double logbl(long double);
long double lroundl(long double);
long double nearbyintl(long double);
long double nextafterl(long double, long double);
double nexttoward(double, long double);
long double nexttowardl(long double, long double);
long double nexttowardf(long double, long double);

long double remainderl(long double, long double);
long double remquol(long double, long double, int*);
long double roundl(long double);
long double scalblnl(long double, long double);
long double scalbnl(long double, long double);
long double tgammal(long double);
long double truncl(long double);
long double nanl(const char*);


#endif

#include_next <math.h>
