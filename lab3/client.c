#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

float getRTT(int sockfd, char* msg, struct addrinfo* p);
int fillBuff(char* buf, int size) {
  int i = 0;
  for(i = 0; i < size+1; i++){
    buf[i] = 'a';
  }
  buf[size] = '\0';
}

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
  //connect(sockfd, res->ai_addr, res->ai_addrlen);

  int msg1_size = 1000;
  char msg1[msg1_size+1];
  fillBuff(msg1, msg1_size);
  float RTT1 = getRTT(sockfd, msg1, res); // 2 bytes = 16 bits
  printf("RTT is %f\n", RTT1);
  int msg2_size = 1050;
  char msg2[msg2_size+1];
  fillBuff(msg2, msg2_size);
  float RTT2 = getRTT(sockfd, msg2, res); // 20 bytes = 160 bits 
  printf("RTT is %f\n", RTT2);
  // RTT / 2 = transmission time + propagation time
  // RTT2/2 - RTT1/2 = transmission time for 20-2 bytes = 144bits
  // transmission rate = 144bits / timespent
  // propatation delay = RTT/2 - transmission time
  float R = (msg2_size-msg1_size)*8 / ((float)(RTT2-RTT1)/2.0);
  float t_prop = RTT1/2.0 - msg1_size*8.0/R;
  float t_prop2 = RTT2/2.0 - msg2_size*8.0/R;
  printf("R is %f, and t_prop1 is %f which should be same as t_prop2 %f\n", \
    R, t_prop, t_prop2);
}

float getRTT(int sockfd, char* msg, struct addrinfo* p) {
  size_t BUFSIZE = 5000;
  int bytes_sent=0, bytes_received=0;
  struct timeval curtime, echotime;
  char buffer[BUFSIZE];

  
  // connectors address info
  struct sockaddr_storage their_addr;

  gettimeofday(&curtime, NULL);
  
  // communicate with server
  socklen_t addr_len;
  addr_len = sizeof their_addr;
  bytes_sent = sendto(sockfd, msg, strlen(msg), 0, p->ai_addr, p->ai_addrlen);
  bytes_received = recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *)&their_addr, &addr_len);
  printf("sent bytes:%d, recv %d\n", bytes_sent, bytes_received);
  
  //bytes_sent = send(sockfd, msg, strlen(msg), 0);
  //bytes_received = recv(sockfd, buffer, BUFSIZE, 0);
  gettimeofday(&echotime, NULL);
  return (float)(echotime.tv_sec-curtime.tv_sec) + \
    (((float)(echotime.tv_usec-curtime.tv_usec))/1000000.0);
}
