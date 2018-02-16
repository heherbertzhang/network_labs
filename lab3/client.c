#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

float getRTT(int sockfd, char* msg);

int main(int argc, char *argv[])
{
  int sockfd;
  struct addrinfo hints, *res;
  
  // first, load up address structs with getaddrinfo():
  
  printf("Starting\n");
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_DGRAM; // UDP
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
  
  getaddrinfo("localhost", "5555", &hints, &res);
  
  // make a socket
  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  
  // connect!
  connect(sockfd, res->ai_addr, res->ai_addrlen);

  float RTT1 = getRTT(sockfd, "hi"); // 2 bytes = 16 bits
  printf("RTT is %f\n", RTT1);
  float RTT2 = getRTT(sockfd, "hihihihihihihihihihi"); // 20 bytes = 160 bits 
  printf("RTT is %f\n", RTT2);
  // RTT / 2 = transmission time + propagation time
  // RTT2/2 - RTT1/2 = transmission time for 20-2 bytes = 144bits
  // transmission rate = 144bits / timespent
  // propatation delay = RTT/2 - transmission time
  float R = 144.0 / ((float)(RTT2-RTT1)/2.0);
  float t_prop = RTT1/2.0 - 16.0/R;
  float t_prop2 = RTT2/2.0 - 160.0/R;
  printf("R is %f, and t_prop1 is %f which should be same as t_prop2 %f\n", \
    R, t_prop, t_prop2);
}

float getRTT(int sockfd, char* msg) {
  size_t BUFSIZE = 1000;
  int bytes_sent, bytes_received;
  struct timeval curtime, echotime;
  char buffer[BUFSIZE];
  gettimeofday(&curtime, NULL);
  bytes_sent = send(sockfd, msg, strlen(msg), 0);
  bytes_received = recv(sockfd, buffer, BUFSIZE, 0);
  gettimeofday(&echotime, NULL);
  return (float)(echotime.tv_sec-curtime.tv_sec) + \
    (((float)(echotime.tv_usec-curtime.tv_usec))/1000000.0);
}
