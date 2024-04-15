#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

// pareil que pour "serveur.sh", faites "./client.sh", ca recupere automatiquement votre IP (pour les tests)
// et ca attribue tout seul un nouveau port

int main(int argc, char *argv[]) {

  printf("Début programme\n");
  int dS = socket(PF_INET, SOCK_STREAM, 0);
  printf("Socket Créé\n");

  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ;
  aS.sin_port = htons(atoi(argv[2])) ;
  socklen_t lgA = sizeof(struct sockaddr_in) ;

  if(connect(dS, (struct sockaddr *) &aS, lgA) == -1) { // tentative connexion au serveur
    printf("Pas de serveur trouvé\n"); // echec
    exit(1);
  }
  printf("Socket Connecté\n"); // reussite

  int role;
  recv(dS, &role, sizeof(role), 0); // on recupere le role du client, 0 = envoi en premier, 1 = ecoute en premier

  if(role == 0) {
    printf("Je suis le premier client\n");
  } else {
    printf("Je suis le deuxieme client\n");
  }

  int actif = 1; // actif : etat du client
  int MSG_SIZE = 32; // tailel des msg envoyé, a changer pa la suitep our mettre un truc variable qu'on recupere de puis le serveur

  while (actif) {

    if(role == 0) { // envoi
      char *msg = (char*)malloc(MSG_SIZE);
      printf("< ");
      fgets(msg, MSG_SIZE, stdin);
      if(send(dS, msg, MSG_SIZE, 0) != -1) { // si le send n'echoue pas
        recv(dS, &actif, sizeof(int), 0); // on met dans actif le retour du serveur -> c'est le serveur
        // qui determine si on doit arreter la connexion ou pas, donc le serveur renvoie l'etat que doit avoir actif du client
        role = 1; // on change les roles, le sender devient receiver et inversemment
      } else {
        printf("erreur envoi\n");
      }
    } else {
      char *msg = (char*)malloc(MSG_SIZE);
      recv(dS, msg, MSG_SIZE, 0); // on recupere le message
      recv(dS, &actif, sizeof(int), 0); // et pareil, on modifie actif suivant la reponse serveur
      printf("> %s", msg);
      role = 0; // on change les roles
    }
  }
  
  shutdown(dS,2) ;
  printf("Fin du programme\n");
}