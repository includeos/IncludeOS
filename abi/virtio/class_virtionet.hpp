/**  
     STANDARD:      
     
     We're aiming for standard compliance:
     
     Virtio 1.0, OASIS Committee Specification Draft 01
     (http://docs.oasis-open.org/virtio/virtio/v1.0/csd01/virtio-v1.0-csd01.pdf)
     
     In the following abbreviated to Virtio 1.01
     
     ...Alas, nobody's using it yet, so we're stuck with "legacy" for now.
*/
     

#ifndef CLASS_VIRTIONET_H
#define CLASS_VIRTIONET_H


#include <common>
#include <class_pci_device.hpp>
#include <virtio/class_virtio.hpp>
#include <delegate>

#include <net/class_ethernet.hpp>
#include <net/class_arp.hpp>

/** Virtio Net Features. From Virtio Std. 5.1.3 */

/* Device handles packets with partial checksum. This “checksum offload” 
   is a common feature on modern network cards.*/
#define VIRTIO_NET_F_CSUM 0

/* Driver handles packets with partial checksum. */
#define VIRTIO_NET_F_GUEST_CSUM 1

/* Control channel offloads reconfiguration support. */
#define VIRTIO_NET_F_CTRL_GUEST_OFFLOADS 2

/* Device has given MAC address. */
#define VIRTIO_NET_F_MAC 5

/* Driver can receive TSOv4. */
#define VIRTIO_NET_F_GUEST_TSO4 7

/* Driver can receive TSOv6. */
#define VIRTIO_NET_F_GUEST_TSO6 8

/* Driver can receive TSO with ECN.*/
#define VIRTIO_NET_F_GUEST_ECN 9

/* Driver can receive UFO. (UFO?? WTF!) */
#define VIRTIO_NET_F_GUEST_UFO 10

/* Device can receive TSOv4. */
#define VIRTIO_NET_F_HOST_TSO4 11

/* Device can receive TSOv6. */
#define VIRTIO_NET_F_HOST_TSO6 12

/* Device can receive TSO with ECN. */
#define VIRTIO_NET_F_HOST_ECN 13

/* Device can receive UFO.*/
#define VIRTIO_NET_F_HOST_UFO 14

/* Driver can merge receive buffers. */
#define VIRTIO_NET_F_MRG_RXBUF 15

/* Configuration status field is available. */
#define VIRTIO_NET_F_STATUS 16

/* Control channel is available.*/
#define VIRTIO_NET_F_CTRL_VQ 17

/* Control channel RX mode support.*/
#define VIRTIO_NET_F_CTRL_RX 18

/* Control channel VLAN filtering.*/
#define VIRTIO_NET_F_CTRL_VLAN 19

/* Driver can send gratuitous packets.*/
#define VIRTIO_NET_F_GUEST_ANNOUNCE 21

/* Device supports multiqueue with automatic receive steering.*/
#define VIRTIO_NET_F_MQ 22

/* Set MAC address through control channel.*/
#define VIRTIO_NET_F_CTRL_MAC_ADDR 23


// From Virtio 1.01, 5.1.4
#define VIRTIO_NET_S_LINK_UP  1
#define VIRTIO_NET_S_ANNOUNCE 2


#define MTUSIZE 1514

/** Virtio-net device driver.  */
class VirtioNet : Virtio {
  
  struct virtio_net_hdr
  {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;          // Ethernet + IP + TCP/UDP headers
    uint16_t gso_size;         // Bytes to append to hdr_len per frame
    uint16_t csum_start;       // Position to start checksumming from
    uint16_t csum_offset;      // Offset after that to place checksum
    uint16_t num_buffers;
  };
  

  PCI_Device* dev;
  
  Virtio::Queue rx_q;
  Virtio::Queue tx_q;
  Virtio::Queue ctrl_q;

  // Moved to Nic
  // Ethernet eth; 
  // Arp arp;

  // From Virtio 1.01, 5.1.4
  struct config{
    mac_t mac = {0};
    uint16_t status;
    
    //Only valid if VIRTIO_NET_F_MQ
    uint16_t max_virtq_pairs = 0; 
  }_conf;
  
  //sizeof(config) if VIRTIO_NET_F_MQ, else sizeof(config) - sizeof(uint16_t)
  int _config_length = sizeof(config);
  
  void get_config();
  
  void receive_data(void* data, uint32_t len);

  char* _mac_str=(char*)"00:00:00:00:00:00";
  int _irq = 0;
  
  
  void irq_handler();
  int add_receive_buffer();

  
public: 
  const char* name();  
  const mac_t& mac();
  const char* mac_str();
  
  VirtioNet(PCI_Device* pcidev, Ethernet& eth);


};

#endif
