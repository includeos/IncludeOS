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

#ifndef DELEGATE_HPP_INCLUDED
#define DELEGATE_HPP_INCLUDED

#include <type_traits>
#include <functional>
#include <memory>

// ----- SYNOPSIS -----

namespace spec
{
	template<size_t, size_t, typename, typename...> class pure;
	template<size_t, size_t, typename, typename...> class inplace_triv;
	template<size_t, size_t, typename, typename...> class inplace;
}

namespace detail
{
	constexpr size_t default_capacity = sizeof(size_t) * 4;

	template<typename T> using default_alignment = std::alignment_of<
		std::function<T>
	>;
}


template<
	typename T,
	template<size_t, size_t, typename, typename...> class Spec = spec::inplace,
	size_t size = detail::default_capacity,
	size_t align = detail::default_alignment<T>::value
>
class delegate; // unspecified

template<
	typename R, typename... Args,
	template<size_t, size_t, typename, typename...> class Spec,
	size_t size,
	size_t align
>
class delegate<R(Args...), Spec, size, align>;

class empty_delegate_error : public std::bad_function_call
{
public:
    const char* what() const throw() {
      return "Empty delegate called";
    }
};

// ----- IMPLEMENTATION -----

namespace detail
{
template<typename R, typename... Args> static R empty_pure(Args...)
{
	throw empty_delegate_error();
}

template<
	typename R,
	typename S,
	typename... Args
> static R empty_inplace(S&, Args&&... args)
{
	return empty_pure<R, Args...>(std::forward<Args>(args)...);
}

template<
	typename T,
	typename R,
	typename... Args
> using closure_decay = std::conditional<
	std::is_convertible<T, R(*)(Args...)>::value,
	R(*)(Args...),
	typename std::decay<T>::type
>;

template<typename T = void, typename...> struct pack_first
{
	using type = std::remove_cv_t<T>;
};

template<typename... Ts>
using pack_first_t = typename pack_first<Ts...>::type;

}

namespace spec
{

// --- pure ---
template<size_t, size_t, typename R, typename... Args> class pure
{
public:
	using invoke_ptr_t = R(*)(Args...);

	explicit pure() noexcept :
		invoke_ptr_{ detail::empty_pure<R, Args...> }
	{}

	template<typename T> explicit pure(T&& func_ptr) noexcept :
		invoke_ptr_{ func_ptr }
	{
		static_assert(std::is_convertible<T, invoke_ptr_t>::value,
			"object not convertible to pure function pointer!");
	}

	pure(const pure&) noexcept = default;
	pure(pure&&) noexcept = default;

	pure& operator= (const pure&) noexcept = default;
	pure& operator= (pure&&) noexcept = default;

	~pure() = default;

	R operator() (Args&&... args) const
	{
		return invoke_ptr_(std::forward<Args>(args)...);
	}

	bool empty() const noexcept
	{
		return invoke_ptr_ == static_cast<invoke_ptr_t>(
			detail::empty_pure<R, Args...>);
	}

	template<typename T> T* target() const noexcept
	{
		return static_cast<T*>(invoke_ptr_);
	}

private:
	invoke_ptr_t invoke_ptr_;
};

// --- inplace_triv ---
template<
	size_t size,
	size_t align,
	typename R,
	typename... Args
> class inplace_triv
{
public:
	using storage_t = std::aligned_storage_t<size, align>;
	using invoke_ptr_t = R(*)(storage_t&, Args&&...);

	explicit inplace_triv() noexcept :
		invoke_ptr_{ detail::empty_inplace<R, storage_t, Args...> }
	{
		new(&storage_)std::nullptr_t{ nullptr };
	}

	template<
		typename T,
		typename C = typename detail::closure_decay<T, R, Args...>::type
	> explicit inplace_triv(T&& closure) :
		invoke_ptr_{ static_cast<invoke_ptr_t>(
			[](storage_t& storage, Args&&... args) -> R
			{ return reinterpret_cast<C&>(storage)(std::forward<Args>(args)...); }
		)}
	{
		static_assert(sizeof(C) <= size,
			"inplace_triv delegate closure too large!");

		static_assert(std::alignment_of<C>::value <= align,
			"inplace_triv delegate closure alignment too large");

		static_assert(std::is_trivially_copyable<C>::value,
			"inplace_triv delegate closure not trivially copyable!");

		static_assert(std::is_trivially_destructible<C>::value,
			"inplace_triv delegate closure not trivially destructible!");

		new(&storage_)C{ std::forward<T>(closure) };
	}

	inplace_triv(const inplace_triv&) noexcept = default;
	inplace_triv(inplace_triv&&) noexcept = default;

	inplace_triv& operator= (const inplace_triv&) noexcept = default;
	inplace_triv& operator= (inplace_triv&&) noexcept = default;

	~inplace_triv() = default;

	R operator() (Args&&... args) const
	{
		return invoke_ptr_(storage_, std::forward<Args>(args)...);
	}

	bool empty() const noexcept
	{
		return reinterpret_cast<std::nullptr_t&>(storage_) == nullptr;
	}

	template<typename T> T* target() const noexcept
	{
		return reinterpret_cast<T*>(&storage_);
	}

private:
	invoke_ptr_t invoke_ptr_;
	mutable storage_t storage_;
};

// --- inplace ---
template<
	size_t size,
	size_t align,
	typename R,
	typename... Args
> class inplace
{
public:
	using storage_t = std::aligned_storage_t<size, align>;

	using invoke_ptr_t = R(*)(storage_t&, Args&&...);
	using copy_ptr_t = void(*)(storage_t&, storage_t&);
	using destructor_ptr_t = void(*)(storage_t&);

	explicit inplace() noexcept :
		invoke_ptr_{ detail::empty_inplace<R, storage_t, Args...> },
		copy_ptr_{ copy_op<std::nullptr_t, storage_t>() },
		destructor_ptr_{ nullptr }
	{}

	template<
		typename T,
		typename C = typename detail::closure_decay<T, R, Args...>::type
	> explicit inplace(T&& closure) noexcept :
		invoke_ptr_{ static_cast<invoke_ptr_t>(
			[](storage_t& storage, Args&&... args) -> R
			{ return reinterpret_cast<C&>(storage)(std::forward<Args>(args)...); }
		) },
		copy_ptr_{ copy_op<C, storage_t>() },
		destructor_ptr_{ static_cast<destructor_ptr_t>(
			[](storage_t& storage) noexcept -> void
			{ reinterpret_cast<C&>(storage).~C(); }
		) }
	{
		static_assert(sizeof(C) <= size,
			"inplace delegate closure too large");

		static_assert(std::alignment_of<C>::value <= align,
			"inplace delegate closure alignment too large");

		new(&storage_)C{ std::forward<T>(closure) };
	}

	inplace(const inplace& other) :
		invoke_ptr_{ other.invoke_ptr_ },
		copy_ptr_{ other.copy_ptr_ },
		destructor_ptr_{ other.destructor_ptr_ }
	{
		copy_ptr_(storage_, other.storage_);
	}

	inplace(inplace&& other)  :
		storage_ { std::move(other.storage_) },
		invoke_ptr_{ other.invoke_ptr_ },
		copy_ptr_{ other.copy_ptr_ },
		destructor_ptr_{ other.destructor_ptr_ }
	{
		other.destructor_ptr_ = nullptr;
	}

	inplace& operator= (const inplace& other)
	{
		if (this != std::addressof(other))
		{
			invoke_ptr_ = other.invoke_ptr_;
			copy_ptr_ = other.copy_ptr_;

			if (destructor_ptr_)
				destructor_ptr_(storage_);

			copy_ptr_(storage_, other.storage_);
			destructor_ptr_ = other.destructor_ptr_;
		}
		return *this;
	}

	inplace& operator= (inplace&& other)
	{
		if (this != std::addressof(other))
		{
			if (destructor_ptr_)
				destructor_ptr_(storage_);

			storage_ = std::move(other.storage_);

			invoke_ptr_ = other.invoke_ptr_;
			copy_ptr_ = other.copy_ptr_;
			destructor_ptr_ = other.destructor_ptr_;

			other.destructor_ptr_ = nullptr;
		}
		return *this;
	}

	~inplace()
	{
		if (destructor_ptr_)
			destructor_ptr_(storage_);
	}

	R operator() (Args&&... args) const
	{
		return invoke_ptr_(storage_, std::forward<Args>(args)...);
	}

	bool empty() const noexcept
	{
		return destructor_ptr_ == nullptr;
	}

	template<typename T> T* target() const noexcept
	{
		return reinterpret_cast<T*>(&storage_);
	}

private:
	mutable storage_t storage_ {};

	invoke_ptr_t invoke_ptr_;
	copy_ptr_t copy_ptr_;
	destructor_ptr_t destructor_ptr_;

	template<
		typename T,
		typename S,
		typename std::enable_if_t<
		std::is_copy_constructible<T>::value, int
		> = 0
	> copy_ptr_t copy_op()
	{
		return [](S& dst, S& src) noexcept -> void
		{
			new(&dst)T{ reinterpret_cast<T&>(src) };
		};
	}

	template<
		typename T,
		typename S,
		typename std::enable_if_t<
		!std::is_copy_constructible<T>::value, int
		> = 0
	> copy_ptr_t copy_op()
	{
		static_assert(std::is_copy_constructible<T>::value,
			"constructing delegate with move only type is invalid!");
	}
};
} // namespace spec

template<
	typename R, typename... Args,
	template<size_t, size_t, typename, typename...> class Spec,
	size_t size,
	size_t align
>
class delegate<R(Args...), Spec, size, align>
{
public:
	using result_type = R;

	using storage_t = Spec<size, align, R, Args...>;

	explicit delegate() noexcept :
		storage_{}
	{}

	template<
		typename T,
		typename = std::enable_if_t<
        !std::is_same<std::decay_t<T>, delegate>::value>
		/*&& std::is_same<
			decltype(std::declval<T&>()(std::declval<Args>()...)),
			R
		>::value>*/
	>
	delegate(T&& val) :
		storage_{ std::forward<T>(val) }
	{}

	// delegating constructors
	delegate(std::nullptr_t) noexcept :
		delegate()
	{}

	// construct with member function pointer

	// object pointer capture
	template<typename C>
	delegate(C* const object_ptr, R(C::* const method_ptr)(Args...))
		noexcept : delegate(
			[object_ptr, method_ptr](Args&&... args) -> R
	{
		return (object_ptr->*method_ptr)(std::forward<Args>(args)...);
	})
	{}

	template<typename C>
	delegate(C* const object_ptr, R(C::* const method_ptr)(Args...) const)
		noexcept : delegate(
			[object_ptr, method_ptr](Args&&... args) -> R
	{
		return (object_ptr->*method_ptr)(std::forward<Args>(args)...);
	})
	{}

	// object reference capture
	template<typename C>
	delegate(C& object_ref, R(C::* const method_ptr)(Args...))
		noexcept : delegate(
			[&object_ref, method_ptr](Args&&... args) -> R
	{
		return (object_ref.*method_ptr)(std::forward<Args>(args)...);
	})
	{}

	template<typename C>
	delegate(C& object_ref, R(C::* const method_ptr)(Args...) const)
		noexcept : delegate(
			[&object_ref, method_ptr](Args&&... args) -> R
	{
		return (object_ref.*method_ptr)(std::forward<Args>(args)...);
	})
	{}

	// object pointer as parameter
	template<
		typename C,
		typename... MemArgs,
		typename std::enable_if_t<
		std::is_same<detail::pack_first_t<Args...>, C*>::value, int> = 0
	> delegate(R(C::* const method_ptr)(MemArgs...))
		noexcept : delegate(
			[method_ptr](C* object_ptr, MemArgs... args) -> R
	{
		return (object_ptr->*method_ptr)(std::forward<MemArgs>(args)...);
	})
	{}

	template<
		typename C,
		typename... MemArgs,
		typename std::enable_if_t<
		std::is_same<detail::pack_first_t<Args...>, C*>::value, int> = 0
	> delegate(R(C::* const method_ptr)(MemArgs...) const)
		noexcept : delegate(
			[method_ptr](C* object_ptr, MemArgs... args) -> R
	{
		return (object_ptr->*method_ptr)(std::forward<MemArgs>(args)...);
	})
	{}

	// object reference as parameter
	template<
		typename C,
		typename... MemArgs,
		typename std::enable_if_t<
		std::is_same<detail::pack_first_t<Args...>, C&>::value, int> = 0
	> delegate(R(C::* const method_ptr)(MemArgs...))
		noexcept : delegate(
			[method_ptr](C& object, MemArgs... args) -> R
	{
		return (object.*method_ptr)(std::forward<MemArgs>(args)...);
	})
	{}

	template<
		typename C,
		typename... MemArgs,
		typename std::enable_if_t<
		std::is_same<detail::pack_first_t<Args...>, C&>::value, int> = 0
	> delegate(R(C::* const method_ptr)(MemArgs...) const)
		noexcept : delegate(
			[method_ptr](C& object, MemArgs... args) -> R
	{
		return (object.*method_ptr)(std::forward<MemArgs>(args)...);
	})
	{}

	// object copy as parameter
	template<
		typename C,
		typename... MemArgs,
		typename std::enable_if_t<
		std::is_same<detail::pack_first_t<Args...>, C>::value, int> = 0
	> delegate(R(C::* const method_ptr)(MemArgs...))
		noexcept : delegate(
			[method_ptr](C object, MemArgs... args) -> R
	{
		return (object.*method_ptr)(std::forward<MemArgs>(args)...);
	})
	{}

	template<
		typename C,
		typename... MemArgs,
		typename std::enable_if_t<
		std::is_same<detail::pack_first_t<Args...>, C>::value, int> = 0
	> delegate(R(C::* const method_ptr)(MemArgs...) const)
		noexcept : delegate(
			[method_ptr](C object, MemArgs... args) -> R
	{
		return (object.*method_ptr)(std::forward<MemArgs>(args)...);
	})
	{}

	delegate(const delegate&) = default;
	delegate(delegate&&) = default;

	delegate& operator= (const delegate&) = default;
	delegate& operator= (delegate&&) = default;

	~delegate() = default;

	R operator() (Args... args) const
	{
		return storage_(std::forward<Args>(args)...);
	}

	bool operator== (std::nullptr_t) const noexcept
	{
		return storage_.empty();
	}

	bool operator!= (std::nullptr_t) const noexcept
	{
		return !storage_.empty();
	}

	explicit operator bool() const noexcept
	{
		return !storage_.empty();
	}

	void swap(delegate& other)
	{
		storage_t tmp = storage_;
		storage_ = other.storage_;
		other.storage_ = tmp;
	}

	void reset()
	{
		storage_t empty;
		storage_ = empty;
	}

	template<typename T> T* target() const noexcept
	{
		return storage_.template target<T>();
	}

	template<
		typename T,
		typename D = std::shared_ptr<T>,
		typename std::enable_if_t<size >= sizeof(D), int> = 0
	> static delegate make_packed(T&& closure)
	{
		D ptr = std::make_shared<T>(std::forward<T>(closure));
		return [ptr](Args&&... args) -> R
		{
			return (*ptr)(std::forward<Args>(args)...);
		};
	}

	template<
		typename T,
		typename D = std::shared_ptr<T>,
		typename std::enable_if_t<!(size >= sizeof(D)), int> = 0
	> static delegate make_packed(T&&)
	{
		static_assert(size >= sizeof(D), "Cannot pack into delegate");
	}

private:
	storage_t storage_;
};

#endif // DELEGATE_HPP_INCLUDED
