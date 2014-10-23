#include <os>
#include <class_dev.hpp>

#include <assert.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <tuple.hpp>
#include <signal>
#include <delegate>
#include <sort>
#include <vector_map>

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

class TestSignal
{
public:
	signal<void()> test;
};

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

void Service::start()
{
	std::cout << "*** Service is up - with OS Included! ***" << std::endl;
	
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
		
		std::cout << "success!" << std::endl;
	}
	/////////////////////////////////////////////////////////////////////////////
	//// std::map
	/////////////////////////////////////////////////////////////////////////////
	{
		std::map<int, int> testMap;
	}
	/////////////////////////////////////////////////////////////////////////////
	//// std::function
	/////////////////////////////////////////////////////////////////////////////
	{
		std::function<void()> test = testFunction;
		
		std::cout << "calling std::function:" << std::endl;
		test();
	}
	/////////////////////////////////////////////////////////////////////////////
	//// vector<std::function>
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
		std::vector_map<int, int> testMap;
	}
	/////////////////////////////////////////////////////////////////////////////
	//// std::initializer_list
	/////////////////////////////////////////////////////////////////////////////
	{
	  struct IP4 {
		union addr{
		  uint8_t part[4];
		  uint32_t whole;
		};
	  };
	  
	  IP4::addr ip {192,168,0,11};
	  std::cout << ip.part[0] << "." << ip.part[1] << "." 
				<< ip.part[2] << "." << ip.part[3] << std::endl;
	}
	std::cout << "Service out!" << std::endl;
}
