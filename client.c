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

  while (1) {
    char * chaine1 = (char*)malloc(32);
    printf("Entrez votre message (tapez 'fin' pour quitter):");
    fgets(chaine1,32,stdin);

    if (strncmp(chaine1, "fin", 4) == 0)
      break;

    send(dS, chaine1, strlen(chaine1)+1 , 0) ;
    printf("Message Envoyé \n");

    char * r = (char*)malloc(32);
    recv(dS, &r, sizeof(r), 0) ;
    printf("Réponse reçue : %s\n", r) ;
  }
  

  shutdown(dS,2) ;
  printf("Fin du programme");
}