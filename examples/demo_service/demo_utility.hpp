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

#ifndef DEMO_UTILITY_HPP
#define DEMO_UTILITY_HPP


#include <array>
#include <type_traits>

namespace util
{

template<typename T, size_t N> class ring_buff
{
public:
	explicit ring_buff() : index_(0)
	{
		static_assert(N > 0, "N should be larger than zero!");
	};

	~ring_buff() {};

	ring_buff(const ring_buff&) = default;
	ring_buff(ring_buff&&) = default;

	ring_buff& operator= (const ring_buff&) = default;
	ring_buff& operator= (ring_buff&&) = default;

	void push_back(const T& val)
		noexcept(std::is_trivially_copyable<T>::value)
	{
		buff_[index_ % N] = val;
		++index_;
	}

	void push_back(T&& val)
		noexcept(std::is_trivially_move_assignable<T>::value)
	{
		buff_[index_ % N] = std::move(val);
		++index_;
	}

	T& front() noexcept { return buff_[(index_ - 1) % N]; }
	T& back() noexcept { return buff_[(index_ - N) % N]; }

	template<typename F> void fold(F&& func)
	{
		for (size_t i = N; i > 0; --i)
			std::forward<F>(func)(buff_[(index_ + i - 1) % N]);
	}
private:
	size_t index_;
	std::array<T, N> buff_;
};

template<typename T, size_t N> T fold_ring_range(
	ring_buff<T, N>& client_agents
)
{
	T ret;

	size_t capacity = 0;
	client_agents.fold(
		[&capacity](const auto& s) -> void { capacity += s.capacity(); }
	);
	ret.reserve(capacity);

	client_agents.fold(
		[&ret](const auto& val) -> void { ret += val; }
	);

	return ret;
}

}

#endif // DEMO_UTILITY_HPP
