/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int exit_client_program(void){
  return 0;
}

int list_server_directory_content(void){
  return 0;
}


int get_file_in_server_directory(void){
  return 0;
}

int put_file_in_server_directory(void){
  return 0;
}

int delete_file_in_server_directory(void){
  return 0;
}


int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  FILE *server_response;
  char filename[40];
  int position;
  int file_status;
  FILE *fp; 
  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sockfd < 0) 
    error("ERROR opening socket\n");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
    error("ERROR on binding\n");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);

  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
  
    if (n < 0)
      error("ERROR in recvfrom\n");


    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);

    if (hostp == NULL)
      error("ERROR on gethostbyadd\n");

    hostaddrp = inet_ntoa(clientaddr.sin_addr);

    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");

    if (strcmp(buf, "exit") == 0){

      strcpy(buf, "Goodbye Client!\n");
      n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);

      if (n < 0) {
        error("ERROR in sendto\n");
      }
      
      printf("n: %d, sizeof(buf): %ld\n", n, sizeof(buf));

    } else if (strcmp(buf, "ls") == 0){

      //CITATION popen: https://stackoverflow.com/questions/12005902/c-program-linux-get-command-line-output-to-a-variable-and-filter-data 
      server_response = popen("ls", "r");
      
      if(!server_response) {
          fprintf(stderr, "Error attempting to use ls.\n");
          return 1;
      }

      bzero(buf, BUFSIZE);

      while (fgets(buf, sizeof(buf), server_response) != NULL) {
          // Process the line stored in 'buffer'
          printf("looking at service side buf when ls: %s", buf);
          n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
          
          if (n < 0) {
            error("ERROR in sendto\n");
          } 
          bzero(buf, BUFSIZE);
      }
      
      strcpy(buf, "end");
      n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0) {
        error("ERROR in sendto\n");
      } 
      bzero(buf, BUFSIZE);


      if (pclose(server_response) == -1) {
          fprintf(stderr," Error!\n");
          return 1;
      }

    }else if (strncmp(buf, "get", 3 )== 0){
      position = 4;
      strncpy(filename, buf + position, strlen(buf) - position + 1);
      fp = fopen(filename, "r");
      bzero(buf, BUFSIZE);

      while (fgets(buf, BUFSIZE, fp)){
        printf("Reading Value from file: %s", buf);
        n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) {
          error("ERROR in sendto\n");
        } 
        bzero(buf, BUFSIZE);

      //unsure if I should bzero it here. I need to know if fgets overwrites the rest of the char array if it doesn't fill it up all the way
      } 

      fclose(fp);
      strcpy(buf, "end");
      
      n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0) {
        error("ERROR in sendto\n");
      } 
      bzero(buf, BUFSIZE);


    }else if (strncmp(buf, "put", 3) == 0){

      printf("in put\n");


    }else if (strncmp(buf, "delete", 6) == 0){
      position = 7;
      printf("buf = %s\n", buf);
      strncpy(filename, buf + position, strlen(buf) - position + 1);
      printf("filename: %s\n", filename);
      file_status = remove(filename);

      if (file_status !=0) {
        printf("Failed to remove file %s\n", filename);
      }else {
        printf("Succeeded in removing file %s\n", filename);
      }
      // n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);

      if (n < 0) {
        error("ERROR in sendto\n");
      }
      
      printf("n: %d, sizeof(buf): %ld\n", n, sizeof(buf));
    }
    
    printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
    printf("server received %ld/%d bytes: %s\n", strlen(buf), n, buf);
    
    /* 
     * sendto: echo the input back to the client 
     */
    n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
    
    if (n < 0) 
      error("ERROR in sendto\n");

  }
}
