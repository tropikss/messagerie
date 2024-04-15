#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

int main(int argc, char *argv[]) {

  printf("Début programme\n");
  int dS = socket(PF_INET, SOCK_STREAM, 0);
  printf("Socket Créé\n");

  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ;
  aS.sin_port = htons(atoi(argv[2])) ;
  socklen_t lgA = sizeof(struct sockaddr_in) ;
  if(connect(dS, (struct sockaddr *) &aS, lgA) == -1) {
    printf("Pas de serveur trouvé\n");
    exit(1);
  }
  printf("Socket Connecté\n");

  int role;
  recv(dS, &role, sizeof(role), 0);

  if(role == 0) {
    printf("Je suis le premier client\n");
  } else {
    printf("Je suis le deuxieme client\n");
  }

  int actif = 1;
  int MSG_SIZE = 32;

  while (actif) {

    if(role == 0) {
      char *msg = (char*)malloc(MSG_SIZE);
      printf("< ");
      fgets(msg, MSG_SIZE, stdin);
      if(send(dS, msg, MSG_SIZE, 0) != -1) {
        recv(dS, &actif, sizeof(int), 0);
        role = 1;
      } else {
        printf("erreur envoi\n");
      }
    } else {
      char *msg = (char*)malloc(MSG_SIZE);
      recv(dS, msg, MSG_SIZE, 0);
      recv(dS, &actif, sizeof(int), 0);
      printf("> %s", msg);
      role = 0;
    }
  }
  
  shutdown(dS,2) ;
  printf("Fin du programme\n");
}