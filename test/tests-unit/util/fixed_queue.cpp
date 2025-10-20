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

#include <common.cxx>
#include <util/fixed_queue.hpp>

#include <string>

template<typename T, size_t N> void test_basic(
	lest::env& lest_env,
	T val_a,
	T val_b
)
{
SETUP("fixed_queue test_basic")
{
	util::fixed_queue<T, N> fq;

	SECTION("fold on uninitialized")
	{
		size_t count_folds{};
		fq.fold([&count_folds](const auto& val) -> void { ++count_folds; });
		EXPECT(count_folds == 0);
	}

	SECTION("push_back(), compare to front() and back()")
	{
		fq.push_back(val_b);
		for (size_t i = 0; i < N - 1; ++i)
			fq.push_back(val_a);

		EXPECT(fq.back() == val_b);

		fq.push_back(val_b);
		EXPECT(fq.front() == val_b);

		EXPECT((N == 1 || (fq.back() != val_b)));
	}

	SECTION("fold assign")
	{
		fq.fold([val_a](auto& val) { val = val_a; });

		fq.fold([&lest_env, val_a](auto val)
		{
			EXPECT(val == val_a);
		}
		);
	}
}
}

template<typename T, size_t N> void test_range(
	lest::env& lest_env,
	T val_a,
	T val_b
)
{
SETUP("fixed_queue test_range")
{
	test_basic<T, N>(lest_env, val_a, val_b);

	util::fixed_queue<T, N> fq;

	if (N == 1)
	{
		SECTION("merge range of size 1")
		{
			fq.push_back(val_a);
			auto res = merge_ring_range(fq);
			EXPECT(res == val_a);
			return;
		}
	}
	else if (N == 2)
	{
		SECTION("merge range of size 2")
		{
			fq.push_back(val_b);
			fq.push_back(val_a);
			auto res = merge_ring_range(fq);
			EXPECT(res == val_a + val_b);
			return;
		}
	}

	SECTION("merge range of size > 2")
	{
		fq.push_back(val_b);
		for (size_t i = 0; i < N - 2; ++i)
			fq.push_back(val_a);
		fq.push_back(val_b);

		auto manual = val_b;
		for (size_t i = 0; i < N - 2; ++i)
			manual += val_a;
		manual += val_b;

		auto res = merge_ring_range(fq);
		EXPECT(res == manual);
	}
}
}

CASE("Test fixed_queue for pod types")
{
	test_basic<int, 1>(lest_env, 7, 5);
	test_basic<float, 3>(
		lest_env,
		std::numeric_limits<float>::min(),
		std::numeric_limits<float>::max()
	);

}
CASE("Test fixed_queue with range types")
{
	auto& le = lest_env;
	test_range<std::string, 1>(le, "short", "a bit longer, and even more");
	test_range<std::string, 2>(le, "short", "a bit longer, and even more");
	test_range<std::string, 3>(le, "short", "a bit longer, and even more");
	test_range<std::string, 5>(le, "short", "a bit longer, and even more");
	test_range<std::string, 100>(le, "short", "a bit longer, and even more");
}
