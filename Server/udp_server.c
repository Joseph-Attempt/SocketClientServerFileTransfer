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

#define BUFSIZE 1500
#define FILENAMESIZE 40

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int exit_client_program(char buf[BUFSIZE], int sockfd, int clientlen, struct sockaddr_in clientaddr, int n){

  strcpy(buf, "Goodbye Client!\n");
  n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);

  if (n < 0) {
    error("ERROR in sendto\n");
  }
  
  return 0;
}

int list_server_directory_content(char buf[BUFSIZE], int sockfd, int clientlen, struct sockaddr_in clientaddr, int n, FILE *fp){
  //CITATION popen: https://stackoverflow.com/questions/12005902/c-program-linux-get-command-line-output-to-a-variable-and-filter-data 
  fp = popen("ls", "r");
  
  if(!fp) {
      fprintf(stderr, "Error attempting to use ls.\n");
      return 1;
  }

  bzero(buf, BUFSIZE);

  while (fgets(buf, BUFSIZE, fp) != NULL) {
      printf("looking at service side buf when ls: %s", buf);
      n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
      printf("Bytes Sent over: %d\n",n);
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


  if (pclose(fp) == -1) {
    fprintf(stderr," Error!\n");
    return 1;
  }


  return 0;
}


int get_file_in_server_directory(char buf[], int sockfd, int clientlen, struct sockaddr_in clientaddr, int n, int position, int file_status, char filename[FILENAMESIZE], FILE *fp){
  int total_bytes_sent_during_one_read; 
  int bytes_remaining;
  int bytes_read;
  int bytes_from_client_ack;
  char client_ack[20];
  
  position = 4;
  strncpy(filename, buf + position, strlen(buf) - position + 1);
  fp = fopen(filename, "r");
  bzero(buf, BUFSIZE);

  while (1){
    bytes_read = fread(buf, 1, BUFSIZE, fp);
    if (bytes_read < 1) {
      break;
    }

    //CITATION: Influenced by Beej sendall example
    total_bytes_sent_during_one_read = 0;
    bytes_remaining = bytes_read;
    
    while (total_bytes_sent_during_one_read < bytes_read) {
      n = sendto(sockfd, buf+total_bytes_sent_during_one_read, bytes_remaining, 0, (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0) {
        error("ERROR in sendto\n");
        break;
      } 

      total_bytes_sent_during_one_read = total_bytes_sent_during_one_read + n;
      bytes_remaining = bytes_remaining - n;
      printf("n: %d, bytes_remaning: %d, total bytes over one read: %d, bytes_read: %d\n", n, bytes_remaining, total_bytes_sent_during_one_read, bytes_read);
      
      bytes_from_client_ack = recvfrom(sockfd, client_ack, 20, 0, (struct sockaddr *) &clientaddr, &clientlen); //remove confirmation
      
      bzero(client_ack, 20);

    }

    bzero(buf, BUFSIZE);
  } 
  
  fclose(fp);
  strcpy(buf, "end");
  
  n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
  if (n < 0) error("ERROR in sendto\n");
  
  bzero(buf, BUFSIZE);
  bzero(filename, FILENAMESIZE);

  return 0;
}

int put_file_in_server_directory(char buf[], int sockfd, int clientlen, struct sockaddr_in clientaddr, int n, int position, int file_status, char filename[FILENAMESIZE], FILE *fp){
  char server_ack[20];
  int server_ack_bytes_sent;
  position = 4;
  strncpy(filename, buf + position, strlen(buf) - position + 1);
  fp = fopen(filename, "w"); //need to error check fp

  while (1) {
    bzero(buf, BUFSIZE);

    n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen); //remove confirmation
    if (n < 0) error("ERROR in recvfrom\n");
    
    if (strcmp(buf, "end") == 0) {
      printf("\n");
      break;
    }

    fwrite(buf, 1, n, fp);
    sprintf(server_ack, "Received: %d", n);

    server_ack_bytes_sent = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
    if (server_ack_bytes_sent < 0) error("ERROR in sending server ack\n");
  }

  bzero(buf, BUFSIZE);
  bzero(filename, FILENAMESIZE);
  fclose(fp);
  return 0;
}

int delete_file_in_server_directory(char buf[], int sockfd, int clientlen, struct sockaddr_in clientaddr, int n, int position, int file_status, char filename[FILENAMESIZE]){
  position = 7;
  strncpy(filename, buf + position, strlen(buf) - position + 1);
  printf("filename: %s\n", filename);
  file_status = remove(filename);
  bzero(buf, BUFSIZE);

  
  if (file_status !=0) {
    strcpy(buf, "Failed to remove the file ");
    printf("Failed to remove the file %s\n", filename);
  }else {
    strcpy(buf, "Failed to remove the file ");
    printf("Succeeded in removing file %s\n", filename);
  }

  //I should send a message back saying it has been deleted
  n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
  if (n < 0) error("ERROR in sendto\n");
  
  bzero(buf, BUFSIZE);
  bzero(filename, FILENAMESIZE);
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
  char filename[FILENAMESIZE];
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
  if (sockfd < 0) error("ERROR opening socket\n");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

   //build the server's Internet address
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  //bind: associate the parent socket with a port 
  if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) error("ERROR on binding\n");

  clientlen = sizeof(clientaddr);

  while (1) {
    bzero(buf, BUFSIZE);
    
    n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0) error("ERROR in recvfrom\n");


 
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);// gethostbyaddr: determine who sent the datagram
    if (hostp == NULL) error("ERROR on gethostbyadd\n");

    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL) error("ERROR on inet_ntoa\n");

    if (strcmp(buf, "exit") == 0){
      exit_client_program(buf, sockfd, clientlen, clientaddr, n);
    } else if (strcmp(buf, "ls") == 0){
      list_server_directory_content(buf, sockfd, clientlen, clientaddr, n, fp);
    }else if (strncmp(buf, "get", 3 )== 0){
      get_file_in_server_directory(buf, sockfd, clientlen,  clientaddr, n, position, file_status, filename,fp);
    }else if (strncmp(buf, "put", 3) == 0){
      put_file_in_server_directory(buf, sockfd, clientlen, clientaddr, n, position, file_status, filename, fp);
    }else if (strncmp(buf, "delete", 6) == 0){
      delete_file_in_server_directory(buf, sockfd, clientlen, clientaddr,  n, position, file_status,  filename);
    }
    
    printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
    printf("server received %ld/%d bytes: %s\n", strlen(buf), n, buf);
    

  }
}
