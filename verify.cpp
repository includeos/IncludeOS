#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "crc32.h"

int main(int, char *[])
{
  CRC32_BEGIN(x);
  for (int i = 0; i < 1000; i++) {
    int len = 4096;
    auto* buf = new char[len];
    memset(buf, 'A', len);
    x = crc32(x, buf, len);
    delete[] buf;
  }
  printf("CRC32 should be: %08x\n", CRC32_VALUE(x));
    
    const uint16_t PORT = 6667;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "10.0.0.42", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(PORT);
    
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        perror("ERROR connecting");
    
    printf("Calculating CRC32...\n");
    uint32_t crc = 0xFFFFFFFF;
    while (true)
    {
      char buffer[256];
      int n = read(sockfd,buffer, 4096);
      if (n < 0) perror("ERROR reading from socket");
      
      if (n == 0) break;
      
      // update CRC32 partially
      crc = crc32(crc, buffer, n);
      
      printf("Partial CRC: %08x\n", ~crc);
    }
    crc = ~crc;
    
    printf("Final CRC: %08x\n", crc);
    close(sockfd);
    return 0;
}
