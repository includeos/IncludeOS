// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud
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

#ifndef OSABI_SIGNAL_HPP
#define OSABI_SIGNAL_HPP

#include <functional>
#include <vector>

template<typename F>
class signal
{
public:
	//! \brief Connect a function to this signal
	template <class Fn>
	void connect(Fn fn)
	{
		funcs.emplace_back(fn);
	}
	
	//! \brief Emit this signal by executing all the connected functions
	template<typename... Args>
	void emit(Args... args)
	{
		for(auto fn : funcs)
			fn(args...);
	}
	
private:
	std::vector<std::function<F>> funcs;
};

#endif
