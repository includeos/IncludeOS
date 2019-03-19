#include <kprint>

#include "gic.h"

extern "C" {
  #include <libfdt.h>
}

#include "gic_regs.h"
//#define PRATTLE(fmt, ...) kprintf(fmt, ##__VA_ARGS__)

#define GIC_TRACE 1

#if defined(GIC_TRACE)
  #define GIC_PRINT(type, fmt, ...) \
    kprintf("GIC %s : ",type);\
    kprintf(fmt, ##__VA_ARGS__);
#else
  #define GIC_PRINT(type, fmt, ...) do {} while(0);
#endif

#define GIC_INFO(fmt, ...) GIC_PRINT("INFO",fmt, ##__VA_ARGS__)
#define GIC_DEBUG(fmt, ...) GIC_PRINT("DEBUG",fmt, ##__VA_ARGS__)
#define GIC_ERROR(fmt, ...) GIC_PRINT("ERROR",fmt, ##__VA_ARGS__)

//extending lifdt.. maybe do this in a more centralized location
//that pain!!
int fdt_interrupt_cells(const void *fdt,int nodeoffset)
{
  const struct fdt_property *prop;
	int val;
	int len;
  const char *name="#interrupt-cells";
	prop = fdt_get_property(fdt, nodeoffset,name, &len);
	if (!prop)
		return len;
    //hmm

	if (len != (sizeof(int)))
		return -FDT_ERR_BADNCELLS;

  val=fdt32_ld((const fdt32_t *)((char *)prop->data));

  if (val == -FDT_ERR_NOTFOUND)
    return 1;

	if ((val <= 0) || (val > FDT_MAX_NCELLS))
		return -FDT_ERR_BADNCELLS;
	return val;
}


uint64_t fdt_load_addr(const struct fdt_property *prop,int *offset,int addr_cells)
{
  uint64_t addr=0;
  for (int j = 0; j < addr_cells; ++j) {
    addr |= (uint64_t)fdt32_ld((const fdt32_t *)((char *)prop->data + *offset)) <<
      ((addr_cells - j - 1) * 32);
      *offset+=sizeof(uint32_t);
  }
  return addr;
}


uint64_t fdt_load_size(const struct fdt_property *prop,int *offset,int size_cells)
{
  uint64_t size=0;
  for (int j = 0; j < size_cells; ++j) {
    size |= (uint64_t)fdt32_ld((const fdt32_t *)((char *)prop->data + *offset)) <<
      ((size_cells - j - 1) * 32);
      *offset+=sizeof(uint32_t);
  }
  return size;
}



//qemu gives us a cortex-a15-gic (why ?)
void gic_init_fdt(const char * fdt,uint32_t fdt_offset)
{
  const struct fdt_property *prop;
  int addr_cells = 0, size_cells = 0, interrupt_cells = 0;
  size_cells = fdt_size_cells(fdt,fdt_offset);
  addr_cells = fdt_address_cells(fdt, fdt_offset);//fdt32_ld((const fdt32_t *)prop->data);
  //how to get ?
  interrupt_cells=fdt_interrupt_cells(fdt,fdt_offset);

  GIC_INFO("size_cells %d\n",size_cells);
  GIC_INFO("addr_cells %d\n",addr_cells);
  int proplen;
  prop = fdt_get_property(fdt, fdt_offset, "reg", &proplen);
  printf("Proplen %d\r\n",proplen);
  int cellslen = (int)sizeof(uint32_t) * (addr_cells + size_cells);
  if (proplen/cellslen < 2)
  {
    GIC_ERROR("fdt expected two addresses")
    return;
  }
  int offset=0;
  uint64_t len;
  uint64_t gicd;
  uint64_t gicc;

  gicd=fdt_load_addr(prop,&offset,addr_cells);
  GIC_DEBUG("gicd %016lx\r\n",gicd);
  len=fdt_load_size(prop,&offset,size_cells);
  GIC_DEBUG("len %016lx\r\n",len);
  gicc=fdt_load_addr(prop,&offset,addr_cells);
  GIC_DEBUG("gicc %016lx\r\n",gicc);
  //not really needed'

  len=fdt_load_size(prop,&offset,size_cells);
  GIC_DEBUG("len %016lx\r\n",len);

  //gic_init(uint64_t gic_gicd_base,uint64_t gic_gicc_base);
  GIC_INFO("interrupt_cells %d\n",interrupt_cells);

  gic_init(gicd,gicc);
}



//cant have multiple now this isnt good..

struct gic_params{
  struct gic_v3_cpu_interface_gicc *gicc;
  struct gic_v3_distributor_map *gicd;
  void set_gicc(uint64_t addr) {
    gicc=static_cast<struct gic_v3_cpu_interface_gicc *>((void*)addr);
  }
  void set_gicd(uint64_t addr) {
    gicd=static_cast<struct gic_v3_distributor_map *>((void*)addr);
  }
};

static struct gic_params gic;

constexpr uint32_t gicc_pmr_low=0xFF;
constexpr uint32_t gicc_ctlr_disable=0x0;
constexpr uint32_t gicc_ctlr_enable=0x1;
constexpr uint32_t gicc_bpr_no_group=0x00;

constexpr uint32_t gicc_iar_irq_mask=0x3ff;
constexpr uint32_t gicc_irq_spurious=0x3ff;

static inline uint32_t gic_gicc_pending()
{
  return gic.gicc->iar&gicc_iar_irq_mask;
}

static inline void gic_gicc_clear_irq(uint32_t irq)
{
  gic.gicc->eoir=irq;// or do we need the whole thing..?;
}

void init_gicc()
{
  uint32_t pending_irq;
  //disable cpuinterface
  gic.gicc->ctlr =gicc_ctlr_disable;
  //set lowest priority
  gic.gicc->pmr = gicc_pmr_low;
  gic.gicc->bpr = gicc_bpr_no_group;

  while ((pending_irq=gic_gicc_pending()) != gicc_irq_spurious)
  {
    GIC_DEBUG("Pending irq %d\n",pending_irq);
    gic_gicc_clear_irq(pending_irq);
  }

  gic.gicc->ctlr =gicc_ctlr_enable;

}

void init_gicd()
{
  //disable gicd
  gic.gicd->ctlr=GICD_CTLR_DISABLE;

  int irq_lines=32*((gic.gicd->type&0x1F )+1);
  GIC_DEBUG("IRQ Lines %d\r\n",irq_lines);
  //rounded up
  int regs_count=(irq_lines+GIC_V3_GICD_INT_PER_REG-1)/GIC_V3_GICD_INT_PER_REG;

  /* Disable interrupts and clear pending*/
  for (auto reg=0;reg < regs_count; reg++ )
  {
    gic.gicd->enable_clr[reg]=0xffffffff; //clear_enable
    gic.gicd->pending_clr[reg]=0xffffffff; //clear pending
  }

  //rounded up
  int priority_regs=(irq_lines+GIC_V3_GICD_PRIORITY_PER_REG-1)/GIC_V3_GICD_PRIORITY_PER_REG;
  /* Set priority to lowest on all interrupts*/
  for (auto reg =0;reg < priority_regs;reg++)
  {
    GIC_DEBUG("REG %08x : %08x\r\n",&gic.gicd->priority[reg],0xffffffff);
    gic.gicd->priority[reg]=0xffffffff;
  }
  //rounded up
  int target_regs=(irq_lines+GIC_V3_GICD_TARGETS_PER_REG-1)/GIC_V3_GICD_TARGETS_PER_REG;
  //set all interrupts to target cpu0
  //from SPIO QEMU= 16
  //TODO Revisit
  for (auto reg =16/GIC_V3_GICD_TARGETS_PER_REG;reg < target_regs;reg++)
  {
    GIC_DEBUG("REG %08x : %08x\r\n",&gic.gicd->targets[reg],GIC_V3_GICD_TARGETS_CORE0_BMAP);
    gic.gicd->targets[reg]=GIC_V3_GICD_TARGETS_CORE0_BMAP;
  }

  int config_regs=(irq_lines+GIC_V3_GICD_CFGR_PER_REG-1)/GIC_V3_GICD_CFGR_PER_REG;
  //from PPIO QEMU 32
  //TODO revisit
  for (auto reg =32/GIC_V3_GICD_CFGR_PER_REG;reg < target_regs;reg++)
  {
    GIC_DEBUG("REG %08x : %08x\r\n",&gic.gicd->config[reg],GICD_ICFGR_LEVEL);
    gic.gicd->config[reg]=GICD_ICFGR_LEVEL;
  }
  //enable gicd
  gic.gicd->ctlr=GICD_CTLR_ENABLE;

}

void gic_init(uint64_t gic_gicd_base,uint64_t gic_gicc_base)
{
  GIC_INFO("Initializing gic\r\n");
  gic.set_gicc(gic_gicc_base);
  gic.set_gicd(gic_gicd_base);
  init_gicd();
  init_gicc();
}

//int gic_find_pending_irq(exception int *irq); ?
void gicd_irq_disable(int irq) //disable irq
{
  int reg=irq/GIC_V3_GICD_INT_PER_REG;
  gic.gicd->enable_clr[reg]=(1<<(irq%GIC_V3_GICD_INT_PER_REG));
}
void gicd_irq_enable(int irq) //enable irq
{
  int reg=irq/GIC_V3_GICD_INT_PER_REG;
  gic.gicd->enable_set[reg]=(1<<(irq%GIC_V3_GICD_INT_PER_REG));
}
void gicd_irq_clear(int irq) //clear pending
{
  int reg=irq/GIC_V3_GICD_INT_PER_REG;
  gic.gicd->pending_clr[reg]=(1<<(irq%GIC_V3_GICD_INT_PER_REG));
}

/**
 * processor is a bit pattern ?
  if targets per reg is 4 then 8 bits is max cpu's.. something off ?
  and cpu should probably be uint8_t ?
  0x1 processor 0
  0x2 processor 1
  0x4 processor 2
  0x8 processor 3
*/
void gicd_set_target(uint32_t irq,uint8_t processor)
{
  int shift = (irq%GIC_V3_GICD_TARGETS_PER_REG)*GIC_V3_GICD_TARGETS_SIZE_PER_REG;
  int reg = (irq/GIC_V3_GICD_TARGETS_PER_REG);
  gic.gicd->targets[reg] &= ~(0xff<<shift); //clear
  gic.gicd->targets[reg] |= ((processor&0xff)<<shift); //set new processor target
}

void gicd_set_priority(uint32_t irq,uint8_t priority)
{
  int shift = (irq%GIC_V3_GICD_PRIORITY_PER_REG)*GIC_V3_GICD_PRIORITY_SIZE_PER_REG;
  int reg = (irq/GIC_V3_GICD_PRIORITY_PER_REG);
  gic.gicd->priority[reg] &= ~(0xff<<shift); //clear
  gic.gicd->priority[reg] |= ((priority&0xff)<<shift); //set new processor target
}

void gicd_set_config(uint32_t irq,uint8_t config)
{
  int shift = (irq%GIC_V3_GICD_CFGR_PER_REG)*GIC_V3_GICD_CFGR_SIZE_PER_REG;
  int reg = (irq/GIC_V3_GICD_CFGR_PER_REG);
  gic.gicd->config[reg] &= ~(0x3<<shift); //clear
  gic.gicd->config[reg] |= ((config&0x3)<<shift);
}

int gicd_probe_pending(int irq)
{
  int reg=irq/GIC_V3_GICD_INT_PER_REG;
  int pending = ( gic.gicd->pending_set[reg] & (1<<(irq%GIC_V3_GICD_INT_PER_REG)));
  return pending;
}

int gicd_decode_irq()
{
  //worst ever way of decoding irq ?
  //check if pending reg == 0 instead of each irq
  int irq_lines=32*((gic.gicd->type&0x1F )+1);
  for (int i=0; i < irq_lines;i++)
  {
    if (gicd_probe_pending(i))
      return i;
  }
  //irq max or negative value?
  return 0x3ff;
}
