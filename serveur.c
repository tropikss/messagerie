#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

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

int envoyer_message_prive(client *clients, int nb_clients, const char *pseudo_dest, const char *message)
{
  int i;
  int destinataire_trouve = 0;

  // Recherche du socket du destinataire à partir de son pseudo
  for (i = 0; i < nb_clients; i++)
  {
    if (strcmp(clients[i].nom, pseudo_dest) == 0)
    {
      destinataire_trouve = 1;
      int res = send(clients[i].id_client, message, strlen(message), 0);
      if (res == -1)
      {
        return -1; // code d'erreur si l'envoie a échoué
      }
      printf("Envoi du message '%s' à %s\n", message, pseudo_dest);
      break;
    }
  }

  if (!destinataire_trouve)
  {
    printf("Utilisateur '%s' non trouvé.\n", pseudo_dest);
    return -2; // code d'erreur pour indiquer que le destinataire n'a pas été trouvé
  }
  return 0; // tout s'est bien passé
}

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
  char *pseudo;
  char *message; // Pour recupérer le message dans la commande /mp

  int state = 1;

  while (state)
  {
    recv(id_client, msg, MSG_SIZE, 0);
    printf("%s: %s", nom, msg);

    // Vérification si le message commence par "/"
    if (msg[0] == '/')
    {
      if (strstr(msg, "/mp ") == msg) // Vérifie si le message commence par "/mp "
      {
        pseudo = strtok(msg + 4, " "); // Récupère le nom de l'utilisateur après "/mp "
        message = strtok(NULL, "");    // Récupère le reste du message comme le message à envoyer
        printf("Message privé à : %s\n", pseudo);
        printf("Contenu du message : %s\n", message);

        int resultat = envoyer_message_prive(data_client, *tab_size, pseudo, message);
        if (resultat == -1) // l'envoi du mp a echoué
        {
          const char *msg_erreur_envoi = "Erreur : Echec de l'envoi du message privé.\n";
          send(id_client, msg_erreur_envoi, strlen(msg_erreur_envoi), 0);
        }
        if (resultat == -2) // le destinataire n'a pas été trouvé
        {
          const char *msg_erreur_dest = "Erreur : Le destinataire n'a pas été trouvé.\n";
          send(id_client, msg_erreur_dest, strlen(msg_erreur_dest), 0);
        }
        if (resultat == 0) // l'envoi du message privé s'est bien passé
        {
          const char *msg_ok = "L'envoi du message privé s'est bien passé";
          send(id_client, msg_ok, strlen(msg_ok), 0);
        }
      }
      if (strcmp(msg, "/help\n\0"))
      {
        printf("Commandes disponibles :\n\n/mp <pseudo> message : Permet d'envoyé un message privé.\n/list : Liste des utilisateurs connectés.\n");
      }
      if (strcmp(msg, "/list\n\0"))
      {
        // Appel de la fonction qui va permettre d'afficher tous les utilisateurs en connectés
        printf("Commande liste des utilisateurs en ligne");
      }
      else
      {
        printf("Ce n'est pas une commande connue");
      }
    }
    else
    {
      // Si le message ne commence pas par "/", traiter comme un message normal
      send_data.type = 1;
      strncpy(send_data.info.msg, msg, MSG_SIZE);

      if (pthread_mutex_lock(mutex_client) != 0)
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
      if (pthread_mutex_unlock(mutex_client) != 0)
      {
        printf("Erreur lors du déverrouillage du mutex");
        exit(EXIT_FAILURE);
      }

      state = strcmp(msg, "fin\n\0"); // si y'a fin alors on arrete tout
    }
  }

  send_data.type = 0;
  send_data.info.i = 0;

  if (pthread_mutex_lock(mutex_client) != 0)
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
