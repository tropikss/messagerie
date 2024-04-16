#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MSG_SIZE 500
#define MAX_CLIENTS 2

int client_sockets[MAX_CLIENTS];
pthread_t client_threads[MAX_CLIENTS];

// Vous pouvez lancer le programme avec "./serveur.sh", c'est un programme qui attribue automatiquement
// un nouveau port a serveur et client, bien sur le meme entre eux


//Fonction pour relayer les messages entre les clients
void *relay_messages(void *arg) {
  int sender_index = *((int *) arg);
  int receiver_index = (sender_index + 1) % MAX_CLIENTS;

  char message[MSG_SIZE];
  int bytes_received;

  while ((bytes_received = recv(client_sockets[sender_index], message, MSG_SIZE, 0)) > 0) {
    message[bytes_received] = '\0';
    send(client_sockets[receiver_index], message, strlen(message), 0);
    // Vérifie si le message reçu est "fin"
    if (strcmp(message, "fin\n") == 0) {
      printf("Client %d a terminé la conversation.\n", sender_index + 1);
      break; // Sort de la boucle while
    }
  }
  //Si le client se déconnecte, ferme le socket et arrête le thread
  close(client_sockets[sender_index]);
  client_sockets[sender_index] = -1;
  printf("Client %d disconnected.\n", sender_index + 1);
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

  //Accepte les connexions des clients
  for (int i = 0; i < MAX_CLIENTS; i++) {
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    client_sockets[i] = accept(dS, (struct sockaddr *) &client_address, &client_address_len);
    if (client_sockets[i] == -1) {
      perror("accept");
      close(dS);
      return EXIT_FAILURE;
    }
    printf("Client %d connected.\n", i + 1);

    //Crée un thread pour chaque client
    int *client_index_ptr = malloc(sizeof(int));
    *client_index_ptr = i;
    pthread_create(&client_threads[i], NULL, relay_messages, client_index_ptr);
  }

  //Attend la fin de tous les threads clients
  for (int i = 0; i < MAX_CLIENTS; i++) {
      pthread_join(client_threads[i], NULL);
  }

  close(dS);
  return EXIT_SUCCESS;
}
