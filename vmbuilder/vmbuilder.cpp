#include <iostream>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <elf.h>

#define SECT_SIZE 512


using namespace std;

const char* bootloc="./bootloader";
const char* srvloc="./service";
const char* imgloc="./image";

const int offs_srvsize=2;
const int offs_srvoffs=6;

int main(){
  cout << endl << "Creating VM disk image" << endl << endl;
  
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
  if(stat(srvloc, &stat_srv)==-1){
    cout << "Could not open " << srvloc << " - exiting. " << endl;
    return errno;
  }

  int srv_sect=stat_srv.st_size / SECT_SIZE + 
    (stat_srv.st_size % SECT_SIZE > 0 ? 1 : 0);
  cout << "Size of service: \t" 
       << stat_srv.st_size 
       << " bytes, => " << srv_sect
       << " sectors. "<< endl;

  int img_size=(boot_sect+srv_sect)*SECT_SIZE;
  cout << "Total disk size: \t" 
       << img_size
       << " bytes, => "
       << boot_sect+srv_sect
       << " sectors. " 
       << endl;

  //int disksize=boot_sect*SECT_SIZE + srv_sect*SECT_SIZE;
  /* 
     Bochs requires old-school disk specifications. 
     sectors=cyls*heads*spt (sectors per track)
  */
  int cylinders=32, heads=1, spt=63;
  int disksize=cylinders*heads*spt*SECT_SIZE;
  
  if(disksize<img_size){
    cout << endl << " ---- ERROR ----" << endl
	 << "Image is too big for the disk! " << endl
	 << "Image size: " << img_size << " B" << endl
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
  FILE* file_srv=fopen(srvloc,"r");
  
  //Location of service code within the image
  char* srv_imgloc=disk+(boot_sect*SECT_SIZE);  
  cout << "Read " << fread(srv_imgloc,
			   1,stat_srv.st_size,file_srv)
       << " sectors from service image"<< endl;  
  
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
  case(EM_X86_64) : "Intel x86_64" ;
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
  cout << "Pheader 1, location: " << prog_hdr->p_offset << endl;
  //int srv_start=prog_hdr->p_offset;
  int srv_start=elf_header->e_entry; //prog_hdr->p_offset;
  //int srv_start=0;
  
  //Write OS/Service size to the bootloader
  *((int*)(disk+offs_srvsize))=srv_sect;
  *((int*)(disk+offs_srvoffs))=srv_start;
    
  int* magic_loc=(int*)(disk+img_size);
		  
  cout << "Applying magic signature: 0xFA7CA7" << endl
       << "Data currently at location: " << img_size << endl
       << "Location on image: 0x" << hex << img_size << endl
       << "Computed memory location: " 
       << hex << img_size + 0x8000 << endl;
  *magic_loc=0xFA7CA7;
  
  //Write the image
  FILE* image=fopen(imgloc,"w");
  int wrote=fwrite((void*)disk,1,disksize,image);
  cout << "Wrote " 
       << wrote
       << " bytes => " << wrote / SECT_SIZE
       <<" sectors to "<< imgloc << endl;

  

  //Cleanup
  fclose(file_boot);
  fclose(file_srv);  
  fclose(image);
  delete[] disk;
  
}
