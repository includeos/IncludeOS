//
// pci.h
//
// PCI bus interface
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
//
//  > Ported to IncludeOS by Alfred Bratterud 2014
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
// 

#ifndef PCI_H
#define PCI_H

#include "dev.h"


//
// Ports for access to PCI config space
//

#define PCI_CONFIG_ADDR                 0xCF8
#define PCI_CONFIG_DATA                 0xCFC

//
// PCI config space register offsets
//

#define PCI_CONFIG_VENDOR               0x00
#define PCI_CONFIG_CMD_STAT             0x04
#define PCI_CONFIG_CMD                  0x04    // 16 bits
#define PCI_CONFIG_STATUS               0x06    // 16 bits
#define PCI_CONFIG_CLASS_REV            0x08
#define PCI_CONFIG_HDR_TYPE             0x0C
#define PCI_CONFIG_CACHE_LINE_SIZE      0x0C    // 8 bits
#define PCI_CONFIG_LATENCY_TIMER        0x0D    // 8 bits
#define PCI_CONFIG_BASE_ADDR_0          0x10
#define PCI_CONFIG_BASE_ADDR_1          0x14
#define PCI_CONFIG_BASE_ADDR_2          0x18
#define PCI_CONFIG_BASE_ADDR_3          0x1C
#define PCI_CONFIG_BASE_ADDR_4          0x20
#define PCI_CONFIG_BASE_ADDR_5          0x24
#define PCI_CONFIG_CIS                  0x28
#define PCI_CONFIG_SUBSYSTEM            0x2C
#define PCI_CONFIG_ROM                  0x30
#define PCI_CONFIG_CAPABILITIES         0x34
#define PCI_CONFIG_INTR                 0x3C
#define PCI_CONFIG_INTERRUPT_LINE       0x3C    // 8 bits
#define PCI_CONFIG_INTERRUPT_PIN        0x3D    // 8 bits
#define PCI_CONFIG_MIN_GNT              0x3E    // 8 bits
#define PCI_CONFIG_MAX_LAT              0x3F    // 8 bits

#define PCI_BASE_ADDRESS_MEM_MASK       (~0x0FUL)
#define PCI_BASE_ADDRESS_IO_MASK        (~0x03UL)

//
// PCI Command
//

#define  PCI_COMMAND_IO                 0x0001   // Enable response in I/O space
#define  PCI_COMMAND_MEMORY             0x0002   // Enable response in Memory space
#define  PCI_COMMAND_MASTER             0x0004   // Enable bus mastering
#define  PCI_COMMAND_SPECIAL            0x0008   // Enable response to special cycles
#define  PCI_COMMAND_INVALIDATE         0x0010   // Use memory write and invalidate
#define  PCI_COMMAND_VGA_PALETTE        0x0020   // Enable palette snooping
#define  PCI_COMMAND_PARITY             0x0040   // Enable parity checking
#define  PCI_COMMAND_WAIT               0x0080   // Enable address/data stepping
#define  PCI_COMMAND_SERR               0x0100   // Enable SERR/
#define  PCI_COMMAND_FAST_BACK          0x0200   // Enable back-to-back writes

//
// PCI Status
//

#define  PCI_STATUS_CAP_LIST            0x010  // Support Capability List
#define  PCI_STATUS_66MHZ               0x020  // Support 66 Mhz PCI 2.1 bus
#define  PCI_STATUS_UDF                 0x040  // Support User Definable Features [obsolete]
#define  PCI_STATUS_FAST_BACK           0x080  // Accept fast-back to back
#define  PCI_STATUS_PARITY              0x100  // Detected parity error
#define  PCI_STATUS_DEVSEL_MASK         0x600  // DEVSEL timing
#define  PCI_STATUS_DEVSEL_FAST         0x000   
#define  PCI_STATUS_DEVSEL_MEDIUM       0x200
#define  PCI_STATUS_DEVSEL_SLOW         0x400

//
// PCI Capability lists
//

#define PCI_CAP_LIST_ID                 0       // Capability ID
#define PCI_CAP_LIST_NEXT               1       // Next capability in the list
#define PCI_CAP_FLAGS                   2       // Capability defined flags (16 bits)
#define PCI_CAP_SIZEOF                  4

#define PCI_CAP_ID_PM                   0x01    // Power Management
#define PCI_CAP_ID_AGP                  0x02    // Accelerated Graphics Port
#define PCI_CAP_ID_VPD                  0x03    // Vital Product Data
#define PCI_CAP_ID_SLOTID               0x04    // Slot Identification
#define PCI_CAP_ID_MSI                  0x05    // Message Signalled Interrupts
#define PCI_CAP_ID_CHSWP                0x06    // CompactPCI HotSwap

//
// PCI Power Management
//

#define PCI_PM_CTRL                     4       // PM control and status register
#define PCI_PM_CTRL_STATE_MASK          0x0003  // Current power state (D0 to D3)

//
// PCI device codes
//

#define PCI_CLASS_MASK          0xFF0000
#define PCI_SUBCLASS_MASK       0xFFFF00

#define PCI_HOST_BRIDGE         0x060000
#define PCI_BRIDGE              0x060400
#define PCI_ISA_BRIDGE          0x060100

#define PCI_CLASS_STORAGE_IDE   0x010100

#define PCI_ID_ANY              0xFFFFFFFF

#define PCI_UNITCODE(vendorid, deviceid) ((vendorid) << 16 | (deviceid))
#define PCI_UNITNO(devno, funcno) ((devno) << 3 | (funcno))

#define PCI_VENDOR_ID(unitcode) (((unitcode) >> 16) & 0xFFFF)
#define PCI_DEVICE_ID(unitcode) ((unitcode) & 0xFFFF)

#define PCI_DEVNO(unitno) ((unitno) >> 3)
#define PCI_FUNCNO(unitno) ((unitno) & 7)

unsigned char pci_read_config_byte(struct unit *unit, int addr);
unsigned short pci_read_config_word(struct unit *unit, int addr);
unsigned long pci_read_config_dword(struct unit *unit, int addr);

void pci_write_config_byte(struct unit *unit, int addr, unsigned char value);
void pci_write_config_word(struct unit *unit, int addr, unsigned short value);
void pci_write_config_dword(struct unit *unit, int addr, unsigned long value);

void pci_read_buffer(struct unit *unit, int addr, void *buffer, int len);
void pci_write_buffer(struct unit *unit, int addr, void *buffer, int len);

void pci_enable_busmastering(struct unit *unit);

void enum_pci_bus(struct bus *bus);
unsigned long get_pci_hostbus_unitcode();

#endif
