
#include <kernel/context.hpp>
#include <cstdint>
#include <memory>

#if !defined(__GNUC__)
    #define FASTCALL __fastcall
    #define GCCFASTCALL
#else
    #define FASTCALL
    #define GCCFASTCALL __attribute__((fastcall))
#endif

#ifdef ARCH_i686
extern "C" void FASTCALL __context_switch(uintptr_t stack, Context::context_func& func) GCCFASTCALL;
#else
extern "C" void __context_switch(uintptr_t stack, Context::context_func& func);
#endif
extern "C" void* get_cpu_esp();

extern "C"
void __context_switch_delegate(uintptr_t, Context::context_func& func)
{
  func();
}

void Context::jump(void* location, context_func func)
{
  // switch to stack from @location
  __context_switch((uintptr_t) location, func);
}

void Context::create(unsigned stack_size, context_func func)
{
  // create stack, and jump to end (because it grows down)
  auto stack_mem =
      std::unique_ptr<char[]>(new char[16+stack_size],
                              std::default_delete<char[]> ());
  jump(&(stack_mem.get()[stack_size]), func);
}
