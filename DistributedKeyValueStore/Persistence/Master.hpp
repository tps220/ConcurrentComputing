#pragma once
#include <iostream>
#include <pthread.h>
#include <sys/types.h>
       #include <ifaddrs.h>
#include "locks.hpp"

struct Master {
  bool isMaster;
  barrier_t barrier;
  Master() : isMaster(false) {}
};

void check_host_name(int hostname) { //This function returns host name for local computer
   if (hostname == -1) {
      perror("gethostname");
      exit(1);
   }
}
void check_host_entry(struct hostent * hostentry) { //find host info from host name
   if (hostentry == NULL){
      perror("gethostbyname");
      exit(1);
   }
}
void IP_formatter(char *IPbuffer) { //convert IP string to dotted decimal format
   if (NULL == IPbuffer) {
      perror("inet_ntoa");
      exit(1);
   }
}

bool isMaster(char* targetIP) {
  struct ifaddrs *ifaddr, *ifa;
  int family, s, n;
  char host[NI_MAXHOST];
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }

  bool hasSameAddress = false;
  /* Walk through linked list, maintaining head pointer so we can free list later */
  for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL) {
      continue;
    }
    family = ifa->ifa_addr->sa_family;
    /* For an AF_INET* interface address, display the address */
    if (family == AF_INET || family == AF_INET6) {
      s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      if (s != 0) {
        printf("getnameinfo() failed: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
      }
      hasSameAddress |= !strcmp(host, targetIP);
    }
  }
  freeifaddrs(ifaddr);
  return hasSameAddress;
}