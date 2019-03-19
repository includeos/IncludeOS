#if !defined(GIC_REGS)
#define GIC_REGS

//SKIP PER_REG ?
#define GIC_V3_GICD_INT_PER_REG           32
#define GIC_V3_GICD_PRIORITY_PER_REG      4
#define GIC_V3_GICD_PRIORITY_SIZE_PER_REG 8
#define GIC_V3_GICD_TARGETS_CORE0_BMAP    0x01010101 //cpu interface 0 what about the rest
#define GIC_V3_GICD_TARGETS_PER_REG       4
#define GIC_V3_GICD_TARGETS_SIZE_PER_REG  8
#define GIC_V3_GICD_CFGR_PER_REG          16
#define GIC_V3_GICD_CFGR_SIZE_PER_REG     2
/** these are all the same as INT_PER_REG */
#define GIC_V3_GICD_SET_ENABLE            32
#define GIC_V3_GICD_CLR_ENABLE            32
#define GIC_V3_GICD_CLR_PENDING           32
#define GIC_V3_GICD_SET_PENDING           32


#define GICD_CTLR_ENABLE    0x1
#define GICD_CTLR_DISABLE   0x0

/*
#define GIC_V3_GIC_CTLR  0x000
#define GIC_V3_GIC_PMR   0x000 //Interrupr priority mask register
#define GIC_V3_GIC_CTLR  0x000
#define GIC_V3_GIC_CTLR  0x000
#define GIC_V3_GIC_CTLR  0x000
#define GIC_V3_GIC_CTLR  0x000
*/
//is this per cpu or in general ?
/** CPU interface register map */
struct gic_v3_cpu_interface_gicc {
  ///CPU interface control reg
  volatile uint32_t ctlr;
  ///Interupt priority mask
  volatile uint32_t pmr;
  ///Binary point register
  volatile uint32_t bpr;
  ///Interrupt ack reg
  volatile uint32_t iar;
  ///End of interrupt register
  volatile uint32_t eoir;
  ///Running Priority register
  volatile uint32_t rpr;
  ///Highest Pending Interrupt register
  volatile uint32_t hpir;
  ///Aliased Binary Point Register
  volatile uint32_t abpr;
  ///CPU Interface Identification register
  volatile uint32_t iidr;
};

#define GICC_CTLR_ENABLE    0x1
#define GICC_CTLR_DISABLE   0x2
#define GICC_BPR_NO_GROUP   0x0
#define GICC_PMR_PRIO_LOW   0xFF
#define GICC_PMR_PRIO_HIGH  0x00
#define GICC_IAR_IRQ_MASK   0x3ff
//1023=spuriuous
#define GICC_IAR_SPURIOUS   0x3ff

/** GID distributor register map */
struct gic_v3_distributor_map {
  /* distributor control register */
  volatile uint32_t ctlr;
  /**Interrupt controller type register */
  volatile uint32_t type;
  /** Distributor implementer identifier register*/
  volatile uint32_t idr;
  //spacing
  volatile uint32_t reserved[29]; //should end the next at 0x80
  /** Interrupt group registers 0x080*/
  volatile uint32_t group[32];
  /** Interrupt set enable registers 0x100*/
  volatile uint32_t enable_set[32];
  /** Interrupt clear enable registers 0x180*/
  volatile uint32_t enable_clr[32];
  /** Interrupt set pending registers 0x200*/
  volatile uint32_t pending_set[32];
  /** Interrupt clr pending registers 0x280*/
  volatile uint32_t pending_clr[32];
  /** Interrupt set active registers 0x300*/
  volatile uint32_t active_set[32];
  /** Interrupt clr active registers 0x380**/
  volatile uint32_t active_clr[32];
  /** Interrupt priority registers 0x400 - 0x800 ?*/
  volatile uint32_t priority[32*8];
  /**0x800 Interrupt processor targets*/
  volatile uint32_t targets[32*8];
  /**0xC00 Interrupt configuration registers*/
  volatile uint32_t config[32*4]; //+0x80
  /**0xE00 Non secure access control reg*/
  volatile uint32_t nonsecure[32];
  volatile uint32_t reserved4[32*3];//+(0x200-0x80)
  volatile uint32_t soft_int; // 0xf00
  volatile uint32_t reserved5[3];//3 is correct
  volatile uint32_t soft_pending_clr[4]; //0xf10 0x10
  volatile uint32_t soft_pending_set[4]; //0xf20 10
};




#endif
