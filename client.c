#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define MSG_SIZE 32

// pareil que pour "serveur.sh", faites "./client.sh", ca recupere automatiquement votre IP (pour les tests)
// et ca attribue tout seul un nouveau port

typedef struct {
  int type;
  union {
    int i;
    char msg[MSG_SIZE];
  } info;
} data;

void* msg_recv(void* args) {
    int* dS = (int*)args;
    data recv_data;
    char* msg = (char*)malloc(MSG_SIZE); 

    while(1) {
        recv(*dS, &recv_data, sizeof(data), 0);

        if(recv_data.type == 0) { // int
            pthread_exit(NULL);
        } else if(recv_data.type == 1) { // char[]
            // Copier le message de recv_data.info.msg dans msg
            strncpy(msg, recv_data.info.msg, MSG_SIZE);
            printf("> %s", msg); // Imprimer le message
        }
    }

    free(msg);
    return NULL;
}

void* msg_send(void* args) {

  int* dS = (int*)args;
  char *msg = (char*)malloc(MSG_SIZE);

  while(1) {
    msg = (char*)malloc(MSG_SIZE);

    printf("< ");
    fgets(msg, MSG_SIZE, stdin);
    send(*dS, msg, MSG_SIZE, 0);
  }
  pthread_exit(NULL);
}

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

  pthread_t sender;
  pthread_t receiver;
  int state = 1;
  data recv_data;

  if(pthread_create(&sender, NULL, msg_send, (void*)&dS) != 0) {
      printf("erreur thread sender\n");
  }

  if(pthread_create(&receiver, NULL, msg_recv, (void*)&dS) != 0) {
      printf("erreur thread receiver\n");
  }

  pthread_join(receiver, NULL);
  pthread_cancel(sender);
  
  shutdown(dS,2) ;
  printf("Fin du programme\n");
}