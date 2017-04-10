// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
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

#ifndef POSIX_NETDB_H
#define POSIX_NETDB_H

#ifdef __cplusplus
extern "C" {
#endif

struct hostent {
  char *h_name; // Official name of the host
  char **h_aliases; // Pointer to array of pointers to alternative host names
  int h_addrtype; // Address type
  int h_length; // Length of address
  char  **h_addr_list; // Pointer to array of pointers to network addresses
};

struct netent {
  char *n_name; // FQDN of host
  char **n_aliases; // Pointer to array of pointers to alternative network names
  int n_addrtype; // Address type
  uint32_t n_net; // Network number
};

struct protoent {
  char *p_name; // Name of protocol
  char **p_aliases; // Pointer to array of pointers to alternative protocol names
  int p_proto; // Protocol number
};

struct servent {
  char *s_name; // Name of service
  char **s_aliases; // Pointer to array of pointers to alternative service names
  int s_port; // Port number
  char *s_proto; //Name of protocol
};

struct addrinfo {
  int ai_flags; // Input flags
  int ai_family; // Address family
  int ai_socktype; // Socket type
  int ai_protocol; // Protocol
  socklen_t ai_addrlen; // Length of socket address
  struct sockaddr *ai_addr; // Socket address
  char *ai_canonname; // Canonical name of service location
  struct addrinfo *ai_next; // Pointer to next in list
};

// Constants for use in flags field of addrinfo structure
#define AI_PASSIVE 1
#define AI_CANONNAME 2
#define AI_NUMERICHOST 4
#define AI_NUMERICSERV 8
#define AI_V4MAPPED 16
#define AI_ALL 32
#define AI_ADDRCONFIG 64

// Constants for use in flags argument to getnameinfo()
#define NI_NOFQDN 1
#define NI_NUMERICHOST 2
#define NI_NAMEREQD 4
#define NI_NUMERICSERV 8
#define NI_NUMERICSCOPE 16
#define NI_DGRAM 32

// Constants for error values for getaddrinfo() and getnameinfo()
#define EAI_AGAIN -1
#define EAI_BADFLAGS -2
#define EAI_FAIL -3
#define EAI_FAMILY -4
#define EAI_MEMORY -5
#define EAI_NONAME -6
#define EAI_SERVICE -7
#define EAI_SOCKTYPE -8
#define EAI_SYSTEM -9
#define EAI_OVERFLOW -10

void endhostent(void);
void endnetent(void);
void endprotoent(void);
void endservent(void);
void freeaddrinfo(struct addrinfo *);
const char *gai_strerror(int);
int getaddrinfo(const char *__restrict__, const char *__restrict__,
  const struct addrinfo *__restrict__, struct addrinfo **__restrict__);
struct hostent *gethostent(void);
int getnameinfo(const struct sockaddr *__restrict__, socklen_t, char *__restrict__,
   socklen_t, char *__restrict__, socklen_t, int);
struct netent *getnetbyaddr(uint32_t, int);
struct netent *getnetbyname(const char *);
struct netent *getnetent(void);
struct protoent *getprotobyname(const char *);
struct protoent *getprotobynumber(int);
struct protoent *getprotoent(void);
struct servent *getservbyname(const char *, const char *);
struct servent*getservbyport(int, const char *);
struct servent *getservent(void);
void sethostent(int);
void setnetent(int);
void setprotoent(int);
void setservent(int);

#ifdef __cplusplus
}
#endif

#endif // POSIX_NETDB_H
