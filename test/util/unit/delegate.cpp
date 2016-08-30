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
#include <util/delegate.hpp>

const std::string f1(){ return "f1"; }
const std::string f2(){ return "f2"; }

CASE("A delegate can be compared with other delegates and itself")
{
  delegate<const std::string()> d{f1};
  delegate<const std::string()> d2{f2};

  // Apparently it's hard to predict symbol layout even for global functions.
  // All we care about here is that the delegates actually have a working operator<
  EXPECT((d < d2 or d2 < d));

  EXPECT(d == d);
  EXPECT(d != d2);

  EXPECT_NOT(d == d2);
  EXPECT_NOT(d == nullptr);
}

CASE("A delegate can be set to be the same as another delegate (= operator overload)")
{
  delegate<const std::string()> d{};
  delegate<const std::string()> d2{};

  EXPECT(d == d2);

  d = f1;
  d = d2;

  EXPECT(d == d2);
}

CASE("A delegate can swap values with another delegate (1)")
{
  delegate<const std::string()> d{f1};
  delegate<const std::string()> d2{f2};
  delegate<const std::string()> d3{d};

  EXPECT((d == d3 and d!= d2));

  d.swap(d2); // std::swap: the objects swap values (d becomes d2, d2 becomes d)

  EXPECT((d2 == d3 and d != d3));
}

CASE("A delegate can swap values with another delegate (2)")
{
  delegate<int()> d2 = []{ return 53; };
  delegate<int()> d3 = []{ return 35; };

  EXPECT(d2() == 53);
  EXPECT(d3() == 35);

  d2.swap(d3);

  EXPECT(d2() == 35);
  EXPECT(d3() == 53);
}

CASE("A delegate returns correct value (1)")
{
  std::vector<std::string> v = {"Something", "Something else"};
  delegate<size_t()> d = [v]{ return v.size(); };

  size_t v_size = v.size();
  size_t d_vector_size = d();

  EXPECT(v_size == 2u);
  EXPECT(v_size == d_vector_size);
}

CASE("A delegate returns correct value (2)")
{
  std::vector<std::string> v2;
  delegate<size_t()> d2 = [v2]{ return v2.size(); };

  size_t v2_size = v2.size();
  size_t d2_vector_size = d2();

  EXPECT(v2_size == 0u);
  EXPECT(v2_size == d2_vector_size);
}

CASE("A delegate returns correct value (3)")
{
  delegate<std::string(const int i)> d3 = [](const int i){ return std::to_string(i); };
  int i = 12;
  std::string d3_string = d3(i);

  EXPECT(d3_string == "12");
}

CASE("A delegate can be true or false (bool operator overload) and when it is reset it is false")
{
  GIVEN("A default initialized delegate")
  {
    delegate<void(const char* s)> d{};

    THEN("The delegate is false")
    {
      EXPECT_NOT(d);
      EXPECT(d == nullptr);
    }

    THEN("The delegate can be assigned")
      {
        d = [](const char* s){ printf("String: %s", s); };
        EXPECT(d);
      }

    WHEN("A delegate is reset")
      {
        d.reset();

        THEN("The delegate is false")
          {
            EXPECT(not d);
          }
      }
  }
  GIVEN("An null-initialized delegate throws when called")
    {
      delegate<void()> dnull{nullptr};
      EXPECT_NOT(dnull);

      std::function<void()> nofunc{};
      EXPECT_NOT(nofunc);

      try {
        nofunc();
      }catch(...){
        // Calling a zero-initialized std::function throws - so should delegates;
        EXPECT_THROWS(dnull());
      }

    }
}
