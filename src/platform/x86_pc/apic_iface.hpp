
#pragma once
#ifndef X86_APIC_IFACE_HPP
#define X86_APIC_IFACE_HPP

#include <array>
#include <cstdint>

namespace x86 {

  class IApic {
  public:
    virtual const char* name() const noexcept = 0;
    virtual uint32_t get_id()  noexcept = 0;
    virtual uint32_t version() noexcept = 0;

    virtual void
    interrupt_control(uint32_t bits, uint8_t spurious) noexcept = 0;

    virtual void enable() noexcept = 0;
    virtual void smp_enable() noexcept = 0;

    virtual void eoi() noexcept = 0;
    virtual int  get_isr() noexcept = 0;
    virtual int  get_irr() noexcept = 0;

    virtual void ap_init (int id) noexcept = 0;
    virtual void ap_start(int id, uint32_t vec) noexcept = 0;

    virtual void send_ipi(int id, uint8_t vector) noexcept = 0;
    virtual void send_bsp_intr() noexcept = 0;
    virtual void bcast_ipi(uint8_t vector) noexcept = 0;

    virtual void     timer_init(const uint8_t) = 0;
    virtual void     timer_begin(uint32_t) noexcept = 0;
    virtual uint32_t timer_diff() noexcept = 0;
    virtual void     timer_interrupt(bool) noexcept = 0;
  };
}

#endif
