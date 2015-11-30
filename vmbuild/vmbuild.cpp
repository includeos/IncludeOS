// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <iostream>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <elf.h>
#include <stdlib.h>

#define SECT_SIZE 512


using namespace std;

const int offs_srvsize=2;
const int offs_srvoffs=6;

bool test=false;

const string info="Create a bootable disk image for IncludeOS.";
const string usage="Usage: buildvm <bootloader> <service_binary> [test=false]";

int main(int argc, char** argv){
  
  if(argc < 3){
    cout << info << endl << usage << endl; 
    exit(0);
  }
  
  const char* bootloc=argv[1];
  const string srvloc=string(argv[2]);
  string img_name=srvloc.substr(srvloc.find_last_of("/")+1,string::npos)+".img";

  //const char* imgloc=img_name.c_str();
  

  cout << endl << "Creating VM disk image './" << img_name << "'" << endl << endl;


  if(argc > 3 && string(argv[3])=="test"){
    test=true;
    cout << "*** TEST MODE *** " << endl;
  }

  
  struct stat stat_boot;
  struct stat stat_srv;

  //Verifying boot loader
  if(stat(bootloc,&stat_boot) == -1){
    cout << "Could not open " << bootloc << " - exiting" << endl;
    return errno;
  }    
  
  if(stat_boot.st_size > SECT_SIZE){
    cout << "Boot sector too big! (" 
	 << stat_boot.st_size << " bytes)" << endl;
    return 99;
  }

  int boot_sect=stat_boot.st_size / SECT_SIZE + 
    (stat_boot.st_size % SECT_SIZE > 0 ? 1 : 0);  
  
  cout << "Size of bootloader:\t " 
       << stat_boot.st_size 
       << " bytes, => " << boot_sect
       << " sectors." << endl;

  
  //Verifying service 
  if(stat(srvloc.c_str(), &stat_srv)==-1){
    cout << "Could not open " << srvloc << " - exiting. " << endl;
    return errno;
  }

  int srv_sect=stat_srv.st_size / SECT_SIZE + 
    (stat_srv.st_size % SECT_SIZE > 0 ? 1 : 0);
  cout << "Size of service: \t" 
       << stat_srv.st_size 
       << " bytes, => " << srv_sect
       << " sectors. "<< endl;

  int img_size_sect = boot_sect+srv_sect;
  int img_size_bytes =(boot_sect+srv_sect)*SECT_SIZE;
  cout << "Total disk size: \t" 
       << img_size_bytes
       << " bytes, => "
       << boot_sect+srv_sect
       << " sectors. " 
       << endl;

  /* 
     Bochs requires old-school disk specifications. 
     sectors=cyls*heads*spt (sectors per track)
  */
  int heads = 1, spt = 63; // These can be constant (now the simplest defaults)
  int cylinders = (img_size_sect % spt) == 0 ? 
    (img_size_sect / spt) : // Sector count is a multiple of 63
    (img_size_sect / spt) + 1; // There's a remainder, so we add one track
  int disksize = cylinders * heads * spt * SECT_SIZE;
  //int disksize = img_size;
  
  if(disksize<img_size_bytes){
    cout << endl << " ---- ERROR ----" << endl
	 << "Image is too big for the disk! " << endl
	 << "Image size: " << img_size_bytes << " B" << endl
	 << "Disk size: " << disksize << " B" << endl;
    exit(999);
  }
  
  cout << "Creating disk of size: "
       << "Cyls: " << cylinders << endl
       << "Heads: " << heads << endl
       << "Sec/Tr:" << spt << endl
       << "=> " << disksize/SECT_SIZE << "sectors" << endl
       << "=> " << disksize << " bytes" << endl;
  
  char* disk=new char[disksize];
  
  
  //Zero-initialize:
  for(int i=0;i<disksize;i++)
    disk[i]=0;
  
  //Load the boot loader into memory
  FILE* file_boot=fopen(bootloc,"r");  
  cout << "Read " << fread(disk,1,stat_boot.st_size,file_boot) 
       << " bytes from boot image"<< endl;

  //Load the service into memory
  FILE* file_srv=fopen(srvloc.c_str(),"r");
  
  //Location of service code within the image
  char* srv_imgloc=disk+(boot_sect*SECT_SIZE);  
  cout << "Read " << fread(srv_imgloc,
			   1,stat_srv.st_size,file_srv)
       << " bytes from service image"<< endl;  
  
  /* 
     ELF Header summary
  */
  Elf32_Ehdr* elf_header=(Elf32_Ehdr*)srv_imgloc;
  cout << "Reading ELF headers..." << endl;
  cout << "Signature: ";
  for(int i=0;i<EI_NIDENT;i++)
    cout << elf_header->e_ident[i];  
  cout << endl;
  cout << "Type: " << (elf_header->e_type==ET_EXEC ? " ELF Executable " : "Non-executable") << endl;
  cout << "Machine: ";
  switch(elf_header->e_machine){
  case(EM_386): cout <<  "Intel 80386";
    break;
  case(EM_X86_64): cout << "Intel x86_64" ;
    break;
  default:
    cout << "UNKNOWN (" << elf_header->e_machine << ")";
    break;
  }
  cout << endl;
  cout << "Version: " << elf_header->e_version << endl;
  cout << "Entry point: 0x" << hex << elf_header->e_entry << endl;
  cout << "Number of program headers: " << elf_header->e_phnum << endl;
  cout << "Program header offset: " << elf_header->e_phoff << endl;
  cout << "Number of section headers: " << elf_header->e_shnum << endl;
  cout << "Section hader offset: " << elf_header->e_shoff << endl;
  cout << "Size of ELF-header: " << elf_header->e_ehsize << " bytes" << endl;
  
  /* Print a summary of the ELF-sections */
  /*
  for(int i=0;i<elf_header->e_shnum;i++){
    Elf32_Shdr* sect_hdr=(Elf32_Shdr*)(srv_imgloc+elf_header->e_shoff+(i*elf_header->e_ehsize));
    cout << "Section " << sect_hdr->sh_name << " : 0x" << sect_hdr->sh_offset << " + " << sect_hdr->sh_addr << endl;
    }*/
  
  // END Elf-header summary
  
  cout << endl << "Fetching offset of section .text (the service starting point)" << endl;
  
  Elf32_Phdr* prog_hdr=(Elf32_Phdr*)(srv_imgloc+elf_header->e_phoff);
  cout << "Starting at pheader 1, phys.addr: 0x" << hex << prog_hdr->p_paddr << endl;
  //int srv_start=prog_hdr->p_offset; //The offset
  // prog_hdr->p_paddr; //The physical address
  int srv_start=  elf_header->e_entry;
  //int srv_start=0;
  
  //Write OS/Service size to the bootloader
  *((int*)(disk+offs_srvsize))=srv_sect;
  *((int*)(disk+offs_srvoffs))=srv_start;
    
  int* magic_loc=(int*)(disk+img_size_bytes);
		  
  cout << "Applying magic signature: 0xFA7CA7" << endl
       << "Data currently at location: " << img_size_bytes << endl
       << "Location on image: 0x" << hex << img_size_bytes << endl
       << "Computed memory location: " 
       << hex << img_size_bytes -512 + 0x8000  << endl;
  *magic_loc=0xFA7CA7;

  if(test){
    cout << endl << "TEST overwriting service with testdata" << endl;
    for(int i=0;i<img_size_bytes-512;i++)
      disk[512+i]=i%256;
  }

  
  //Write the image
  FILE* image=fopen(img_name.c_str(),"w");
  int wrote=fwrite((void*)disk,1,disksize,image);
  cout << "Wrote " 
       << dec << wrote
       << " bytes => " << wrote / SECT_SIZE
       <<" sectors to "<< img_name << endl;


  //Cleanup
  fclose(file_boot);
  fclose(file_srv);  
  fclose(image);
  delete[] disk;
  
}
