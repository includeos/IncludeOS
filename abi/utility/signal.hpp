#ifndef OSABI_SIGNAL_HPP
#define OSABI_SIGNAL_HPP

#include <functional>
#include <vector>

template<typename F>
class signal
{
public:
	//! \brief Connect a function to this signal
	void connect(function<F> fn)
	{
		funcs.push_back(fn);
	}
	
	//! \brief Emit this signal by executing all the connected functions
	template<typename... Args>
	void emit(Args... args)
	{
		for(auto fn : funcs)
			fn(args...);
	}
	
private:
	vector<function<F>> funcs;
};

#endif
