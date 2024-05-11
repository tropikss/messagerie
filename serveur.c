#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

//------------------------------------------
#define MSG_SIZE 100  // Taille max du message
#define MAX_CLIENT 10 // Nombre max de clients
//------------------------------------------
static unsigned int nb_client = 0; // Nombre de clients connectés
static int uid = 10;               // Taille des identifiants(unique) de chaque client

// Vous pouvez lancer le programme avec "./serveur.sh", c'est un programme qui attribue automatiquement
// un nouveau port au serveur et au client, bien sur le meme entre eux

//***********************OUTDATE**************************
// void sigHandler(int signo) {
//     if (signo == SIGUSR1) {
//         printf("Au moins un des threads s'est terminé\n");
//         exit(EXIT_SUCCESS);
//     }
// }

// Structure du client - Information sur eux
typedef struct
{
  struct sockaddr_in address; // Adresse du client(adresse IP et le port du client)
  int sockID;                 // Identifiant de la socket du client
  int id_client;              // Identifiant unique du client
  char nom[64];               // Nom du client
} client;

client *clientsTab[MAX_CLIENT]; // Tableau de tous les clients connectés

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // Initialisation d'un mutex pour synchroniser l'accès concurrentiel aux données des clients

//***********************OUTDATE**************************
// typedef struct {
//   int type;
//   int taille; //ajout de la taille du message pas généré par chatGPT :P MathisssEEEuuu (sais qu'il va même pas regardez mon code)
//   union {
//     int i;
//     char msg[MSG_SIZE];
//   } info;
// } data;

// Fonction qui ajoute un nouveau client
void ajout_client(client *nouveau_client)
{
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENT; ++i)
  {
    if (!clientsTab[i])
    {
      clientsTab[i] = nouveau_client;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

// Fonction qui retire un client du tableau à l'aide de son identifiant
void retire_client(int uid)
{
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENT; ++i)
  {
    if (clientsTab[i])
    {
      if (clientsTab[i]->id_client == uid)
      {
        clientsTab[i] = NULL;
        break;
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

// Fonction qui envoie un message à tout le monde sauf au sender
void send_message(char *s, int uid)
{
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENT; ++i)
  {
    if (clientsTab[i])
    {
      if (clientsTab[i]->id_client != uid) // vérifie si cela n'est id du sender
      {
        if (write(clientsTab[i]->sockID, s, strlen(s)) < 0)
        {
          perror("ERREUR: Envoie du message échoué");
          break;
        }
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

// Vérifie si le message est un message privé
int check_message(char *s)
{
  char *input_msg = strdup(s);
  char *command = strtok(input_msg, " ");
  command = strtok(NULL, " ");     // Extracte le deuxième mot du message
  if (strcmp(command, "/mp") == 0) // Vérifie si c'est /mp
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// Récupère le nom du destinataire et le message à envoyer
char **get_name_and_message(char *s)
{
  char *input_msg = strdup(s);
  char **result = malloc(2 * sizeof(char *)); // Allocate memory for an array of two strings
  if (result == NULL)
  {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  char *token = strtok((char *)input_msg, " ");
  if (token != NULL)
  {
    token = strtok(NULL, " "); // On n'utilise pas le premier mot (nom:)
    token = strtok(NULL, " "); // Ni le deuxième (/mp)
    if (token != NULL)
    {
      result[0] = strdup(token); // Copier le 3eme mot - destinataire
      token = strtok(NULL, " ");
      if (token != NULL)
      {
        result[1] = strdup(token); // Copier le 4eme mot - message privé
      }
      else
      {
        result[1] = strdup("");
      }
    }
    else
    {
      result[0] = strdup("");
      result[1] = strdup("");
    }
  }
  return result;
}

// Fonction qui envoie un message privé au destinateur
void send_messagePRV(char *s, char *destinateur)
{
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENT; ++i)
  {
    if (clientsTab[i])
    {
      if (strcmp(clientsTab[i]->nom, destinateur) == 0)
      {
        if (write(clientsTab[i]->sockID, s, strlen(s)) < 0)
        {
          perror("ERREUR: Envoie du message échoué");
          break;
        }
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

// Gère la communication entre les clients (envoie et réception)
// Executer dans un thread séparé pour chaque client
void *new_client(void *args)
{
  //***********************OUTDATE**************************
  // data send_data;
  // client *data_client = (client *)args;
  // int id_client = data_client->id_client;
  // int *tab_client = data_client->tab_client_client;
  // int *tab_size = data_client->tab_size;
  // pthread_mutex_t *mutex_client = data_client->mutex_client;
  // char *nom = data_client->nom;

  char message[MSG_SIZE];               // Message sortant
  char nom[64];                         // Nom du client
  int state = 0;                        // Indique si le client quitte la convo
  nb_client++;                          // Incrémente le nombre de clients
  client *data_client = (client *)args; // Pointeur de type client pour accéder aux données du client

  // Vérification du nom du client
  if (recv(data_client->sockID, nom, 64, 0) <= 0 || strlen(nom) < 2 || strlen(nom) >= 64 - 1)
  {
    printf("Taille incorrecte.\n");
    state = 1;
  }
  else
  {
    strcpy(data_client->nom, nom);
    sprintf(message, "%s a rejoint le chat\n", data_client->nom);
    printf("%s", message);
    send_message(message, data_client->id_client);
  }

  memset(message, 0, MSG_SIZE);

  while (1)
  {
    if (state) // le client doit quiiter le chat
    {
      break;
    }

    // Réception d'un message du client
    int receive = recv(data_client->sockID, message, MSG_SIZE, 0);
    if (receive > 0)
    {
      if (strlen(message) > 0)
      {
        if (check_message(message) == 1) // c'est un message privé
        {
          char **info = get_name_and_message(message);
          send_messagePRV(info[1], info[0]);
          int i;
          for (i = 0; i < strlen(message); i++)
          { // trim \n
            if (message[i] == '\n')
            {
              message[i] = '\0';
              break;
            }
          }
          printf("%s \n", message);
        }
        else
        {
          // Message normal
          send_message(message, data_client->id_client);
          int i;
          for (i = 0; i < strlen(message); i++)
          { // trim \n
            if (message[i] == '\n')
            {
              message[i] = '\0';
              break;
            }
          }
          printf("%s \n", message);
        }
      }
    }

    else if (receive == 0 || strcmp(message, "/exit") == 0)
    {
      sprintf(message, "%s a quitté le chat\n", data_client->nom);
      printf("%s", message);
      send_message(message, data_client->id_client);
      state = 1;
    }
    else
    {
      printf("ERREUR: -1\n");
      state = 1;
    }
    memset(message, 0, MSG_SIZE);
  }

  close(data_client->sockID);            // Fermeture du socket du client
  retire_client(data_client->id_client); // Suppression du client de la file d'attente
  free(data_client);                     // Libération de la mémoire
  nb_client--;                           // Décrémentation du nombre total de clients
  pthread_detach(pthread_self());

  return NULL;
}

int main(int argc, char *argv[])
{

  printf("Début programme\n");

  int dS = socket(PF_INET, SOCK_STREAM, 0);
  printf("Socket Créé\n");

  /* Socket settings */
  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY;
  ad.sin_port = htons(atoi(argv[1])); // PORT SERVEUR

  /* Ignore pipe signals */
  signal(SIGPIPE, SIG_IGN);
  int option = 1;
  if (setsockopt(dS, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0)
  {
    perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
  }

  /* Bind */
  int res = bind(dS, (struct sockaddr *)&ad, sizeof(ad));
  if (res == -1)
  {
    perror("Erreur bind");
    exit(1);
  }
  printf("Socket Nommé\n");

  /* Listen */
  listen(dS, 7);
  printf("Mode écoute\n");

  //***********************OUTDATE**************************
  // int *nb_client = (int *)malloc(sizeof(int));
  // *nb_client = 0;

  // int *tab_client = (int *)malloc(MAX_CLIENT * sizeof(int)); // création du tableau d'id client
  // pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  // pthread_t *tab_thread = (pthread_t *)malloc(MAX_CLIENT * sizeof(pthread_t));

  // struct sockaddr_in sock_client;
  // socklen_t lg = sizeof(struct sockaddr_in);

  // client *shared_client = (client *)malloc(sizeof(client));
  // shared_client->mutex_client = &mutex;
  // shared_client->tab_client_client = tab_client;

  // struct sigaction sa;
  // sa.sa_handler = sigHandler;
  // sigemptyset(&sa.sa_mask);
  // sa.sa_flags = 0;
  // sigaction(SIGUSR1, &sa, NULL);

  // printf("Initialisation réussie\n");

  struct sockaddr_in sock_client;
  pthread_t tid;

  while (1)
  {
    socklen_t lg = sizeof(sock_client);
    int request = accept(dS, (struct sockaddr *)&sock_client, &lg); // on attend une connexion
    printf("Demande connexion\n");
    if (nb_client < MAX_CLIENT) // vérifie si on atteint pas le nb max
    {
      if (request > 0)
      { // on vérifie que la connexion s'est bien faite
        printf("Connexion possible\n");

        // Alloue de la mémoire pour un nouveau client, puis initialise ses données
        client *newClient = (client *)malloc(sizeof(client));
        newClient->address = sock_client;
        newClient->sockID = request;
        newClient->id_client = uid++;

        // Ajoute le nouveau client à la file d'attente des client
        ajout_client(newClient);
        // Crée un thread pour gérer la communication avec le client
        pthread_create(&tid, NULL, &new_client, (void *)newClient);
        sleep(1);

        //***********************OUTDATE**************************
        // pthread_mutex_lock(&mutex);
        // tab_client[*nb_client] = request;
        // printf("Client %d connecté ", *nb_client + 1);
        // shared_client->id_client = request;
        // shared_client->tab_size = nb_client;
        // *nb_client += 1;
        // char *nom = (char *)malloc(MAX_NOM * sizeof(char));
        // sprintf(nom, "client n%d", *nb_client);
        // shared_client->nom = nom;

        // pthread_mutex_unlock(&mutex);

        // if (pthread_create(&tab_thread[*nb_client], NULL, new_client, shared_client) != 0)
        // {
        //   printf("avec erreur\n");
        // }
        // else
        // {
        //   printf("avec succès\n");
        // }
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
