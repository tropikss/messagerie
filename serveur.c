#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

    FILE *file;
    char ip_address[20];
    
    // Ouvre le fichier en mode lecture
    file = fopen("adresse_ip.txt", "r");

    // Vérifie si le fichier a été ouvert avec succès
    if (file == NULL) {
        printf("Erreur lors de l'ouverture du fichier.\n");
        return 1;
    }

    // Lit l'adresse IP à partir du fichier
    if (fgets(ip_address, 20, file) != NULL) {
        printf("Adresse IP récupérée depuis le fichier : %s\n", ip_address);
    } else {
        printf("Erreur lors de la lecture du fichier.\n");
    }

    // Ferme le fichier
    fclose(file);
  
  printf("Début programme\n");

  int dS = socket(PF_INET, SOCK_STREAM, 0);
  printf("Socket Créé\n");


  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY ;
  ad.sin_port = htons(atoi("1234")) ;
  int res = bind(dS, (struct sockaddr*)&ad, sizeof(ad));
  printf("res : %d\n", res);
  printf("Socket Nommé\n");

  listen(dS, 7) ;
  printf("Mode écoute\n");

  struct sockaddr_in sock_client1 ;
  socklen_t lg1 = sizeof(struct sockaddr_in) ;

  struct sockaddr_in sock_client2 ;
  socklen_t lg2 = sizeof(struct sockaddr_in) ;
  
  int rep_client1 = accept(dS, (struct sockaddr*) &sock_client1,&lg1) ;
  if(rep_client1 == 0) {
    printf("Client 1 connecté");
  }

  int rep_client2 = accept(dS, (struct sockaddr*) &sock_client2,&lg2) ;
    if(rep_client2 == 0) {
    printf("Client 2 connecté");
  }

  int actif = 1;
  
  char msg [50];
  memset(msg, 0, sizeof(msg));
  char r [10] = "recu";

  while(actif) {

    // de 1 a 2

    memset(msg, 0, sizeof(msg)); // clear de msg

    recv(rep_client1, msg, sizeof(msg), 0) ; // on attend le message de 1
    printf("message recu : %s", msg);
    if(msg == "fin") {
        printf("fin conversation");
        break;
    }
    send(rep_client2, &msg, sizeof(msg), 0); // on le transmet a 2
    printf("message renvoyee");

    recv(rep_client2, msg, sizeof(msg), 0) ; // on attend la reponse de 2
    printf("reponse recu : %s", msg);
    send(rep_client1, &msg, sizeof(msg), 0); // qu'on transmet a 1
    printf("reponse renvoyee");

    // de 2 a 1

    memset(msg, 0, sizeof(msg)); // clear de msg

    recv(rep_client2, msg, sizeof(msg), 0) ; // on attend le message de 2
    printf("message recu : %s", msg);
    if(msg == "fin") {
        printf("fin conversation");
        break;
    }
    send(rep_client1, &msg, sizeof(msg), 0); // on le transmet a 1
    printf("message reenvoye");

    recv(rep_client1, msg, sizeof(msg), 0) ; // on attend la reponse de 1
    printf("reponse recu : %s", msg);
    send(rep_client2, &msg, sizeof(msg), 0); // qu'on transmet a 2
    printf("reponse reenvoyee");

  }

  shutdown(rep_client1, 2) ; 
  shutdown(rep_client2, 2) ;
  shutdown(dS, 2) ;
  printf("Fin du programme\n");
}