#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFERSIZE 256

int read1word(char*in, char*out){
  int i = 0;
  for(i = 0; i < BUFFERSIZE; i++){
    if(in[i]!=' '&&in[i]!='\n'&&in[i]!= '\0'){
      out[i]=in[i];
    }else{
      out[i]='\0';
      return i+1;
    }
  }
  return i+1;
}

int main(int argc, char** argv){
  int sockfd;
  struct addrinfo hints, *res;

  // first, load up address structs with getaddrinfo():
  printf("Starting %s\n", argv[0]);
  if(argc < 4){
    printf("Not enough args\n");
  }
  char* serveraddr = argv[1];
  char* serverport = argv[2];
  char* username = argv[3];
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM; // TCP
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
  getaddrinfo(serveraddr, serverport, &hints, &res);
  // make a socket
  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  // connect!
  connect(sockfd, res->ai_addr, res->ai_addrlen);
  send(sockfd, username, BUFFERSIZE, 0);

  int byte_sent, bytes_recv;
  size_t bytes_read;
  char buffer[BUFFERSIZE];
  bzero(buffer, BUFFERSIZE);

  fd_set readfds, readfds_;
  int maxfd = sockfd;
  FD_SET(0, &readfds);
  FD_SET(sockfd, &readfds);

  while(1){
    readfds_ = readfds;
    if(select(maxfd+1, &readfds_, NULL, NULL, NULL)==-1){
      perror("select");
      exit(4);
    }

    if(FD_ISSET(sockfd, &readfds_)) {
      // printf("%s\n", "received msg");
      bzero(buffer, BUFFERSIZE);
      bytes_recv = recv(sockfd, buffer, BUFFERSIZE, 0);
      if (bytes_recv <= 0) {
        close(sockfd);
        return 0;
      } else{
        printf("%s", buffer);
      }
    } else if( FD_ISSET(0, &readfds_)) { // read from terminal
      char * line;
      getline(&line, &bytes_read, stdin);
      // printf("typed: %s\n", line);
      send(sockfd, line, BUFFERSIZE, 0);
      // if(strncmp(line, "broadcast", 9) == 0){
      //   send(sockfd, "broadcast", BUFFERSIZE, 0);
      //   send(sockfd, line+10, BUFFERSIZE, 0);
      // }
      // else if(strncmp(line, "list", 4) == 0){
      //   send(sockfd, "list", BUFFERSIZE, 0);
      // }
      // else if(strncmp(line, "exit", 4) == 0){
      //   send(sockfd, "exit", BUFFERSIZE, 0);
      // }
      // else{
      //   send(sockfd, "private", BUFFERSIZE, 0);
      //   char username[BUFFERSIZE];
      //   int loc = read1word(buffer, username);
      //   if(loc > strlen(buffer)){continue;}
      //   send(sockfd, username, BUFFERSIZE, 0);
      //   send(sockfd, buffer+loc, BUFFERSIZE, 0);
      // }
    }
  }
}
