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

#include <string>

struct count_ctor
{
	size_t copy_count = 0;
	size_t move_count = 0;

	explicit count_ctor() {}

	count_ctor(const count_ctor& other) :
		copy_count(other.copy_count + 1),
		move_count(other.move_count)
	{}

	count_ctor(count_ctor&& other) :
		copy_count(other.copy_count),
		move_count(other.move_count + 1)
	{}
};

struct count_ctor_wrap
{
	count_ctor cc;
	explicit count_ctor_wrap() {};

	count_ctor foo(count_ctor arg) { return arg; }
};

struct user_closure
{
	int cap_val;

	explicit user_closure(int c) :
		cap_val(c)
	{}

	user_closure(const user_closure&) = default;
	user_closure(user_closure&&) = default;

	int operator() (int& arg)
	{
		arg += cap_val;
		return arg++;
	}

	int operator() ()
	{
		return cap_val;
	}
};

class user_class
{
public:
	using del_t = delegate<int(int)>;

	explicit user_class(int val) : val_(val) {}

	int foo(int a)
	{
		return a * val_;
	}

	int simple_ret()
	{
		return val_;
	}

	del_t get_del()
	{
		return{ this, &user_class::foo };
	}
private:
	int val_;
};

const std::string f1() { return "f1"; }
const std::string f2() { return "f2"; }

CASE("A delegate can be compared to nullptr")
{
	delegate<const std::string()> d{ f1 };
	EXPECT_NOT(d == nullptr);
	EXPECT(d != nullptr);

	delegate<const std::string()> d2;
	EXPECT(d2 == nullptr);
	EXPECT_NOT(d2 != nullptr);

	auto func = [](delegate<int()> del) { return static_cast<bool>(del); };
	bool valid = func(nullptr);
	EXPECT(valid == false);
}

CASE("A delegate can be copy assigned with a different closure type")
{
	using func_t = std::function<int(void)>;
	using del_t = delegate<int(void), spec::inplace, sizeof(func_t)>;

	struct copy_closure
	{
		int cap_val;
		explicit copy_closure(int c) : cap_val(c) {}

		copy_closure(const copy_closure&) = default;
		copy_closure(copy_closure&&) = default;

		int operator() () { return cap_val; }
	};

	int cap_val = 3;
	auto lam = [cap_val]() { return cap_val; };

	del_t del_a = func_t{ lam };
	del_t del_b = copy_closure{ -1 };

	del_b = del_a;
	int ret_val = del_b();
	EXPECT(ret_val == cap_val);
}

CASE("A delegate can swap values with another delegate")
{
	delegate<int()> d2 = [] { return 53; };
	delegate<int()> d3 = [] { return 35; };

	EXPECT(d2() == 53);
	EXPECT(d3() == 35);

	std::swap(d2, d3);

	EXPECT(d2() == 35);
	EXPECT(d3() == 53);
}

CASE("A delegate can be constructed with a generic lambda")
{
	using del_t = delegate<void(int&)>;

	auto test = [&lest_env](del_t& del)
	{
		EXPECT(static_cast<bool>(del));

		int start_val = 3;
		int val = start_val;
		del(val);

		EXPECT(val == start_val + 1);
	};

	del_t del_a = [](auto& i) { ++i; };

	del_t del_b;
	del_b = del_a;

	test(del_a);
	test(del_b);
}

CASE("A delegate returns correct value (1)")
{
	std::vector<std::string> v = { "Something", "Something else" };
	delegate<size_t()> d = [v] { return v.size(); };

	size_t v_size = v.size();
	size_t d_vector_size = d();

	EXPECT(v_size == 2u);
	EXPECT(v_size == d_vector_size);
}

CASE("A delegate returns correct value (2)")
{
	std::vector<std::string> v2;
	delegate<size_t()> d2 = [v2] { return v2.size(); };

	size_t v2_size = v2.size();
	size_t d2_vector_size = d2();

	EXPECT(v2_size == 0u);
	EXPECT(v2_size == d2_vector_size);
}

CASE("A delegate returns correct value (3)")
{
	using del_t = delegate<std::string(const int i)>;
	del_t d3 = [](const int i) { return std::to_string(i); };

	int i = 12;
	std::string d3_string = d3(i);

	EXPECT(d3_string == "12");
}

CASE("A delegate can be moved")
{
	using del_t = delegate<size_t()>;

	std::vector<int> v = { 2, 9, 99, 2 };
	del_t del_a = [v] { return v.size(); };

	del_t del_b = std::move(del_a);

	size_t ret = del_b();
	EXPECT(ret == v.size());
  EXPECT_NOT(del_a);

  user_class usr(1);
  auto usr_del = usr.get_del();
  EXPECT(usr_del);
  auto usr_del2 = std::move(usr_del);
  EXPECT_NOT(usr_del);
}

CASE("A delegate can be constructed with a class member function pointer")
{
	user_class uc{ 3 };

	user_class::del_t del_a{ uc, &user_class::foo };
	int ret_a = del_a(6);
	EXPECT(ret_a == 6 * 3);

	user_class::del_t del_b = uc.get_del();
	int ret_b = del_b(6);
	EXPECT(ret_b == 6 * 3);
}

CASE("A delegate can be const")
{
	using del_t = const delegate<int()>;

  int default_val = 7;
  auto const_test = [lest_env, default_val](del_t del) mutable
	{
		int ret = del();
    EXPECT(ret == default_val);
	};

	const_test([]() { return 7; });
	const_test([default_val]() { return default_val; });
	const_test([&default_val]() { return default_val; });

	user_class uc{ default_val };
	const_test(del_t{ uc, &user_class::simple_ret });
	const_test(del_t{ &uc, &user_class::simple_ret });
}

CASE("The delegate operator() uses correct argument type forwarding")
{
	using del_t = delegate<count_ctor(count_ctor)>;

  auto test_arg_fwd = [lest_env](del_t del) mutable
	{
		auto cc_a = del(count_ctor{});
		EXPECT(cc_a.copy_count == 0);
		EXPECT(cc_a.move_count <= 3);

		count_ctor cc_b{};
		auto cc_ret_b = del(cc_b);
		EXPECT(cc_ret_b.copy_count <= 1);
		EXPECT(cc_ret_b.move_count <= 2);

		count_ctor cc_c{};
		auto cc_ret_c = del(std::move(cc_c));
		EXPECT(cc_ret_c.copy_count == 0);
		EXPECT(cc_ret_c.move_count <= 3);
	};

	int val = 3;
	test_arg_fwd(del_t{ [](count_ctor arg) { return arg; } });
  test_arg_fwd(del_t{ [val](count_ctor arg) { return arg; } });
  test_arg_fwd(del_t{ [&val](count_ctor arg) { return arg; } });

	count_ctor_wrap ccw{};
	test_arg_fwd(del_t{ ccw, &count_ctor_wrap::foo });
	test_arg_fwd(del_t{ &ccw, &count_ctor_wrap::foo });
}

CASE("A delegate can be constructed with a mutable lambda")
{
	using del_t = delegate<int()>;

	int val = 3;
	del_t del{ [val]() mutable { return ++val; } };

	int ret_a = del();
	EXPECT(ret_a == 4);

	int ret_b = del();
	EXPECT(ret_b == 5);

	EXPECT(val == 3);
}

CASE("A delegate can be constructed with any valid closure type")
{
	using func_t = std::function<int(int&)>;
	using del_t = delegate<int(int&), spec::inplace, sizeof(func_t)>;

	int default_val = 3;
	int inc_val = 4;
  auto test_closure = [lest_env, default_val, inc_val](del_t del) mutable
	{
		int val = default_val;
		int ret = del(val);

		EXPECT(ret == default_val + inc_val);
		EXPECT(val == default_val + inc_val + 1);
	};

	auto lam = [inc_val](int& arg) { arg += inc_val; return arg++; };

	test_closure(lam);
	test_closure(func_t{ lam });
	test_closure(user_closure{ inc_val });

	struct move_only_closure
	{
		explicit move_only_closure() {}

		move_only_closure(const move_only_closure&) = delete;
		move_only_closure(move_only_closure&&) = default;

		int operator() (int&) { return 3; };
	};

	// this should not compile
	//del_t impossible{ move_only_closure{} };
}

CASE("A delegate properly destroys its closure")
{
	using del_t = delegate<int(int)>;

	struct dtor_closure
	{
		bool active;
		bool& empty;

		explicit dtor_closure(bool& arg) :
			active(false),
			empty(arg)
		{}

		dtor_closure(const dtor_closure&) = default;
		dtor_closure(dtor_closure&&) = default;

		dtor_closure& operator= (const dtor_closure&) = default;
		dtor_closure& operator= (dtor_closure&&) = default;

		~dtor_closure()
		{
			if (active)
				empty = true;
		}

		int operator() (int arg)
		{
			active = true;
			return arg;
		}
	};

	bool empty = false;
	{
		del_t del{ dtor_closure{ empty } };

		EXPECT_NOT(empty);

		int arg_val = 5;
		int ret = del(arg_val);
		EXPECT(ret == arg_val);
	}

	EXPECT(empty);
}

CASE("A delegate can be constructed with any argument reference type")
{
	int default_val = 5;

	delegate<int(int)> del_a{ [](int arg) { return arg; } };
	int ret_a = del_a(default_val);
	EXPECT(ret_a == default_val);

	delegate<int(int&)> del_b{ [](int& arg) { return arg; } };
	int ret_b = del_b(default_val);
	EXPECT(ret_b == default_val);

	delegate<int(int&&)> del_c{ [](int&& arg) { return arg; } };
	int ret_c = del_c(std::move(default_val));
	EXPECT(ret_c == default_val);
}

CASE("A delegate constructor can be called multiple times with the same type")
{
	constexpr size_t start = 0;
	constexpr size_t end = 100;

	using del_t = delegate<int(void)>;

	std::vector<del_t> vec;
	for (int i = start; i <= end; ++i)
		vec.emplace_back([i]() { return i; });

	int first = vec.front()();
	EXPECT(first == start);

	int last = vec.back()();
	EXPECT(last == end);
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
			d = [](const char* s) { printf("String: %s", s); };
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
		delegate<void()> dnull{ nullptr };
		EXPECT_NOT(dnull);

		std::function<void()> nofunc{};
		EXPECT_NOT(nofunc);

		try {
			nofunc();
		}
		catch (std::bad_function_call&) {
			// Calling a zero-initialized std::function throws - so should delegates;
			// this should be std::bad_function_call but we need modules to only import
			// the exception from header <functional>
			EXPECT_THROWS_AS(dnull(), std::bad_function_call);
		}
		catch (...)
		{
			EXPECT((false && "something terrible happend"));
		}
	}
}
