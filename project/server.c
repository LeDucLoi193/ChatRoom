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
#include <sys/wait.h>

#define BUFF_SIZE 512
#define MAX 25
#define MAX_CLIENT 10
#define TRUE 1

// User define
typedef struct User {
  char username[MAX];
  char password[MAX];
  int state;
  struct User *next;
} User;

// Client define with name and socket
typedef struct clients {
  char clientName[MAX];   
  int sock;
  int receiverSock;
  int menu_status;  // luu menu nguoi dung dang chon, khoi tao la 0
  int haveWaitingMessage; // 0 -> no, 1 -> yes
  struct clients *next;
} clients;

clients *head, *curClient;

User *root, *cur;
pid_t test;

User* makeNewNode(User userInput) {
  User* newUser = (User*) malloc (sizeof(User));
  strcpy(newUser->username, userInput.username);
  strcpy(newUser->password, userInput.password);
  newUser->state = userInput.state;
  newUser->next = NULL;

  return newUser;
}

void insertNode(User userInput) {
  User* new = makeNewNode(userInput);
  if (root == NULL) {
    root = new;
    cur = root;
  } else {
    new->next = cur->next;
    cur->next = new;
    cur = new;
  }
}

User* findNode(char username[]) {
  User* node = root;
  if (node == NULL)
    return NULL;
  else {
    do {
      if (strcmp(node->username, username) == 0) {
        return node;
      }
      node = node->next;
    } while (node != NULL);
  }

  return NULL;
}

void printNode() {
  User *node = root;
  do {
    printf("%s\t%s\t%d \n", node->username, node->password, node->state);
    node = node->next;
  } while(node != NULL);
}

void freeList() {
  User* node = root ;
  if (root == NULL) {
    return;
  }

  while (node != NULL){
    root = root->next;
    free(node);
    node = root;
  }
}

void freeClientNode (int socket) {
  clients *node = head;
  if (head == NULL)
    return;
  else if (head->sock == socket) {
    head = node->next;
    free(node);
    curClient = head;
    return;
  }
  else if (curClient->sock == socket) {
    clients *temp = head;
    while (temp != NULL) {
      if (temp->next == curClient)
        break;
      else
        temp = temp->next;
    }
    temp->next = curClient->next;
    free(curClient);
    curClient = temp->next;
  }
  else {
    while (node->next != NULL) {
      if (node->next->sock == socket) {
          clients *curClientNode = node->next;
          node->next = curClientNode->next;
          free(curClientNode);
          return;
      }
      node = node->next;
    }
  }
}


void AddClientPort(int new_sock){
  clients *temp = head;
  puts("Connection oke..");
  
  if(head == NULL) {
    temp = (struct clients *)malloc(sizeof(struct clients));
    // strcpy(temp->clientName, clientName);
    strcpy(temp->clientName, "");
    temp->receiverSock = 0;
    temp->sock = new_sock;
    temp->menu_status = 0;
    temp->haveWaitingMessage = 0;
    temp->next = NULL;
    head = temp;
    curClient = head;
  }
  else {
    clients *new = (struct clients *)malloc(sizeof(struct clients));
    strcpy(new->clientName, "");
    new->sock = new_sock;
    new->receiverSock = 0;
    new->menu_status = 0;
    new->haveWaitingMessage = 0;
    new->next = NULL;
    curClient->next = new;
    curClient = new;
  }
}

int AddClientName(int socket, char clientName[]) {
  clients *temp = head;
  while (temp != NULL) {
    if (temp->sock == socket && strcmp(temp->clientName, "") == 0) {
      strcpy(temp->clientName, clientName);
      return 1;
    }

    temp = temp->next;
  }

  return 0;
}

void AddReceiverSocket(int socket, int receiverSock) {
  // socket: sender's socket
  clients *temp = head;

  // save receiver socket
  temp = head;
  while (temp != NULL) {
    if (temp->sock == socket) {
      temp->receiverSock = receiverSock;
      return;
    }

    temp = temp->next;
  }
}

void AddMenuStatus (int socket, char choice) {
  clients *temp = head;

  // save receiver socket
  temp = head;
  while (temp != NULL) {
    if (temp->sock == socket) {
      temp->menu_status = choice - '0';
      return;
    }

    temp = temp->next;
  }
}

clients* findOnlineUser(char username[]) {
  clients* node = head;
  if (node == NULL)
    return NULL;
  else {
    do {
      if (strcmp(node->clientName, username) == 0) {
        return node;
      }
      node = node->next;
    } while (node != NULL);
  }

  return NULL;
}

void readFileUser(char fileName[], FILE *f) {
  f = fopen(fileName, "r");
  User readUser;

  freeList();

  if (f == NULL)
    printf("Cannot read input file!\n");
  else {
    while(!feof(f)) {
      fscanf(f, "%s\t%s\t%d\n" ,readUser.username, readUser.password, &readUser.state);
      insertNode(readUser);
    }
  }

  fclose(f);

  return;
}

void writeFileUser(char fileName[], FILE *f, User writeUser) {
  f = fopen(fileName, "a");

  if (f == NULL)
    printf("Cannot read input file!\n");
  else {
    fputs("\n", f);
    fputs(writeUser.username, f);
    fputs("\t", f);
    fputs(writeUser.password, f);
    fputs("\t", f);
    fprintf(f, "%d", writeUser.state);
  }

  fclose(f);

  return;
}

void writeAllToFileUser(char fileName[], FILE *f) {
  User *node = root;
  remove(fileName);
  f = fopen(fileName, "w");

  if (f == NULL)
    printf("Cannot read input file!\n");
  else {
    do {
      fputs(node->username, f);
      fputs("\t", f);
      fputs(node->password, f);
      fputs("\t", f);
      fprintf(f, "%d", node->state);
      if (node->next != NULL) {
        fputs("\n", f);
      }
      node = node->next;
    } while(node != NULL);
  }

  fclose(f);

  return;
}

void sig_chld (int signo) {
  pid_t pid;
  int stat;
  pid = waitpid(-1, &stat, WNOHANG);
  test = pid;
  printf("child %d terminated\n", pid);
}

char* CutString (char buff[]) {
  char *res;
  res = (char *)malloc(BUFF_SIZE);

  for (int i=3; i<strlen(buff); ++i) {
    *(res + i - 3) = buff[i];
  }
  *(res + strlen(buff) - 3 + 1) = '\0';

  return res;
}

/* 
00 - LoginAndExitMenu
01 - Login - username
02 - Login - password
03 - MenuChat
04 - Chat 1-1
41 - Chat 1-1 send and receive message
42 - Chat 1-1 - save waiting message
43 - Chat 1-1 - send waiting message
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
  else if (code == 41) {
    *(EncodedMess) = '4';
    *(EncodedMess + 1) = '1';
  }
  else if (code == 42) {
    *(EncodedMess) = '4';
    *(EncodedMess + 1) = '2';
  }
  else if (code == 43) {
    *(EncodedMess) = '4';
    *(EncodedMess + 1) = '3';
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

void SendLoginAndExitMenu (int socket) {
  // code: 00
  char mess[BUFF_SIZE];
  memset(mess, 0, sizeof(mess));

  sprintf(mess, "---Welcome---\n1. Login\n2. Exit\nYour choice: ");
  char encodeMenu[BUFF_SIZE];
  strcpy(encodeMenu, EncodeMessage(mess, 0));
  send(socket, encodeMenu, strlen(encodeMenu), 0);
}

void SendLoginMenu (int socket) {
  // Login successfully
  char mess[BUFF_SIZE];
  memset(mess, 0, sizeof(mess));

  sprintf(mess, "---Menu chat---\n1. Chat 1-1\n2. Chat group\n3. Exit\nYour choice: ");
  char encodeMenu[BUFF_SIZE];
  strcpy(encodeMenu, EncodeMessage(mess, 3));
  send(socket, encodeMenu, strlen(encodeMenu), 0);
}

void HandleLoginAndExitMenu (int socket, char mess[], fd_set readfds) {
  // code: 01
  char sendMess[BUFF_SIZE];

  if (mess[3] == '1') {
    // LoginMenu
    strcpy(sendMess, EncodeMessage("Your account: ", 1));
    send(socket, sendMess, strlen(sendMess), 0);
  }
  else if (mess[3] == '2') {
    // Exit
    strcpy(sendMess, EncodeMessage("Goodbye bro..", 1));
    send(socket, sendMess, strlen(sendMess), 0);
    close(socket);
    // FD_CLR(socket, &readfds);
  }
  else {
    // Failed
    strcpy(sendMess, EncodeMessage("Wrong choice, try again: ", 0));
    send(socket, sendMess, strlen(sendMess), 0);
  }
}

char* HandleLoginUsername (int socket, char mess[], char username[]) {
  // code: 01
  char sendMess[BUFF_SIZE];

  strcpy(username, CutString(mess));

  User *foundUser = findNode(username);
  if (foundUser == NULL) {
    strcpy(sendMess, EncodeMessage("Username does not exist. Try again: ", 1));
    send(socket, sendMess, strlen(sendMess), 0);

    return "";
  } 
  else {
    strcpy(sendMess, EncodeMessage("Enter password: ", 2));
    send(socket, sendMess, strlen(sendMess), 0);

    return username;
  }
}

void HandleLoginPassword (int socket, char username[], char mess[], char fileName[], FILE *f) {
  // code: 02
  char sendMess[BUFF_SIZE];
  char password[MAX];

  strcpy(password, CutString(mess));

  clients *testUser = findOnlineUser(username);
  if (testUser != NULL) {
    SendLoginMenu(socket);
  }
  else {
    User *foundUser = findNode(username);
    if (strcmp(foundUser->password, password) != 0) {
      strcpy(sendMess, EncodeMessage("Wrong password. Try again: ", 2));
      send(socket, sendMess, strlen(sendMess), 0);
    } 
    else {
      foundUser->state = 1;
      writeAllToFileUser(fileName, f);
      readFileUser(fileName, f);
      AddClientName(socket, username);
      SendLoginMenu(socket);
    }
  }
}

void HandleMenuChat (int socket, char mess[]) {
  // code: 03
  char sendMess[BUFF_SIZE];
  clients *node = head; // find username

  do {
    if (node->sock == socket) {
      break;
    }
    node = node->next;
  } while (node != NULL);

  if (mess[3] == '1') {
    AddMenuStatus(socket, mess[3]);
    if (node->haveWaitingMessage == 1) {
      // check if user have waiting message
      strcpy(sendMess, EncodeMessage("You have unread message.", 43));
      send(socket, sendMess, strlen(sendMess), 0);
    }
    else {
      strcpy(sendMess, EncodeMessage("Enter receiver: ", 4));
      send(socket, sendMess, strlen(sendMess), 0);
    }
  }
  else if (mess[3] == '2') {

  }
  else if (mess[3] == '3') {
    strcpy(sendMess, EncodeMessage("Goodbye bro..", 3));
    send(socket, sendMess, strlen(sendMess), 0);
    close(socket);
  }
}

// Only check sent message to exit or continue
int HandleChat (int sentSocket, char mess[]) {
  // code: 41
  char sendMess[BUFF_SIZE];

  clients* node = head;
  do {
    if (node->sock == sentSocket) {
      break;
    }
    node = node->next;
  } while (node != NULL);

  if (strcmp(CutString(mess), "exit") == 0 || strcmp(CutString(mess), "Exit") == 0) {
    AddReceiverSocket(sentSocket, 0);
    AddMenuStatus(sentSocket, '0');
    strcpy(sendMess, EncodeMessage("Exit...\n", 04));
    send(sentSocket, sendMess, strlen(sendMess), 0);
    // strcpy(sendMess, EncodeMessage("Your friend exits..Enter exit to out chat\n", 04));
    // send(node->receiverSock, sendMess, strlen(sendMess), 0);
    return 0;
  }
  else {
    strcpy(mess, CutString(mess));
    strcpy(sendMess, node->clientName);
    strcat(sendMess, ": ");
    strcat(sendMess, mess);
    strcat(sendMess, "\n");
    strcpy(sendMess, EncodeMessage(sendMess, 41));
    send(node->receiverSock, sendMess, strlen(sendMess), 0);
  }

  return 1;
}

// return receiver's port
int HandleSingleChat (int socket, char mess[]) {
  // code: 04
  char sendMess[BUFF_SIZE];
  char username[MAX];

  strcpy(username, CutString(mess));

  clients *foundUser = findOnlineUser(username);
  if (foundUser == NULL) {
    strcpy(sendMess, EncodeMessage("Username does not exist or login yet. Try again: ", 4));
    send(socket, sendMess, strlen(sendMess), 0);

    return -1;
  } 
  else if (foundUser->menu_status == 0) {
    // User is in another menu -> code: 42
    AddReceiverSocket(socket, foundUser->sock);

    char buff[BUFF_SIZE];
    memset(buff, 0, sizeof(buff));
    printf("aaa\n");
    sprintf(buff, "User does not choose menu yet or in another menu.\nEnter message to send: ");
    char encodeMenu[BUFF_SIZE];
    strcpy(encodeMenu, EncodeMessage(buff, 42));
    // strcpy(sendMess, EncodeMessage("User does not choose menu yet or in another menu.\n", 04));
    send(socket, encodeMenu, strlen(encodeMenu), 0);
  }
  else {
    AddReceiverSocket(socket, foundUser->sock);
    strcpy(sendMess, EncodeMessage("Enter message to send: ", 41));
    send(socket, sendMess, strlen(sendMess), 0);
  }
  
  return foundUser->sock;
}

void HandleSaveWaitMessage (int socket, char mess[], char fileName[], FILE *f) {
  // User is in another menu -> code: 42
  char sendMess[BUFF_SIZE];

  f = fopen(fileName, "a");
  clients *temp = head, *node = head;

  // find sender's name -> temp
  do {
    if (temp->sock == socket) {
      break;
    }
    temp = temp->next;
  } while (temp != NULL);
  // find receiver's name -> node
  do {
    if (node->sock == temp->receiverSock) {
      break;
    }
    node = node->next;
  } while (node != NULL);

  if (strcmp(CutString(mess), "exit") == 0 || strcmp(CutString(mess), "Exit") == 0) {
    AddReceiverSocket(socket, 0);
    AddMenuStatus(socket, '0');
    strcpy(sendMess, EncodeMessage("Exit...\n", 04));
    send(socket, sendMess, strlen(sendMess), 0);
    // strcpy(sendMess, EncodeMessage("Your friend exits..Enter exit to out chat\n", 04));
    // send(node->receiverSock, sendMess, strlen(sendMess), 0);
  } 
  else {
    if (f == NULL)
      printf("Cannot read input file!\n");
    else {
      fputs("\n", f);
      fputs(temp->clientName, f);
      fputs("\t", f);
      fputs(node->clientName, f);
      fputs("\t", f);
      fputs(CutString(mess), f);
    }
    node->haveWaitingMessage = 1;
    strcpy(sendMess, EncodeMessage("Done? If yes, enter exit. If no, enter message to send: ", 42));
    send(socket, sendMess, strlen(sendMess), 0);
  }

  fclose(f);

  return;
}

void HandleSendWaitMessage (int socket, char mess[], char fileName[], FILE *f) {
  // User is in another menu -> code: 42
  char sendMess[BUFF_SIZE], sender[BUFF_SIZE], receiver[BUFF_SIZE], message[BUFF_SIZE];

  f = fopen(fileName, "r");
  clients *temp = head;

  // find receiver's name -> temp
  do {
    if (temp->sock == socket) {
      break;
    }
    temp = temp->next;
  } while (temp != NULL);

  if (f == NULL)
    printf("Cannot read input file!\n");
  else {
    while(!feof(f)) {
      fscanf(f, "%s\t%s\t%s\n", sender, receiver, message);
      if (strcmp(receiver, temp->clientName) == 0) {
        strcpy(sendMess, sender);
        strcat(sendMess, ": ");
        strcat(sendMess, message);
        strcpy(sendMess, EncodeMessage(sendMess, 43));
        send(socket, sendMess, strlen(sendMess), 0);
      }
    }
  }
  
  fclose(f);

  return;
}

int main(int argc, char* argv[]) {
  if (argc == 1) {
    printf("Missing port number.\n");
    return 0;
  }
  else if (argc > 2) {
    printf("Wrong param.\n");
    return 0;
  }
  else {
    FILE *fUser = NULL, *fMessage = NULL;
    char fileNameUser[] = "nguoidung.txt";
    char fileNameMessage[] = "savedmessage.txt";
    char username[MAX];
    int firstChoice, secondChoice;

    readFileUser(fileNameUser, fUser);
    printNode();

    int listenfd, connfd, n, LISTENQ = 4;
    pid_t childpid;
    pid_t pid; 
    socklen_t client;
    char buff[BUFF_SIZE+1];
    struct sockaddr_in cliaddr, servaddr;
    char mess[BUFF_SIZE];

    // setup for select()
    int clientArray[MAX_CLIENT];
    fd_set readfds;
    int maxfd, nEvents, j = 0, sd, max_sd;
    struct timeval tv;
    int opt = TRUE;

    void sig_chld(int);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {   
      perror("setsockopt");   
      exit(EXIT_FAILURE);   
    } 

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
    listen(listenfd, LISTENQ);
    maxfd = listenfd;
    printf("%s\n", "Server is running..... waiting for connections.");

    while(1) {
      FD_ZERO(&readfds);
      // Set bit for listenfd
      FD_SET(listenfd, &readfds);
      max_sd = listenfd;

      // add child sockets to set
      clients *test = head;
      while (test != NULL) {
        // socket descriptor 
        sd = test->sock;

        // if valid socket descriptor then add to read list  
        if(sd > 0)   
          FD_SET(sd , &readfds);   
             
        // highest file descriptor number, need it for the select function  
        if(sd > max_sd)   
          max_sd = sd;

        test = test->next;
      }

      
      nEvents = select(maxfd + 1, &readfds, NULL, NULL, NULL);
      signal(SIGCHLD, sig_chld);
      if (FD_ISSET(listenfd, &readfds)) {
        client = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr*) &cliaddr, (socklen_t*)&client);
        maxfd = connfd;
        // add client to array
        AddClientPort(connfd);
        // send menu to client
        SendLoginAndExitMenu(connfd);    
      }

      // check status of clients
      clients *temp = head;
      
      while (temp != NULL) {
        if (FD_ISSET(temp->sock, &readfds)) {
          if (recv(temp->sock, buff, sizeof(buff), 0) > 0) {
            strtok(buff, "\n");
            // Handle LoginAndExitMenu
            if (strcmp(DecodeMessage(buff), "00") == 0) {
              HandleLoginAndExitMenu(temp->sock, buff, readfds);
            }
            // Handle username
            else if (strcmp(DecodeMessage(buff), "01") == 0) {
              strcpy(username, HandleLoginUsername(temp->sock, buff, username));
            }
            // Handle password
            else if (strcmp(DecodeMessage(buff), "02") == 0) {
              HandleLoginPassword(temp->sock, username, buff, fileNameUser, fUser);
            }
            // Handle menu-chat
            else if (strcmp(DecodeMessage(buff), "03") == 0) {
              HandleMenuChat(temp->sock, buff);
            }
            // Handle chat 1-1
            else if (strcmp(DecodeMessage(buff), "04") == 0) {
              HandleSingleChat(temp->sock, buff);
            }
            // Handle chat 1-1 send and receive message
            else if (strcmp(DecodeMessage(buff), "41") == 0) {
              HandleChat(temp->sock, buff);
            }
            // Handle save waiting message
            else if (strcmp(DecodeMessage(buff), "42") == 0) {
              HandleSaveWaitMessage(temp->sock, buff, fileNameMessage, fMessage);
            }
            // Handle send waiting message
            else if (strcmp(DecodeMessage(buff), "43") == 0) {
              HandleSendWaitMessage(temp->sock, buff, fileNameMessage, fMessage);
            }
          }
          else {
            printf("One client disconnect..\n");
            close(temp->sock);
            // FD_CLR(temp->sock, &readfds);
          }
        } // end FD_ISSET
        temp = temp->next;
      }
    }
  }

  return 0;
}