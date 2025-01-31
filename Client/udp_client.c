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

//NOTE: NEED TO BZERO OUT MORE DURING PUT, GET, DELETE due to writing into text files

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


int get_file_from_server_directory(char buf[BUFSIZE], int sockfd, int serverlen, struct sockaddr_in serveraddr, int n, int position, char filename[40], FILE *fp){
  char client_ack[20];
  int client_ack_bytes_sent;
  position = 4;
  strncpy(filename, buf + position, strlen(buf) - position + 1);
  fp = fopen(filename, "w");

  
  n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen);
  if (n < 0) error("ERROR in sendto\n");

  bzero(buf, BUFSIZE);

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
  
  fclose(fp);
  return 0;
}

int put_file_in_server_directory(char buf[BUFSIZE], int sockfd, int serverlen, struct sockaddr_in serveraddr, int n, int position, char filename[40], FILE *fp){
        position = 4;
        
        strncpy(filename, buf + position, strlen(buf) - position + 1);
        fp = fopen(filename, "r");
        n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen);
        
        bzero(buf, BUFSIZE);

        while (fgets(buf, BUFSIZE, fp)){          
          n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen);
          if (n < 0) error("ERROR in sendto\n");          
          bzero(buf, BUFSIZE);
        } 

        fclose(fp);
        strcpy(buf, "end");
        
        n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen);
        if (n < 0) error("ERROR in sendto\n");

        bzero(buf, BUFSIZE);

  return 0;
}

int delete_file_in_server_directory(char buf[BUFSIZE], int sockfd, int serverlen, struct sockaddr_in serveraddr, int n){
  n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, serverlen);
  if (n < 0) error("ERROR in sendto\n");
  
  n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen); //remove confirmation
  if (n < 0) error("ERROR in recvfrom\n");

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
    char filename[40];
    int file_status;
    FILE *fp; 

    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }

    hostname = argv[1];
    portno = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sockfd < 0) error("ERROR opening socket\n");

    
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
      }
    
    }

    return 0;
}