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

#include <common.cxx>
#include <util/fixedvec.hpp>

/// notes:
/// given that fixed vector is a template class, even with templated size
/// there are a number of restrictions that "could" be put on the template-
/// variations, however there is none: Because it would add completely useless
/// code to an intentionally small template class. The only valid fixed vector, 
/// is one where the type is trivial (POD) and N is a numerical value.

struct POD
{
  POD() {}
  POD(int A, int B) : a(A), b(B) {}
  int a;
  int b;
};

CASE("A new fixed vector of type int is empty")
{
  fixedvector<POD, 10> fv;
  
  EXPECT(fv.empty());
  EXPECT(fv.size() == 0);
}
CASE("A new fixed vector has a sane capacity")
{
  fixedvector<POD, 10> fv;
  
  EXPECT(fv.capacity() == 10);
  EXPECT(fv.free_capacity());
}
CASE("Adding elements normally")
{
  fixedvector<POD, 10> fv;
  
  POD pod(1, 2);
  fv.add(pod);
  EXPECT(fv.size() == 1);
  fv.add(pod);
  EXPECT(fv.size() == 2);
  EXPECT(fv.free_capacity());
}
CASE("Adding elements by emplacing")
{
  fixedvector<POD, 10> fv;
  
  fv.emplace(1, 2);
  EXPECT(fv.size() == 1);
  EXPECT(fv[0].a == 1);
  EXPECT(fv[0].b == 2);
  
  fv.emplace(10, 20);
  EXPECT(fv.size() == 2);
  EXPECT(fv[1].a == 10);
  EXPECT(fv[1].b == 20);
  
  EXPECT(fv.free_capacity());
}
CASE("Clearing a fixed vector")
{
  fixedvector<POD, 10> fv;
  
  for (int N = 0; N < 2; N++)
  {
    for (int i = 0; i < fv.capacity(); i++)
      fv.emplace(5, 6);
    
    EXPECT(fv.free_capacity() == false);
    fv.clear();
    EXPECT(fv.empty());
    EXPECT(fv.size() == 0);
    EXPECT(fv.free_capacity());
  }
}
CASE("Adding elements increases size")
{
  fixedvector<POD, 10> fv;
  
  for (int N = 0; N < 2; N++)
  {
    for (int i = 0; i < fv.capacity(); i++) {
      EXPECT(fv.size() == i);
      fv.emplace(5, 6);
      EXPECT(fv.size() == i + 1);
      // verify POD too
      EXPECT(fv[i].a == 5);
      EXPECT(fv[i].b == 6);
    }
    EXPECT(fv.free_capacity() == false);
    fv.clear();
  }
}
CASE("A cloned fixed vector matches bit for bit")
{
  fixedvector<POD, 10> fv1;
  fixedvector<POD, 10> fv2;
  
  EXPECT(fv1.capacity() == fv2.capacity());
  
  for (int i = 0; i < fv1.capacity(); i++) {
    EXPECT(fv1.size() == i);
    fv1.emplace(5, 6);
    EXPECT(fv1.size() == i + 1);
  }
  EXPECT(fv1.free_capacity() == false);
  
  // copy into fv2
  fv2.copy(fv1.first(), fv1.size());
  // expect same size
  EXPECT(fv2.empty() == fv1.empty());
  EXPECT(fv2.size() == fv1.size());
  // expect all elements to be equal
  for (int i = 0; i < fv1.capacity(); i++) {
    EXPECT(fv2[i].a == fv1[i].a);
    EXPECT(fv2[i].b == fv1[i].b);
  }
  
}
