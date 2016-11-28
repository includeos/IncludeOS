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

template<typename T, size_t N> class ring_buffer
{
public:
	using buffer_t = std::array<T, N>;
	using iterator = typename buffer_t::iterator;
	using const_iterator = typename buffer_t::const_iterator;

	explicit ring_buffer() : index_(0)
	{
		static_assert(N > 0, "ring_buffer size should be larger than zero!");
	};

	~ring_buffer() {};

	ring_buffer(const ring_buffer&) = default;
	ring_buffer(ring_buffer&&) = default;

	ring_buffer& operator= (const ring_buffer&) = default;
	ring_buffer& operator= (ring_buffer&&) = default;

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

	iterator begin() noexcept { return buff_.begin(); }
	iterator end() noexcept { return buff_.end(); }

	const_iterator cbegin() const noexcept { return buff_.cbegin(); }
	const_iterator cend() const noexcept { return buff_.cend(); }

	template<typename F> void fold(F&& func)
		noexcept(noexcept(func(front())))
	{
		for (size_t i = 0, max = index_ < N ? index_ : N ; i < max; ++i)
			std::forward<F>(func)(buff_[(index_ - i) % N]);
	}
private:
	size_t index_;
	buffer_t buff_;
};
template<typename T, size_t N> T merge_ring_range(
	ring_buffer<T, N>& client_agents
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

#endif // DEMO_UTILITY_HPP
