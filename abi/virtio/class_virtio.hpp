/**  
     STANDARD:      
     Virtio 1.0, OASIS Committee Specification Draft 03
     (http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html)
     
     In the following abbreviated to Virtio 1.03 or Virtio std.
*/

#ifndef CLASS_VIRTIO_HPP
#define CLASS_VIRTIO_HPP
#include <class_pci_device.hpp>


#define PAGE_SIZE 4096


//#include <class_irq_handler.hpp>
class Virtio{
  
public:    
  
  /** Virtio Queue class. */
  class Queue {
    /** @note Using typedefs in order to keep the standard notation. */
    typedef uint64_t le64;
    typedef uint32_t le32;
    typedef uint16_t le16;
    typedef uint16_t u16;
    typedef uint8_t u8;

    
    /** Virtio Ring Descriptor. Virtio std. §2.4.5  */
    struct virtq_desc { 
      /* Address (guest-physical). */ 
      le64 addr; 
      /* Length. */ 
      le32 len; 
      
      /* This marks a buffer as continuing via the next field. */ 
#define VIRTQ_DESC_F_NEXT   1 
      /* This marks a buffer as device write-only (otherwise device read-only). */ 
#define VIRTQ_DESC_F_WRITE     2 
      /* This means the buffer contains a list of buffer descriptors. */ 
#define VIRTQ_DESC_F_INDIRECT   4 
      /* The flags as indicated above. */ 
      le16 flags; 
      /* Next field if flags & NEXT */ 
      le16 next; 
    };
    
    
    /** Virtio Available ring. Virtio std. §2.4.6 */
    struct virtq_avail { 
#define VIRTQ_AVAIL_F_NO_INTERRUPT      1 
      le16 flags; 
      le16 idx; 
      le16 ring[/* Queue Size */];  
      le16 used_event; /* Only if VIRTIO_F_EVENT_IDX */ 
    };
    
    
    /** Virtio Used ring elements. Virtio std. §2.4.8 */
    struct virtq_used_elem { 
      /* le32 is used here for ids for padding reasons. */ 
      /* Index of start of used descriptor chain. */ 
      le32 id; 
      /* Total length of the descriptor chain which was used (written to) */ 
      le32 len; 
    };
    
    /** Virtio Used ring. Virtio std. §2.4.8 */
    struct virtq_used { 
#define VIRTQ_USED_F_NO_NOTIFY  1 
      le16 flags; 
      le16 idx; 
      struct virtq_used_elem ring[ /* Queue Size */]; 
      le16 avail_event; /* Only if VIRTIO_F_EVENT_IDX */ 
    }; 
    
    
    /** Virtqueue. Virtio std. §2.4.2 */
    struct virtq { 
      // The actual descriptors (16 bytes each) 
      virtq_desc* desc;// [ /* Queue Size*/  ]; 
      
      // A ring of available descriptor heads with free-running index. 
      virtq_avail* avail; 
      
      // Padding to the next PAGE_SIZE boundary. 
      u8 pad[ /* Padding */ ]; 
 
      // A ring of used descriptor heads with free-running index. 
      virtq_used* used; 
    };
    
    /** Virtque size calculation. Virtio std. §2.4.2 */
    static inline unsigned virtq_size(unsigned int qsz);
    
    //Actual size in bytes - virtq_size(size)
    int _size_bytes;
    virtq* _queue;
    void init_queue(int size, void* buf);
  public:
    Queue(int size);
  };
  

  /** Get the Virtio config registers from the PCI device.
      
      @note it varies how these are structured, hence a void* buf */
  void get_config(void* buf, int len);
  
  /** Get the (saved) device IRQ */
  inline uint8_t irq(){ return _irq; };

  /** Reset the virtio device */
  void reset();
  
  /** Signal "Driver found" */
  void sig_driver_found();

  /** Negotiate supported features with host */
  void negotiate_features(uint32_t features);
  
  /** Register interrupt handler & enable IRQ */
  //void enable_irq_handler(IRQ_handler::irq_delegate d);
  void enable_irq_handler();

  /** Probe PCI device for features */
  uint32_t probe_features();
  
  /** Get locally stored features */
  uint32_t features();
  
  /** Get iobase. Wrapper around PCI_Device::iobase */
  inline uint32_t iobase(){ return _iobase; }

  /** Kick hypervisor.
   
      Will notify the host (Qemu/Virtualbox etc.) about pending data  */
  inline void kick();
  
  Virtio(PCI_Device* pci);

private:
  //PCI memer as reference (so no indirection overhead)
  PCI_Device& _pcidev;
  
  //We'll get this from PCI_device::iobase(), but that lookup takes longer
  uint32_t _iobase;  
  uint8_t _irq = 0;
  uint32_t _features;

  void set_irq();

  //TEST
  int calls = 0;
  
  void default_irq_handler();
  

  
};

#endif

