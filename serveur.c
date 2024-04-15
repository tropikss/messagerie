#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int msg_send(int dS, char * msg, int size) {
  return(send(dS, msg, size, 0) == size);
}

int main(int argc, char *argv[]) {
  
  printf("Début programme\n");

  int dS = socket(PF_INET, SOCK_STREAM, 0);
  printf("Socket Créé\n");


  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY ;
  ad.sin_port = htons(atoi(argv[1])) ; // PORT SERVEUR
  int res = bind(dS, (struct sockaddr*)&ad, sizeof(ad));
  if(res == -1) {
    perror("Erreur bind");
    exit(1);
  }
  printf("Socket Nommé\n");

  listen(dS, 7) ;
  printf("Mode écoute\n");

  struct sockaddr_in sock_client1 ;
  socklen_t lg1 = sizeof(struct sockaddr_in) ;

  struct sockaddr_in sock_client2 ;
  socklen_t lg2 = sizeof(struct sockaddr_in) ;

  int client1 = accept(dS, (struct sockaddr*) &sock_client1,&lg1) ;
  if(client1 > 0) {
    printf("Client 1 connecté\n");
    int premier = 0;
    send(client1, &premier, sizeof(int), 0); // on transmet a 1 qu'il enverra en premier
  }

  int client2 = accept(dS, (struct sockaddr*) &sock_client2,&lg2);
    if(client2 > 0) {
      printf("Client 2 connecté\n\n");
      int deuxieme = 1;
      send(client2, &deuxieme, sizeof(int), 0); // on transmet a 1 qu'il envvera en premier
    }

  int actif = 1;
  int MSG_SIZE = 32;
  char *msg = (char*)malloc(MSG_SIZE);
  int sender = client1;
  int receiver = client2;

  while(actif) {

    msg = (char*)malloc(MSG_SIZE);
    recv(sender, msg, MSG_SIZE, 0);
    printf("> %s",msg);
    if(send(receiver, msg, MSG_SIZE, 0) != -1) {
      if(strcmp(msg, "fin\n\0") == 0) {
        actif = 0;
      }
      send(sender, &actif, sizeof(int), 0);
      send(receiver, &actif, sizeof(int), 0);
    } else {
      printf("erreur envoi\n");
    }

    int temp = sender;
    sender = receiver;
    receiver = temp;
  }

  shutdown(client1, 2) ; 
  shutdown(client2, 2) ;
  shutdown(dS, 2) ;
  printf("Fin du programme\n");
}
