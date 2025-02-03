/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFSIZE 1500
#define FILENAMESIZE 40


/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int display_menu(void){
    printf("Please type in one of following options as shown below: \n"
      "get [file_name]\n"
      "put [file_name]\n"
      "delete [file_name]\n"
      "ls\n"
      "exit\n\n"
      "Your input: ");
      return 0;
}

int exit_client_program(char buf[BUFSIZE], int sockfd, int serverlen, struct sockaddr_in serveraddr, int n){
  n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen);
  n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
  printf("Server Response to Client Exit: %s\n", buf);
  return 0;
}

int list_server_directory_content(char buf[BUFSIZE], int sockfd, int serverlen, struct sockaddr_in serveraddr){
  int n;
  
  n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen);
  if (n < 0) error("ERROR in sendto\n");

  printf("\nThe LS results are: \n");
  bzero(buf, BUFSIZE);

  while (1) {
    n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
    if (n < 0) error("ERROR in recvfrom\n");

    if (strcmp(buf, "end") == 0) {
      printf("\n");
      bzero(buf, BUFSIZE);
      break;
    }

    printf("%s", buf);
    bzero(buf, BUFSIZE);
  }
  
  return 0;
}


int get_file_from_server_directory(char buf[BUFSIZE], int sockfd, int serverlen, struct sockaddr_in serveraddr, int n, int position, char filename[FILENAMESIZE], FILE *fp){
  char client_ack[20];
  int client_ack_bytes_sent;
  position = 4;
  strncpy(filename, buf + position, strlen(buf) - position + 1);

  n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen); 
  if (n < 0) error("ERROR in sendto\n");
  n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen); //remove confirmation
  if (n < 0) error("ERROR in recvfrom\n");
  
  if (strcmp(buf, "error opening file on the server\n") == 0){
    bzero(buf, BUFSIZE);
    bzero(filename, FILENAMESIZE);
    fprintf(stderr, "%s", "File could not be opened on the server\n");
    return 1;
  }

  fp = fopen(filename, "w");
  while (1) {
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen); //remove confirmation
    if (n < 0) error("ERROR in recvfrom\n");

    if (strcmp(buf, "end") == 0) {
      printf("\n");
      break;
    }

    fwrite(buf, 1, n, fp);
    sprintf(client_ack, "Received: %d", n);
    client_ack_bytes_sent = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen);
  }

  bzero(buf, BUFSIZE);
  bzero(filename, FILENAMESIZE);
  
  if (fclose(fp) !=0){ 
    fprintf(stderr, "%s", "Error closing the file\n");
  }else {
    printf("The file transfer was complete!\n");
  }

  return 0;
}

int put_file_in_server_directory(char buf[BUFSIZE], int sockfd, int serverlen, struct sockaddr_in serveraddr, int n, int position, char filename[FILENAMESIZE], FILE *fp){
  int total_bytes_sent_during_one_read; 
  int bytes_remaining;
  int bytes_read;
  int bytes_from_server_ack;
  char server_ack[20];  
  
  position = 4;
  strncpy(filename, buf + position, strlen(buf) - position + 1);
  fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "%s", "Error opening the file\n");    
    bzero(buf, BUFSIZE);
    bzero(filename, FILENAMESIZE);
    return 1;
  }

  n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen); //Telling Server it is going to be put
  if (n < 0) error("ERROR sending data in put functionality\n");

  bzero(buf, BUFSIZE);

  //CITATION: Influenced by Beej sendall example
  while (1){
    bytes_read = fread(buf, 1, BUFSIZE, fp);
    if (bytes_read < 1) break;
    total_bytes_sent_during_one_read = 0;
    bytes_remaining = bytes_read;
    
    while (total_bytes_sent_during_one_read < bytes_read) {
      n = sendto(sockfd, buf+total_bytes_sent_during_one_read, bytes_remaining, 0, (struct sockaddr *) &serveraddr, serverlen);
      if (n < 0) {
        error("ERROR in put sendto\n");
        break;
      } 

      total_bytes_sent_during_one_read = total_bytes_sent_during_one_read + n;
      bytes_remaining = bytes_remaining - n;
      bytes_from_server_ack = recvfrom(sockfd, server_ack, 20, 0, (struct sockaddr *) &serveraddr, &serverlen); //remove confirmation
      
      bzero(server_ack, 20);

    }

    bzero(buf, BUFSIZE);
  } 

  strcpy(buf, "end");
  
  n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen);
  if (n < 0) error("ERROR in sendto\n");

  bzero(buf, BUFSIZE);
  bzero(filename, FILENAMESIZE);
  if (fclose(fp) !=0){ 
    fprintf(stderr, "%s", "Error closing the file\n");
  }else {
    printf("The file transfer was complete!\n");
  }

  return 0;
}

int delete_file_in_server_directory(char buf[BUFSIZE], int sockfd, int serverlen, struct sockaddr_in serveraddr, int n){
  n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen);
  if (n < 0) error("ERROR in sending the file name to be deleted to the server\n");
  
  n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen); //remove confirmation
  if (n < 0) error("ERROR in receiving acknowledgement that the file was deleted\n");

  printf("Message from Server: %s\n", buf);
  return 0;
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    int position;
    char filename[FILENAMESIZE];
    int file_status;
    FILE *fp; 
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }

    hostname = argv[1];
    portno = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sockfd < 0) error("ERROR opening socket\n");
    //CITATION: https://www.youtube.com/watch?v=bA1VMjShQUk
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    
    server = gethostbyname(hostname);// gethostbyname: get the server's DNS entry 
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    
    serveraddr.sin_port = htons(portno);
    
    serverlen = sizeof(serveraddr);//htons converts a 16 bit # from host byte order to netork byte order

    bzero(buf, BUFSIZE);

    //Displaying menu and implementing user choice
    while (1) {
      display_menu();
      fgets(buf, BUFSIZE, stdin);
      buf[strcspn(buf, "\n")] = 0;
      
      if (strcmp(buf, "exit") == 0){
        exit_client_program(buf, sockfd, serverlen, serveraddr, n);
        break;
      } else if (strcmp(buf, "ls") == 0){
        list_server_directory_content(buf, sockfd, serverlen, serveraddr);
      }else if (strncmp(buf, "get", 3 )== 0){
        get_file_from_server_directory(buf, sockfd, serverlen, serveraddr, n, position, filename, fp);
      }else if (strncmp(buf, "put", 3) == 0){
        put_file_in_server_directory(buf, sockfd, serverlen, serveraddr, n, position, filename, fp);
      }else if (strncmp(buf, "delete", 6) == 0){
        delete_file_in_server_directory(buf, sockfd, serverlen, serveraddr, n);
      }else {
        printf("\nYou have not entered a valid option. Please try again with the only a space between one of the five options and the file name. \nPlease ensure your file name does not have whitespaces in it. Thank you!\n");
      }
    
    }

    return 0;
}