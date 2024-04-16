#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MSG_SIZE 500

int dS;

// pareil que pour "serveur.sh", faites "./client.sh", ca recupere automatiquement votre IP (pour les tests)
// et ca attribue tout seul un nouveau port

void *receive_message(void *arg) { //processus pour recevoir les messages
  while (1) {
    char message[MSG_SIZE];
      int bytes_received = recv(dS, message, MSG_SIZE, 0);
      if (bytes_received <= 0) {//si le client ne reçoit plus du serveur
          printf("Server disconnected.\n");
          break;
      }
      message[bytes_received] = '\0';
      printf("> %s", message);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  printf("Début programme\n");
  dS = socket(PF_INET, SOCK_STREAM, 0);
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
  
  //Création de thread pour recevoir les messages
  pthread_t receive_tid;
  pthread_create(&receive_tid, NULL, receive_message, NULL);

  char message[MSG_SIZE];
    while (1) {
      printf("< ");
      fgets(message, MSG_SIZE, stdin);
      send(dS, message, strlen(message), 0);
    }

  close(dS);
  return EXIT_SUCCESS;
}
