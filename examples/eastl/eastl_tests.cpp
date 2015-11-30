// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
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
#include <net/inet>

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <deque>

#include <signal>
#include <delegate>
#include <sort>
#include <vector_map>
#include <initializer_list>
#include <tuple.hpp>

#include <cassert>
#include "test.hpp"

class TestStream
{
public:
  friend std::ostream& operator << (std::ostream& out, const TestStream& test);
};
std::ostream& operator << (std::ostream& out, const TestStream&)
{
  return out << std::string("Test Stream");
}
TestStream testStream;

struct PrintNum
{
    void operator()(int i) const
    {
        std::cout << i << std::endl;
    }
};
void testFunction()
{
  std::cout << "called void testFunction()" << std::endl;
}

namespace std
{
  template <typename F, typename... Args>
  class bind
  {
  private:
    std::function<F> func;
    std::tuple<Args...> elem;
    
  public:
    bind(F function, Args... args)
      : func(function), elem(args...)
    {}
    
    void operator() ()
    {
      //func(elem...);
    }
  };
}

// test showing that constructor is called only once for emplace_back
struct EmplTest
{
  EmplTest()
  {
    std::cout << "EmplTest Constructor " << ++cnt << std::endl;
  }
  EmplTest(const EmplTest&)
  {
    std::cout << "EmplTest CopyConstr " << ++cnt << std::endl;
  }
  
  static int cnt;
};
int EmplTest::cnt = 0;

void tests()
{
  std::function<int* ()> 
  testLambda = []
  {
    std::cout << "std::function<int* ()> called" << std::endl;
    return nullptr;
  };
  
  struct Song
  {
    Song(std::string art, std::string tit)
      : artist(art), title(tit) {}
    
    std::string artist;
    std::string title;
  };
  
  /////////////////////////////////////////////////////////////////////////////
  //// std::ostream, std::istream
  /////////////////////////////////////////////////////////////////////////////
  {
    eastl::string str("test string");
    std::cout << str << " int: " << 52 << " long: " << 52L << std::endl;
    std::cout << "short: " << (short)52 << " char: " << 'C' << std::endl;
    std::cout << "pointer: " << &str << std::endl;
    uintptr_t* long_pointer = (uintptr_t*) UINTPTR_MAX;
    std::cout << "pointer: " << long_pointer << std::endl;
    printf("compare: %p\n", long_pointer);
    
    std::cout << "class: " << testStream << std::endl;
    
    std::cin >> str;
    std::cout << "You wrote: " << str << std::endl;
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::vector
  /////////////////////////////////////////////////////////////////////////////
  {
    std::cout << "checking std::vector..." << std::endl;
    std::vector<int> vec;
    vec.resize(500);
    vec[0] = 500;
    vec[499] = 500;
    
    assert(vec[499] == 500);
    vec.resize(1);
    assert(vec[0] == 500);
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::vector::emplace_back, std::vector::emplace
  /////////////////////////////////////////////////////////////////////////////
  {
    std::cout << "checking std::vector::emplace..." << std::endl;
    std::vector<EmplTest> empl_test;
    EmplTest et;
    std::cout << "creating (count should be 1)" << std::endl;
    assert(EmplTest::cnt == 1);
    
    std::cout << "emplacing (count should be 2)" << std::endl;
    empl_test.emplace_back();
    assert(EmplTest::cnt == 2);
    
    std::cout << "pushing (count should be 4)" << std::endl;
    empl_test.push_back(et);
    assert(EmplTest::cnt == 4);
    
    struct IndexTest
    {
      IndexTest(int i)
        : index(i) {}
      
      IndexTest(const IndexTest& i)
        : index(i.index) {}
      
      IndexTest& operator= (const IndexTest& i)
      {
        this->index = i.index;
        return *this;
      }
      
      int index;
    };
    
    std::vector<IndexTest> idx;
    
    for (int i = 0; i < 20; i++)
      idx.emplace_back(i);
    
    std::cout << "Testing vector::emplace" << std::endl;
    idx.clear();
    
    for (int i = 0; i < 20; i++)
      idx.emplace_back(i);
    
    idx.emplace(idx.begin(), 20);
    
    std::cout << "checking if emplace is working" << std::endl;
    for (unsigned i = 0; i < idx.size(); i++)
    {
      assert(idx[i].index == (int)((20 + i) % 21));
      //std::cout << "idx[i] == " << idx[i].index << std::endl;
    }
    
    std::cout << "emplacing at begin()" << std::endl;
    idx.clear();
    
    for (int i = 0; i < 20; i++)
      idx.emplace(idx.begin(), i);
    
    for (unsigned i = 0; i < idx.size(); i++)
    {
      //std::cout << "idx[i] == " << idx[i].index << std::endl;
      assert(19 - idx[i].index == (int) i);
    }
    assert(idx[0].index == 19);
    
    std::cout << "std::vector passed" << std::endl;
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::list<T>
  /////////////////////////////////////////////////////////////////////////////
  {
    std::cout << "checking std::list..." << std::endl;
    std::list<int> testList;
    
    testList.push_back(1);
    
    assert(testList.front() == 1);
    std::cout << "std::list passed" << std::endl;
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::map<K, V>
  /////////////////////////////////////////////////////////////////////////////
  {
    std::cout << "checking std::map..." << std::endl;
    
    std::map<int, int> testMap;
    
    // there should be no such element
    assert(testMap.find(1) == testMap.end());
    
    // insert the key 1 with value 1
    testMap[1] = 1;
    assert(testMap[1] == 1);
    
    // verify that it is the first and only key
    assert(testMap.find(1) == testMap.begin());
    
    testMap[1] = 2;
    assert(testMap[1] == 2);
    
    std::cout << "std::map passed" << std::endl;
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::map::emplace
  /////////////////////////////////////////////////////////////////////////////
  {
    static int cnt;
    struct ETEST
    {
      ETEST()
      {
        cnt++;
      }
      ETEST(ETEST&)
      {
        cnt++;
      }
    };
    
    std::cout << "checking std::map::emplace..." << std::endl;
    std::map<int, int> empl_test;
    
    assert(cnt == 0);
    
    std::cout << "emplacing (count should be 1)" << std::endl;
    empl_test.emplace(1, 2);
    assert(EmplTest::cnt == 1);
    
    std::cout << "std::map::emplace passed" << std::endl;
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::deque<T>
  /////////////////////////////////////////////////////////////////////////////
  {
    std::cout << "checking std::deque..." << std::endl;
    
    /// testing deque<int>
    std::deque<int> testInt;
    
    /// testing back
    std::cout << "Pushing back to testInt - size: " << testInt.size() << std::endl;
    for (int i = 0; i < 20; i++)
    {
      testInt.push_back(i);
      assert(testInt.back() == i);
    }
    std::cout << "New size: " << testInt.size() << std::endl;
    
    std::cout << "Popping from back from testInt - size: " << testInt.size() << std::endl;
    while (testInt.size())
      testInt.pop_back();
    std::cout << "New size: " << testInt.size() << std::endl;
    
    /// testing front
    std::cout << "Testing push_front" << std::endl;
    
    for (int i = 0; i < 20; i++)
    {
      testInt.push_front(i);
      assert(testInt.front() == i);
    }
    
    std::cout << "Testing pop_front (size: " << testInt.size() << ")" << std::endl;
    
    for (int i = 0; i < 20; i++)
    {
      assert(testInt.front() == 19-i);
      ///
      testInt.pop_front();
      ///
      if (testInt.size())
        assert(testInt.front() == 18-i);
      else
        assert(testInt.empty());
    }
    // size must be 0
    assert(testInt.size() == 0);
    // must be empty
    assert(testInt.empty());
    
    /// resize ///
    std::cout << "Testing resize (size: " << testInt.size() << ")" << std::endl;
    testInt.resize(40, 999);
    
    std::cout << "After resize n=40 v=999 (size: " << testInt.size() << ")" << std::endl;
    std::cout << "Value at front: " << testInt.front() << ", back: " << testInt.back() << std::endl;
    
    /// clear ///
    std::cout << "Testing clear - size: " << testInt.size() << std::endl;
    testInt.clear();
    std::cout << "After clear - size: " << testInt.size() << std::endl;
    
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::function<()>
  /////////////////////////////////////////////////////////////////////////////
  {
    std::cout << "checking std::function..." << std::endl;
    std::function<void()> test = testFunction;
    
    std::cout << "calling std::function:" << std::endl;
    test();
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::vector<std::function>
  /////////////////////////////////////////////////////////////////////////////
  {
    std::vector<std::function<void()>> fvec;
    fvec.push_back(testFunction);
    fvec.push_back(testFunction);
    
    std::cout << "vector[0] == " << &fvec[0] << std::endl;
    std::cout << "vector[1] == " << &fvec[1] << std::endl;
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::function + lambda
  /////////////////////////////////////////////////////////////////////////////
  {
    std::cout << "calling std::function lambda:" << std::endl;
    std::cout << "result: " << testLambda() << std::endl;
  }
  /////////////////////////////////////////////////////////////////////////////
  //// signal
  /////////////////////////////////////////////////////////////////////////////
  {
    std::function<void()> test = testFunction;
    
    class TestSignal
    {
    public:
      signal<void()> test;
    };
    TestSignal testSignal;
    
    // lambda function
    testSignal.test.connect(
    [] {
      std::cout << "std::signal lambda test" << std::endl;
    });
    // explicit function
    testSignal.test.connect(testFunction);
    // std::function reference
    std::function<void()>& tfCopy = test;
    testSignal.test.connect(tfCopy);
    
    std::cout << "emitting signal:" << std::endl;
    testSignal.test.emit();
  }
  /////////////////////////////////////////////////////////////////////////////
  //// delegate
  /////////////////////////////////////////////////////////////////////////////
  {
    std::function<void()> test = testFunction;
    std::function<void()>& testRef = test;
    
    // explicit function
    delegate<void()> delgStatic = testFunction;
    // std::function
    delegate<void()> delgDynamic = test;
    // std::function reference
    delegate<void()> delgDynRef = testRef;
    // lambda function
    delegate<int*()> delgFunctor = testLambda;
    
    std::cout << "calling delegates:" << std::endl;
    delgStatic();
    delgDynamic();
    delgDynRef();
    std::cout << "result: " << delgFunctor() << std::endl;
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::shared_ptr, std::make_shared
  /////////////////////////////////////////////////////////////////////////////
  {
    std::cout << "creating shared pointers:" << std::endl;
    
    std::vector<std::shared_ptr<Song>> sv;
    
    sv.push_back(
      std::make_shared<Song>("Bob Dylan", "The Times They Are A Changing"));
    sv.push_back(
      std::make_shared<Song>("Aretha Franklin", "Bridge Over Troubled Water"));
    sv.push_back(
      std::make_shared<Song>("Thalia", "Entre El Mar y Una Estrella"));
    sv.push_back(
                             std::make_shared<Song>("Alf Prøysen", "Blåbærturen")); //æøå
    
    /*std::vector<std::shared_ptr<Song>> v2;
    std::remove_copy_if(v.begin(), v.end(), back_inserter(v2), 
    [] (std::shared_ptr<Song> s) 
    {
      return s->artist.compare(L"Bob Dylan") == 0;    
    });*/
    
    for (const auto& s : sv)
    {
      std::cout << s->artist << ":" << s->title << std::endl;
    }
    std::cout << "result: " << sv[0]->artist << 
      " (usage: " << sv[0].use_count() << ")" << std::endl;
    
    std::shared_ptr<Song> shared2(sv[0]);
    std::cout << "result: " << shared2->artist << 
                  " (usage: " << sv[0].use_count() << " == " << shared2.use_count() << ")" << std::endl;
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::string
  /////////////////////////////////////////////////////////////////////////////
  {
          std::string s("A string!");
          
          std::cout << s << std::endl;
          for(auto it = s.begin(); it != s.end(); ++it)
            std::cout << *it;
          std::cout << std::endl;
  }
  /////////////////////////////////////////////////////////////////////////////
  //// eastl::vector_map
  /////////////////////////////////////////////////////////////////////////////
  {
    eastl::vector_map<int, int> testMap;
  }
  /////////////////////////////////////////////////////////////////////////////
  //// std::initializer_list
  /////////////////////////////////////////////////////////////////////////////
  {
    struct IP4
    {
      union addr
      {
        uint8_t  part[4];
        uint32_t whole;
      };
    };
    
    IP4::addr ip {192,168,0,11};
    std::cout << ip.part[0] << "." << ip.part[1] << "." 
          << ip.part[2] << "." << ip.part[3] << std::endl;
    
    std::vector<int> test1 (std::initializer_list<int>({1, 2, 3, 4}));
    std::vector<int> test2 {1, 2, 3, 4};
    
    std::cout << test2[0] << ", " << test2[1] << ", " << test2[2] << ", " << test2[3] << std::endl;
  }
}