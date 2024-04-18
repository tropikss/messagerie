#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#define MSG_SIZE 32

// Vous pouvez lancer le programme avec "./serveur.sh", c'est un programme qui attribue automatiquement
// un nouveau port au serveur et au client, bien sur le meme entre eux

void sigHandler(int signo) {
    if (signo == SIGUSR1) {
        printf("Au moins un des threads s'est terminé\n");
        exit(EXIT_SUCCESS);
    }
}

typedef struct {
  int type;
  union {
    int i;
    char msg[MSG_SIZE];
  } info;
} data;

typedef struct {
  int client1;
  int client2;
} Couple;

void* msg_link(void* args) {
  Couple* thread_args = (Couple*)args;
  int sender = thread_args->client1;
  int receiver = thread_args->client2;
  data send_data;

  char * msg = (char*)malloc(MSG_SIZE);
  int state = 1;

  while(state) {
    recv(sender, msg, MSG_SIZE, 0);
    printf("> %s",msg);
    send_data.type = 1;
    strncpy(send_data.info.msg, msg, MSG_SIZE);
    send(receiver, &send_data, sizeof(data), 0);

    state = strcmp(msg, "fin\n\0"); // si y'a fin alors on arrete tout
  }
  send_data.type = 0;
  send_data.info.i = 0;
  send(receiver, &send_data, sizeof(data), 0);
  send(sender, &send_data, sizeof(data), 0);
  pthread_kill(pthread_self(), SIGUSR1);

  return NULL;
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

  int client1 = accept(dS, (struct sockaddr*) &sock_client1,&lg1) ; // on recupere le client 1 
  if(client1 > 0) {
    printf("Client 1 connecté\n");
  }

  int client2 = accept(dS, (struct sockaddr*) &sock_client2,&lg2); // on recupere le client 2
    if(client2 > 0) {
      printf("Client 2 connecté\n\n");
    }

  int actif = 1;
  char *msg = (char*)malloc(MSG_SIZE);

  Couple link1;
  link1.client1 = client1;
  link1.client2 = client2;

  Couple link2;
  link2.client1 = client2;
  link2.client2 = client1;

  pthread_t thread_client1;
  pthread_t thread_client2;

  struct sigaction sa;
  sa.sa_handler = sigHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);

  if(pthread_create(&thread_client1, NULL, msg_link, (void*)&link1) != 0) {
    printf("erreur thread 1\n");
    exit(1);
  }

  if(pthread_create(&thread_client2, NULL, msg_link, (void*)&link2) != 0) {
    printf("erreur thread 2\n");
    exit(1);
  }

  pause();

  shutdown(client1, 2); 
  shutdown(client2, 2);
  shutdown(dS, 2) ;
  printf("Fin du programme\n");
}
