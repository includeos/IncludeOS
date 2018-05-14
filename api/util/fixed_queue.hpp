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

#ifndef FIXEDQUEUE_H_INCLUDED
#define FIXEDQUEUE_H_INCLUDED

#include <array>
#include <type_traits>

namespace util
{

template<typename T, size_t N> class fixed_queue
{
public:
	using buffer_t = std::array<T, N>;

	explicit fixed_queue() : index_(0)
	{
		static_assert(N > 0, "fixed_queue size should be larger than zero!");
	};

	~fixed_queue() {};

	fixed_queue(const fixed_queue&) = default;
	fixed_queue(fixed_queue&&) = default;

	fixed_queue& operator= (const fixed_queue&) = default;
	fixed_queue& operator= (fixed_queue&&) = default;

	void push_back(const T& val)
		noexcept(std::is_trivially_copy_assignable<T>::value)
	{
		++index_;
		buff_[index_ % N] = val;
	}

	void push_back(T&& val)
		noexcept(std::is_trivially_move_assignable<T>::value)
	{
		++index_;
		buff_[index_ % N] = std::move(val);
	}

	T& front() noexcept { return buff_[index_ % N]; }
	T& back() noexcept { return buff_[(index_ + 1) % N]; }

	template<typename F> void fold(F&& func)
	{
		for (size_t i = 0, max = index_ < N ? index_ : N ; i < max; ++i)
			std::forward<F>(func)(buff_[(index_ - i) % N]);
	}
private:
	size_t index_;
	buffer_t buff_;
};

template<typename T, size_t N> T merge_ring_range(
	fixed_queue<T, N>& client_agents
)
{
	T ret;

	size_t capacity{};
	client_agents.fold(
		[&capacity](const auto& s) noexcept -> void
		{ capacity += s.capacity(); }
	);
	ret.reserve(capacity);

	client_agents.fold(
		[&ret](const auto& val) -> void { ret += val; }
	);

	return ret;
}

}

#endif // FIXEDQUEUE_H_INCLUDED
