#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <stdio.h>
 
#define BUFFER_LEN   512
#define SOCKET_ERROR  -1
#define ROUNDS         5

void die(const char* s)
{
  perror(s);
  exit(1);
}
 
int main(int argc, char** argv)
{
  if (argc < 3)
  {
    printf("%s\t [dest] [port]\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  const char* SERVICE      = argv[1];
  const int   SERVICE_PORT = atoi(argv[2]);
  
  // create UDP socket
  int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s == SOCKET_ERROR)
  {
    die("socket");
  }
  
  struct sockaddr_in si_other;
  
  // populate remote
  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(SERVICE_PORT);
  if (inet_aton(SERVICE, &si_other.sin_addr) == 0)
  {
    fprintf(stderr, "inet_aton() failed\n");
    exit(1);
  }
  
  char* buf = new char[BUFFER_LEN]();
  
  int rounds = ROUNDS;
  while (rounds--)
  {
    printf("[send] ");
    fflush(stdout);
    
    // blocking send
    socklen_t slen = sizeof(si_other);
    int res = 
      sendto(s, buf, BUFFER_LEN, 0, (struct sockaddr*) &si_other, slen);
    
    if (res == SOCKET_ERROR)
    {
      die("sendto()");
    }
    
    printf("[recv] ");
    fflush(stdout);
    
    // blocking read
    int recv_len =
      recvfrom(s, buf, BUFFER_LEN, 0, (struct sockaddr *) &si_other, &slen);
    
    if (recv_len == SOCKET_ERROR)
    {
        die("recvfrom()");
    }
    
    printf("Packet from %s:%d\n", 
            inet_ntoa(si_other.sin_addr), 
            ntohs(si_other.sin_port));
    printf("Data: %s\n", buf);
  }
  
  delete[] buf;
  close(s);
  return 0;
}
