#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#define MSG_SIZE 32
#define MAX_CLIENT 3
#define MAX_NOM 20

// Vous pouvez lancer le programme avec "./serveur.sh", c'est un programme qui attribue automatiquement
// un nouveau port au serveur et au client, bien sur le meme entre eux

void sigHandler(int signo)
{
  if (signo == SIGUSR1)
  {
    printf("Au moins un des threads s'est terminé\n");
    exit(EXIT_SUCCESS);
  }
}

typedef struct
{
  int *tab_client_client;
  pthread_mutex_t *mutex_client;
  int id_client;
  int *tab_size;
  char *nom;
} client;

typedef struct
{
  int type;
  union
  {
    int i;
    char msg[MSG_SIZE];
  } info;
} data;

void *new_client(void *args)
{
  data send_data;

  char *msg = (char *)malloc(MSG_SIZE);
  client *data_client = (client *)args;
  int id_client = data_client->id_client;
  int *tab_client = data_client->tab_client_client;
  int *tab_size = data_client->tab_size;
  pthread_mutex_t *mutex_client = data_client->mutex_client;
  char *nom = data_client->nom;

  int state = 1;

  while (state)
  {
    recv(id_client, msg, MSG_SIZE, 0);
    printf("%s: %s", nom, msg);
    send_data.type = 1;
    strncpy(send_data.info.msg, msg, MSG_SIZE);

    if (pthread_mutex_lock(&mutex_client) != 0) // j'ai changer l'argument (avant : mutex_client) pour pouvoir comparé et traiter l'éventuelle erreur
    {
      printf("Erreur lors du verrouillage du mutex");
      exit(EXIT_FAILURE);
    }
    for (int i = 0; i < *tab_size; i++)
    {
      if (tab_client[i] != id_client)
      {
        send(tab_client[i], &send_data, sizeof(data), 0);
      }
    }
    if (pthread_mutex_unlock(&mutex_client) != 0)
    {
      printf("Erreur lors du déverrouillage du mutex");
      exit(EXIT_FAILURE);
    }

    state = strcmp(msg, "fin\n\0"); // si y'a fin alors on arrete tout
  }

  send_data.type = 0;
  send_data.info.i = 0;

  if (pthread_mutex_lock(&mutex_client) != 0)
  {
    printf("Erreur lors du verouillage du mutex");
  }
  for (int i = 0; i < *tab_size; i++)
  {
    send(tab_client[i], &send_data, sizeof(data), 0);
  }
  if (pthread_mutex_unlock(mutex_client) != 0)
  {
    printf("Erreur lors du déverouillage du mutex");
  }

  if (pthread_kill(pthread_self(), SIGUSR1) != 0)
  {
    printf("Erreur lors de l'envoi du signal au thread");
  }

  return NULL;
}

int main(int argc, char *argv[])
{

  printf("Début programme\n");

  int dS = socket(PF_INET, SOCK_STREAM, 0);
  if (dS == -1)
  {
    perror("Erreur lors de la création de la socket");
    exit(EXIT_FAILURE);
  }
  printf("Socket Créé\n");

  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY;
  ad.sin_port = htons(atoi(argv[1])); // PORT SERVEUR
  int res = bind(dS, (struct sockaddr *)&ad, sizeof(ad));
  if (res == -1)
  {
    perror("Erreur bind");
    exit(1);
  }
  printf("Socket Nommé\n");

  listen(dS, 7);
  printf("Mode écoute\n");

  int state = 1;
  int *nb_client = (int *)malloc(sizeof(int));
  *nb_client = 0;

  int *tab_client = (int *)malloc(MAX_CLIENT * sizeof(int)); // création du tableau d'id client
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  pthread_t *tab_thread = (pthread_t *)malloc(MAX_CLIENT * sizeof(pthread_t));

  struct sockaddr_in sock_client;
  socklen_t lg = sizeof(struct sockaddr_in);

  client *shared_client = (client *)malloc(sizeof(client));
  shared_client->mutex_client = &mutex;
  shared_client->tab_client_client = tab_client;

  struct sigaction sa;
  sa.sa_handler = sigHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGUSR1, &sa, NULL) != 0)
  {
    printf("Erreur lors de l'installation du gestionnaire de signal pour SIGUSR1");
  }

  printf("Initialisation réussie\n");

  while (state)
  {
    int request = accept(dS, (struct sockaddr *)&sock_client, &lg); // on attend une connexion
    printf("Demande connexion\n");
    if (*nb_client < MAX_CLIENT)
    {
      if (request > 0)
      { // on vérifie que la connexion s'est bien faite
        printf("Connexion possible\n");

        if (pthread_mutex_lock(&mutex) != 0)
        {
          printf("Erreur lors du verouillage du mutex");
        }

        tab_client[*nb_client] = request;
        printf("Client %d connecté ", *nb_client + 1);
        shared_client->id_client = request;
        shared_client->tab_size = nb_client;
        *nb_client += 1;
        char *nom = (char *)malloc(MAX_NOM * sizeof(char));
        sprintf(nom, "client n%d", *nb_client);
        shared_client->nom = nom;

        if (pthread_mutex_unlock(&mutex) != 0)
        {
          printf("Erreur lors du dévereouillage du mutex");
        }

        if (pthread_create(&tab_thread[*nb_client], NULL, new_client, shared_client) != 0)
        {
          printf("Erreur lors de la creation du thread\n");
        }
        else
        {
          printf("Creation du thread avec succès\n");
        }
      }
      else
      {
        printf("Erreur connexion\n");
      }
    }
    else
    {
      printf("Nombre maximal de client deja atteint, request refusé\n");
      shutdown(request, 2);
    }
  }

  shutdown(dS, 2);
  printf("Fin du programme\n");
}
