#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

// Vous pouvez lancer le programme avec "./serveur.sh", c'est un programme qui attribue automatiquement
// un nouveau port a serveur et client, bien sur le meme entre eux

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

  int client1 = accept(dS, (struct sockaddr*) &sock_client1,&lg1) ; // on recupere le client 1 
  if(client1 > 0) {
    printf("Client 1 connecté\n");
    int premier = 0;
    send(client1, &premier, sizeof(int), 0); // on transmet a 1 qu'il enverra en premier
  }

  int client2 = accept(dS, (struct sockaddr*) &sock_client2,&lg2); // on recupere le client 2
    if(client2 > 0) {
      printf("Client 2 connecté\n\n");
      int deuxieme = 1;
      send(client2, &deuxieme, sizeof(int), 0); // on transmet a 1 qu'il envvera en premier
    }

  int actif = 1;
  int MSG_SIZE = 32;
  char *msg = (char*)malloc(MSG_SIZE);

  int sender = client1; // personne qui enverra en premier, ici client1 par default
  int receiver = client2; // pareil amis client 2 et recevoir

  while(actif) {

    msg = (char*)malloc(MSG_SIZE);
    recv(sender, msg, MSG_SIZE, 0);
    printf("> %s",msg);
    if(send(receiver, msg, MSG_SIZE, 0) != -1) { // si l'envoi n'echoue pas
      if(strcmp(msg, "fin\n\0") == 0) { // si le message recu vaut "fin", y'avait \n et \0 finalement a la fin
        actif = 0; // si y'a fin alors on arrete tout
      }
      send(sender, &actif, sizeof(int), 0); // et on envoie au deux client d'arreter aussi
      send(receiver, &actif, sizeof(int), 0);
    } else {
      printf("erreur envoi\n");
    }

    int temp = sender; // on stocke le sender le temps de la reatribution
    sender = receiver; // et on echange l'envoyeur et le receveur
    receiver = temp;
  }

  shutdown(client1, 2) ; 
  shutdown(client2, 2) ;
  shutdown(dS, 2) ;
  printf("Fin du programme\n");
}
