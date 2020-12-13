#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 256

void sendToServ(int sock, char mess[], struct sockaddr_in servaddr, int sendBytes) {
  socklen_t len = sizeof(servaddr);
  sendBytes = sendto(sock, mess, strlen(mess), 0, (struct sockaddr *) &servaddr, len);

  if(sendBytes < 0){
    perror("Error: ");
  }

  return;
}

void recvFromServ(int sock, char mess[], struct sockaddr_in servaddr, int rcvBytes, socklen_t len) {
  rcvBytes = recvfrom(sock, mess, BUFF_SIZE, 0, (struct sockaddr *) &servaddr, &len);
  if(rcvBytes < 0){
    perror("Error: ");
  }
  mess[rcvBytes] = '\0';
  return;
}

/* 
00 - LoginAndExitMenu
01 - Login - username
02 - Login - password
03 - MenuChat
04 - Chat 1-1
*/
char* EncodeMessage (char mess[], int code) {
  char *EncodedMess;
  EncodedMess = (char *)malloc(BUFF_SIZE);

  if (code == 0) {
    *(EncodedMess) = '0';
    *(EncodedMess + 1) = '0';
  } 
  else if (code == 1) {
    *(EncodedMess) = '0';
    *(EncodedMess + 1) = '1';
  }
  else if (code == 2) {
    *(EncodedMess) = '0';
    *(EncodedMess + 1) = '2';
  }
  else if (code == 3) {
    *(EncodedMess) = '0';
    *(EncodedMess + 1) = '3';
  }
  else if (code == 4) {
    *(EncodedMess) = '0';
    *(EncodedMess + 1) = '4';
  }

  *(EncodedMess + 2) = ' ';
  for (int i=0; i<strlen(mess); ++i) {
    *(EncodedMess + 3 + i) = mess[i];
  }
  *(EncodedMess + 3 + strlen(mess) + 1) = '\0';

  return EncodedMess;
}

char* DecodeMessage (char mess[]) {
  char *code;
  code = (char *)malloc(BUFF_SIZE);
  
  *(code) = mess[0];
  *(code + 1) = mess[1];
  *(code + 2) = '\0';

  return code;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Wrong param.\n");
    return 0;
  }
  else {
    int sockfd;
    struct sockaddr_in servaddr;
    char sendline[BUFF_SIZE+1], recvline[BUFF_SIZE+1], sendMess[BUFF_SIZE+1];;
    char *text = calloc(1, 1);

    int nBit;

    // Create a socket from client
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("Problem in creating the socket..");
      exit(2);
    }

    // Creation of the remote server socket information structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    //servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    // Connect the client to the server socket
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
      perror("Problem in connecting to server...");
      exit(3);
    }

    // printf("Your account: ");

    while (1) {
      while ((nBit = recv(sockfd, recvline, sizeof(recvline), 0)) != 0) {
        recvline[nBit] = '\0';
        printf("%s", recvline);
        if (strcmp(recvline, "01 Goodbye bro..") == 0) {
          printf("Disconnect to server...\n");
          // close(sockfd);
        }

        if (fgets(sendline, BUFF_SIZE, stdin) != NULL) {
          text = realloc(text, strlen(text) + 1 + strlen(sendline));
          if (!text) {
            perror("No string");
            exit(5);
          }
          strcat(text, sendline);
          // Reply to LoginAndExitMenu
          if (strcmp(DecodeMessage(recvline), "00") == 0) {
            strcpy(sendMess, EncodeMessage(sendline, 0));
          }
          // Reply username
          else if (strcmp(DecodeMessage(recvline), "01") == 0) {
            strcpy(sendMess, EncodeMessage(sendline, 1));
          }
          // Reply password
          else if (strcmp(DecodeMessage(recvline), "02") == 0) {
            strcpy(sendMess, EncodeMessage(sendline, 2));
          }
          // Reply menu-chat
          else if (strcmp(DecodeMessage(recvline), "03") == 0) {
            strcpy(sendMess, EncodeMessage(sendline, 3));
          }
          // Reply chat 1-1
          else if (strcmp(DecodeMessage(recvline), "04") == 0) {
            strcpy(sendMess, EncodeMessage(sendline, 4));
          }
          send(sockfd, sendMess, strlen(sendMess), 0);
        }
      }
    }

    return 0;
  }
  
}