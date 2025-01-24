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

#define BUFSIZE 1024

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
    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }

    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) 
        error("ERROR opening socket\n");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    /*bzero function erases the data in the n (second argument) bytes of the memory starting at the location pointed to by the first argument*/
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    /*bcopy function copies n bytes from src to dst*/
    serveraddr.sin_port = htons(portno);
    //htons converts a 16 bit # from host byte order to netork byte order

    /* get a message from the user */
    bzero(buf, BUFSIZE);
    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    while (1) {
      display_menu();
      fgets(buf, BUFSIZE, stdin);

      buf[strcspn(buf, "\n")] = 0;
      
      if (strcmp(buf, "exit") == 0){
        //Will need to consider if data dropping over the network is an issue for this option. I don't beleive it will be.
        n = sendto(sockfd, buf, BUFSIZE, 0, &serveraddr, serverlen);
        n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
        printf("Server Response to Client Exit: %s\n", buf);
        break;

      } else if (strcmp(buf, "ls") == 0){
        //ls malfunctioning, ls will give all the results with one call and then no results with another, and switch between the two
        n = sendto(sockfd, buf, BUFSIZE, 0, &serveraddr, serverlen);
        
        if (n < 0){ 
          error("ERROR in sendto\n");
        }

        printf("\nThe LS results are: \n");
        // bzero(buf, BUFSIZE);

        while (1) {
          // bzero(buf, BUFSIZE);

          n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
          
          if (n < 0) { 
            error("ERROR in recvfrom\n");
          }

          if (strcmp(buf, "end") == 0) {
            printf("\n");
            break;
          }

          printf("%s", buf);
        }

      }else if (strncmp(buf, "get", 3 )== 0){
        printf("in get\n");
        position = 4;
        strncpy(filename, buf + position, strlen(buf) - position + 1);
        fp = fopen(filename, "w");
        // bzero(buf, BUFSIZE);
        n = sendto(sockfd, buf, BUFSIZE, 0, &serveraddr, serverlen);
        
        if (n < 0){ 
          error("ERROR in sendto\n");
        }

        while (1) {
          n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen); //remove confirmation
          
          if (n < 0){ 
            error("ERROR in recvfrom\n");
          }
          
          printf("Value Being Read from File: %s", buf);
          
          if (strcmp(buf, "end") == 0) {
            printf("\n");
            break;
          }
          
          fputs(buf, fp);

        }
        
        fclose(fp);

      }else if (strncmp(buf, "put", 3) == 0){
        printf("in put\n");

      }else if (strncmp(buf, "delete", 6) == 0){
        printf("in delete\n");

        n = sendto(sockfd, buf, BUFSIZE, 0, &serveraddr, serverlen);
        if (n < 0){ 
          error("ERROR in sendto\n");
        }
        
        n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen); //remove confirmation
        if (n < 0){ 
          error("ERROR in recvfrom\n");
        }

          printf("%s", buf);
      }

    }
    

    return 0;
}


// gcc udp_client.c -o client -g

// gdb --args ./client 127.0.0.1 5001