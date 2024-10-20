#ifndef KERNEL_DIAG_HOOKS_HPP
#define KERNEL_DIAG_HOOKS_HPP
#define RUN_DIAG_HOOKS true

constexpr bool run_diag_hooks = RUN_DIAG_HOOKS;

namespace kernel::diag {
  void post_bss() noexcept;
  void post_machine_init() noexcept;
  void post_init_libc() noexcept;
  void post_service() noexcept;

  void default_post_init_libc() noexcept;

  template <auto Func>
  void hook() {
    if constexpr (run_diag_hooks) {
      Func();
    }
  }
}

#endif
