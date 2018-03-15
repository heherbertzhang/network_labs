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

typedef struct node{
  int sockfd;
  char username[100];
  struct node* next;
} node;

node* findclientbyname(char* username, node* root){
  node* client = root->next;
  while(client != NULL){
    if(strcmp(client->username, username) == 0){
      return client;
    }
    client = client->next;
  }
  return NULL;
}
node* findclientbyfd(int sockfd, node* root){
  node* client = root->next;
  while(client != NULL){
    if(client->sockfd == sockfd){
      return client;
    }
    client = client->next;
  }
  return NULL;
}
void removeNode(node* root, int fd, node** lastNode){
  node* prev = root;
  node* cur = root->next;
  while(cur!= NULL){
    if(cur->sockfd == fd){
      prev->next = cur->next;
      if(cur == *lastNode){
        *lastNode = prev;
      }
      printf("freeing %s\n", cur->username);
      free(cur);
      return;
    }
    prev = cur;
    cur = cur->next;
  }
}
int broadcast(node* clientroot, char* msg){
  node* client = clientroot->next;
  while(client != NULL){
    send(client->sockfd, msg, BUFFERSIZE, 0);
    client = client->next;
  }
  return 0;
}

int list(node* clientroot, int fd){
  char buffer[BUFFERSIZE];
  node* client = clientroot->next;
  printf("%s %d\n", "printing list for", fd);
  while(client != NULL){
    //printf("%s %d\n", "printing list for", fd);
    sprintf(buffer, "list client: %s with fd %d\n", client->username, client->sockfd);
    send(fd, buffer, BUFFERSIZE, 0);
    client = client->next;
  }
  return 0;
}
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

int main(int argc, char** argv) {
  if(argc < 2){printf("Not enough args.\n"); return 0;}
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  struct sockaddr_in server_addr;
  struct addrinfo hints, *res;
  int ssockfd, new_fd; // server socket and new client socket
  node clientroot;
  clientroot.sockfd = 0;
  clientroot.next = NULL;
  node* lastClient = &clientroot;

  // load up addr struct
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // fill in my ip for me
  getaddrinfo(NULL, argv[1], &hints, &res);

  ssockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  bind(ssockfd, res->ai_addr, res->ai_addrlen);
  listen(ssockfd, 10); // 10 pending connections queue will hold

  // now accept an incoming connections
  fd_set readfds, readfds_;
  int fdmax = ssockfd;
  FD_SET(ssockfd, &readfds);
  FD_SET(0, &readfds); // add terminal to the readfds list
  addr_size = sizeof(their_addr);
  char buffer[BUFFERSIZE];

  printf("%s\n", "Server is waiting for client connections");
  while(1){
    int read = 1;
    readfds_ = readfds; // copy to temp
    if (select(fdmax+1, &readfds_, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }
    int i = 0;
    for(i = 0; i <= fdmax; i++){
      if (!FD_ISSET(i, &readfds_)){continue;}
      if(i==ssockfd) { // server receive new client
        new_fd = accept(ssockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) {
          printf("wrong connection!\n");
          continue;
        }
        //get username
        bzero(buffer, BUFFERSIZE);
        recv(new_fd, buffer, BUFFERSIZE, 0);
        if(findclientbyname(buffer, &clientroot) != NULL){
          printf("Already exist %s\n", buffer);
          close(new_fd);
          continue;
        }
        printf("New Client connection!: %s, with fd %d\n", buffer, new_fd);
        // succussfully get new client
        FD_SET(new_fd, &readfds);
        node* newClient = malloc(sizeof(node));
        newClient->sockfd = new_fd;
        strcpy(newClient->username, buffer);
        newClient->next = NULL;
        lastClient->next = newClient;
        lastClient = newClient;
        if(new_fd > fdmax) { fdmax = new_fd; }
        strcat(buffer, " just connected! Welcome!\n");
        broadcast(&clientroot, buffer); // broadcast new user to all
      } else {
        //for client message
        bzero(buffer, BUFFERSIZE);
        int read_bytes;
        read_bytes = recv(i, buffer, BUFFERSIZE, 0);
        if(read_bytes <= 0){
          printf("received bytes < 0!\n");
          FD_CLR(i, &readfds);
          close(i);
          //removeNode(&clientroot, i, &lastClient);
          continue;
        }
        printf("Server got message: %s from %d\n", buffer, i);

        //handle client message
        if(strncmp(buffer, "broadcast", 9) == 0){
          //receive message
          char msg[BUFFERSIZE];
          bzero(msg, BUFFERSIZE);
          sprintf(msg, "New broadcast message from %s: ", findclientbyfd(i, &clientroot)->username);
          strcat(msg, buffer+10);
          broadcast(&clientroot, msg);
        }
        else if(strcmp(buffer, "exit\n")==0){
          FD_CLR(i, &readfds);
          removeNode(&clientroot, i, &lastClient);
          close(i);
        }
        else if(strcmp(buffer, "list\n")==0){
          list(&clientroot, i);
        }
        else {
          char username[100];
          int loc = read1word(buffer, username);
          if(loc > strlen(buffer)){ continue;}
          // find user
          node* client = findclientbyname(username, &clientroot);
          if(client == NULL) { continue; }
          char msg[BUFFERSIZE];
          bzero(msg, BUFFERSIZE);
          sprintf(msg, "New private message from %s: ", findclientbyfd(i, &clientroot)->username);
          strcat(msg, buffer+loc);
          send(client->sockfd, msg, BUFFERSIZE, 0);
        }
      }

    }

  }

}
