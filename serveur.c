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

#define PORT_FILE "1234"
//------------------------------------------
static unsigned int nb_client = 0; // Nombre de clients connectés
static int uid = 10;               // Taille des identifiants(unique) de chaque client

// Vous pouvez lancer le programme avec "./serveur.sh", c'est un programme qui attribue automatiquement
// un nouveau port au serveur et au client, bien sur le meme entre eux

// Structure du client - Information sur eux
typedef struct
{
  struct sockaddr_in address;         // Adresse du client(adresse IP et le port du client)
  int sockID;                         // Identifiant de la socket du client
  struct sockaddr_in address_fichier; // Adresse du client(adresse IP et le port du client) dédié aux fichiers
  int sockID_fichier;                 // Identifiant de la socket du client dédié aux fichiers
  int id_client;                      // Identifiant unique du client
  char nom[64];                       // Nom du client
} client;

client *clientsTab[MAX_CLIENT]; // Tableau de tous les clients connectés

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // Initialisation d'un mutex pour synchroniser l'accès concurrentiel aux données des clients

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

// Fonction qui envoie un message à l'id mis en param
void send_message_id(char *s, int uid)
{
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENT; ++i)
  {
    if (clientsTab[i])
    {
      if (clientsTab[i]->id_client == uid) // vérifie si cela est l'id
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

// Vérifie s'il n'y a pas déjà un nom pareil dans le chat
int check_name(char *s)
{
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENT; ++i)
  {
    if (clientsTab[i])
    {
      if (strcmp(clientsTab[i]->nom, s) == 0) // Même nom trouver
      {
        pthread_mutex_unlock(&clients_mutex);
        return 1;
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
  return 0;
}

// Vérifie si le message est un message privé
int check_message(char *s)
{
  char *input_msg = strdup(s);
  char *command = strtok(input_msg, " ");
  command = strtok(NULL, " "); // Extracte le deuxième mot du message

  char *input_msg_file = strdup(s);
  char *command_file = strtok(input_msg_file, " ");
  command_file = strtok(NULL, "\n"); // Extracte le deuxième mot du message

  if (strcmp(command, "/mp") == 0) // Vérifie si c'est /mp
  {
    return 1;
  }
  else if (strcmp(command_file, "/file") == 0) // Vérifie si c'est /file
  {
    return 3;
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
void send_message_priv(char *s, char *destinateur, int uid)
{
  int receiver_exist = 0;
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENT; ++i)
  {
    if (clientsTab[i])
    {
      if (strcmp(clientsTab[i]->nom, destinateur) == 0)
      {
        receiver_exist = 1;
        if (write(clientsTab[i]->sockID, s, strlen(s)) < 0)
        {
          perror("ERREUR: Envoie du message échoué");
          break;
        }
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);

  //-----------------MODIFICATION POOMEDY------------------------------
  if (receiver_exist == 0)
  {
    printf("Destinataire n'existe pas\n");
    send_message_id("Destinataire n'existe pas\n", uid);
  }
}
//-------------------------------------------------------------------

//-----------------MODIFICATION POOMEDY------------------------------
// Gére la réception de fichier du client
void receive_file(int client_sock)
{
  char filename[256];
  char file_size_str[16];
  int file_size;
  FILE *file;
  char buffer[1024];

  // Reçoit le nom du fichier
  recv(client_sock, filename, sizeof(filename), 0);
  filename[strcspn(filename, "\n")] = 0;

  // Reçoit la taille du fichier
  recv(client_sock, file_size_str, sizeof(file_size_str), 0);
  file_size = atoi(file_size_str);

  // Ouverture du fichier pour l'écriture
  char filepath[512];
  snprintf(filepath, sizeof(filepath), "./server_folder/%s", filename);
  file = fopen(filepath, "wb");
  if (!file)
  {
    perror("Fichier ne peut être ouvert");
    return;
  }

  // Reçoit le contenu du fichier
  int total_received = 0;
  while (total_received < file_size)
  {
    int bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0)
    {
      break;
    }
    printf("%s",buffer);
    fwrite(buffer, 1, bytes_received, file);
    total_received += bytes_received;
    printf("Reçu: %d/%d octets\n", total_received, file_size);
  }

  fclose(file);
  printf("Fichier %s reçu avec succès.\n", filename);
}

// Gère la réception de la taille
int taille_recv(int uid)
{
  int taille;
  int response = recv(uid, &taille, sizeof(int), 0);
  printf("Taille reçu: %d\n", taille);
  if (response > 0)
  {
    return taille + 1;
  }
}

// Gère l'envoi de la taille
void taille_send(int uid, char *sortie)
{
  int taille = strlen(sortie);
  printf("Taille envoyé: %d\n", taille);
  send(uid, &taille + 1, sizeof(int), 0);
}
//-------------------------------------------------------------------

// Gère la communication entre les clients (envoie et réception)
// Executer dans un thread séparé pour chaque client
void *new_client(void *args)
{
  char message[MSG_SIZE];               // Message sortant
  char nom[64];                         // Nom du client
  int state = 0;                        // Indique si le client quitte la convo
  nb_client++;                          // Incrémente le nombre de clients
  client *data_client = (client *)args; // Pointeur de type client pour accéder aux données du client

  // Vérification du nom du client
  int valid = 1;
  while (valid)
  {
    if (recv(data_client->sockID, nom, 64, 0) <= 0 || strlen(nom) < 2 || strlen(nom) >= 64 - 1 || check_name(nom) == 1)
    {
      printf("Nom incorrecte.\n");
      send_message_id("Nom incorrecte.", data_client->id_client);
    }
    else
    {
      send_message_id("Nom correcte", data_client->id_client);
      strcpy(data_client->nom, nom);
      sprintf(message, "%s a rejoint le chat\n", data_client->nom);
      printf("%s", message);
      send_message(message, data_client->id_client);
      valid = 0;
    }
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

    if (receive > 0) // Messages normales ou privés ou fichier
    {
      if (strlen(message) > 0)
      {
        int result = check_message(message);
        if (result == 1) // c'est un message privé
        {
          char **info = get_name_and_message(message);
          char concatenated[MSG_SIZE];
          strcpy(concatenated, data_client->nom);
          strcat(concatenated, ": ");
          strcat(concatenated, info[1]);
          send_message_priv(concatenated, info[0], data_client->id_client);
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
        else if (result == 3) // c'est un fichier
        {
          receive_file(data_client->sockID_fichier);
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

    else if (receive == 0 || strcmp(message, "/exit") == 0) // Demande d'arrêt
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
  close(data_client->sockID_fichier);    // Fermeture du socket du client dédié aux fichiers
  retire_client(data_client->id_client); // Suppression du client de la file d'attente
  free(data_client);                     // Libération de la mémoire
  nb_client--;                           // Décrémentation du nombre total de clients
  pthread_detach(pthread_self());

  return NULL;
}

int main(int argc, char *argv[])
{
  /* ------------------------SOCKET MESSAGES------------------------------------*/
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

  struct sockaddr_in sock_client;
  pthread_t tid;
  /* ------------------------SOCKET MESSAGES------------------------------------*/

  /* ------------------------SOCKET FICHIERS------------------------------------*/
  int dS_fichier = socket(PF_INET, SOCK_STREAM, 0);

  /* Socket settings */
  struct sockaddr_in ad_fichier;
  ad_fichier.sin_family = AF_INET;
  ad_fichier.sin_addr.s_addr = INADDR_ANY;
  ad_fichier.sin_port = htons(atoi(PORT_FILE)); // PORT SERVEUR

  /* Ignore pipe signals */
  signal(SIGPIPE, SIG_IGN);
  int option_fichier = 1;
  if (setsockopt(dS_fichier, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option_fichier, sizeof(option_fichier)) < 0)
  {
    perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
  }

  /* Bind */
  int res_fichier = bind(dS_fichier, (struct sockaddr *)&ad_fichier, sizeof(ad_fichier));
  if (res_fichier == -1)
  {
    perror("Erreur bind");
    exit(1);
  }

  /* Listen */
  listen(dS_fichier, 7);

  struct sockaddr_in sock_client_fichier;
  /* ------------------------SOCKET FICHIERS------------------------------------*/

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

        // Alloue de la mémoire pour un nouveau client
        client *newClient = (client *)malloc(sizeof(client));
        // Initialise ses données
        newClient->address = sock_client;
        newClient->sockID = request;
        newClient->id_client = uid++;

        socklen_t lg_fichier = sizeof(sock_client_fichier);
        int request_fichier = accept(dS_fichier, (struct sockaddr *)&sock_client_fichier, &lg_fichier);

        if (request_fichier > 0)
        { // on vérifie que la connexion s'est bien faite
          newClient->address_fichier = sock_client_fichier;
          newClient->sockID_fichier = request_fichier;

          sleep(1);
        }
        else
        {
          printf("Erreur connexion pour l'envoie des fichiers\n");
        }

        // Ajoute le nouveau client à la file d'attente des client
        ajout_client(newClient);
        // Crée un thread pour gérer la communication des messages normales avec le client
        pthread_create(&tid, NULL, &new_client, (void *)newClient);

        sleep(1);
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

  shutdown(dS_fichier, 2);
  shutdown(dS, 2);
  printf("Fin du programme\n");
}
