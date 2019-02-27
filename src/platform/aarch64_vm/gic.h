//void gic_init();
//TODO check if the GIC is platform independent or not..
//get gic base from FDT
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif
//TODO covert to a generic c++ class ?
//level or edge trigger
#define GICD_ICFGR_LEVEL    0x0
#define GICD_ICFGR_EDGE     0x2

void gic_init_fdt(const char * fdt,uint32_t gic_offset);
//dont think this is right for arm32..
void gic_init(uint64_t gic_gicd_base,uint64_t gic_gicc_base);
//int gic_find_pending_irq(exception int *irq); ?
void gicd_irq_disable(int irq); //disable irq
void gicd_irq_enable(int irq); //enable irq
void gicd_irq_clear(int irq); //clear pending

void gicd_set_target(uint32_t irq,uint8_t processor);
void gicd_set_priority(uint32_t irq,uint8_t priority);
void gicd_set_config(uint32_t irq,uint8_t config);

int gicd_decode_irq();

#if defined(__cplusplus)
}
#endif
