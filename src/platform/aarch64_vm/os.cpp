#include <kernel.hpp>
#include <timer.h>

extern "C" {
  #include <libfdt.h>
}

#include "gic.h"
#include <cpu.h>

void kernel::start(uint64_t fdt_addr) // boot_magic, uint32_t boot_addr)
{
  //printf("printf os start\r\n");
  //belongs in platform ?
  const char *fdt=(const char *)fdt_addr;
  printf("fdt addr %08x\r\n",fdt_addr);
  //checks both magic and version
  if ( fdt_check_header(fdt) != 0 )
  {
    printf("FDT Header check failed\r\n");
    return;
  }
  const int intc = fdt_path_offset(fdt, "/intc");

  if (intc < 0) //interrupt controller not found in fdt.. should never happen..
  {
    printf("interrupt controller not found in dtb\r\n");
    return;
  }
  if (fdt_node_check_compatible(fdt,intc,"arm,cortex-a15-gic") == 0)
  {
    printf("init gic\r\n");
    gic_init_fdt(fdt,intc);
  }
  //cpu_fiq_enable();
  cpu_irq_enable();
  //cpu_serror_enable();
  printf("curr %lu compare %lu ctl %08x\r\n",timer_get_virtual_countval(),timer_get_virtual_compare(),timer_get_virtual_control());

  timer_virtual_stop();
  printf("curr %lu compare %lu ctl %08x\r\n",timer_get_virtual_countval(),timer_get_virtual_compare(),timer_get_virtual_control());

#define TIMER_IRQ 27
  //trigger only once
  gicd_set_config(TIMER_IRQ,GICD_ICFGR_EDGE);
  //highest possible priority
  gicd_set_priority(TIMER_IRQ,0);
  gicd_set_target(TIMER_IRQ,0x01); //target is processor 1
  gicd_irq_clear(TIMER_IRQ);
  gicd_irq_enable(TIMER_IRQ);



  printf("OS start\r\n");
  printf("Timer frequency %.3f MHz\r\n",timer_get_frequency()/1000000.0);

  timer_set_virtual_compare(timer_get_virtual_countval()+timer_get_frequency()/2);

  timer_virtual_start();
  //Wait for interrupt
  cpu_wfi();
/*
  for (int i =0 ; i < 9000;i++)
  {

//    printf("curr %lu compare %lu ctl %08x\r\n",timer_get_virtual_countval(),timer_get_virtual_compare(),timer_get_control());
//    timer_stop();
}*/
    //gicd_irq_disable(TIMER_IRQ);
    //gicd_irq_clear(TIMER_IRQ);
//    timer_start();
//    printf("curr %lu compare %lu ctl %08x\r\n",timer_get_virtual_countval(),timer_get_virtual_compare(),timer_get_control());

  //0.5 sec
  //uint32_t curr=timer_get_downcount();
  //200ms later
  //timer_set_downcount(curr+timer_get_frequency()/5);

  //timer_set_downcount(10000);





  //intc is of this type.. so lets initialize it
  //there is a stringlist compare as well.
  //and a find compatible..
  //TODO ASK team..

  //printf("Timer count %08x\r\n",timer_get_downcount());
  //enable exceptions

  //  cpu_debug_enable();
  /*assert(boot_magic == MULTIBOOT_BOOTLOADER_MAGIC);
  OS::multiboot(boot_addr);
  assert(OS::memory_end_ != 0);
*/
  //platform_init();
}
