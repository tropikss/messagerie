/**
 * @file serveur.c
 * @brief Programme serveur de chat multi-client
 */

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>

//------------------------------------------
#define MSG_SIZE 256  /**< Taille max du message */
#define MAX_CLIENT 10 /**< Nombre max de clients */

#define MAX_CHAINES 10           /**< Nombre max de chaînes créées */
#define MAX_CLIENTS_PAR_CHAINE 2 /**< Nombre max de clients par chaînes */
//------------------------------------------
static unsigned int nb_client = 0; /**< Nombre de clients connectés */
static int uid = 10;               /**< Taille des identifiants(unique) de chaque client */
sem_t sem_client;

/**
 * Structure représentant un client
 */
typedef struct
{
  struct sockaddr_in address; /**< Adresse du client (adresse IP et le port du client) */
  struct sockaddr_in address_file;
  int sockID;                                 /**< Identifiant de la socket du client */
  int id_client;                              /**< Identifiant unique du client */
  char nom[64];                               /**< Nom du client */
  struct chaine_discussion *chaine_connectee; // Pointeur vers la chaîne à laquelle le client est connecté
  int sockID_file;
  int id_client_file;
} client;

client *clientsTab[MAX_CLIENT]; /**< Tableau de tous les clients connectés */

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; /**< Mutex pour synchroniser l'accès concurrentiel aux données des clients */

/**
 * Structure représentant une chaîne de discussion
 */
typedef struct chaine_discussion
{
  char nom[64];
  char description[256];
  client *clients[MAX_CLIENTS_PAR_CHAINE];
  int nb_clients;
} chaine_discussion;

chaine_discussion chaines[MAX_CHAINES];
int nb_chaines = 0;

pthread_mutex_t chaines_mutex = PTHREAD_MUTEX_INITIALIZER; /**< Mutex pour synchroniser l'accès concurrentiel aux chaînes de discussion */

/**
 * Initialiser les chaînes de discussion au démarrage
 */
void init_chaines()
{
  pthread_mutex_lock(&chaines_mutex);
  // Initialiser quelques chaînes pour l'exemple
  for (int i = 0; i < MAX_CHAINES; ++i)
  {
    snprintf(chaines[i].nom, sizeof(chaines[i].nom), "Chaine %d", i + 1);
    snprintf(chaines[i].description, sizeof(chaines[i].description), "Description de la chaîne %d", i + 1);
    chaines[i].nb_clients = 0;
    nb_chaines++;
  }
  pthread_mutex_unlock(&chaines_mutex);
}

/**
 * Lister les chaînes disponibles pour un client
 * * @param sock Identifiant unique du client à aui afficher la liste
 */
void liste_chaines(int sock)
{
  char buffer[1024];
  pthread_mutex_lock(&chaines_mutex);
  for (int i = 0; i < nb_chaines; ++i)
  {
    snprintf(buffer, sizeof(buffer), "Nom: %s, Description: %s, Clients: %d/%d\n",
             chaines[i].nom, chaines[i].description, chaines[i].nb_clients, MAX_CLIENTS_PAR_CHAINE);
    send(sock, buffer, strlen(buffer), 0);
  }
  pthread_mutex_unlock(&chaines_mutex);
}

/**
 * Ajoute un nouveau client au tableau
 * @param nouveau_client Pointeur vers le nouveau client à ajouter
 */
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

/**
 * Retire un client du tableau à l'aide de son identifiant
 * @param uid Identifiant unique du client à retirer
 */
void retire_client(int uid)
{
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENT; ++i)
  {
    if (clientsTab[i])
    {
      if (clientsTab[i]->id_client == uid)
      {
        strcpy(clientsTab[i]->nom, "\n");
        clientsTab[i] = NULL;
        break;
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

/**
 * Envoie un message à tous les clients sauf au sender
 * @param s Message à envoyer
 * @param uid Identifiant unique du sender
 */
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

/**
 * Envoie un message à un client spécifique
 * @param s Message à envoyer
 * @param uid Identifiant unique du destinataire
 */
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

/**
 * Vérifie si un nom est déjà utilisé par un autre client
 * @param s Nom à vérifier
 * @return 1 si le nom est déjà utilisé, sinon 0
 */
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

/**
 * Vérifie si le message est un message privé ou une commande /list ou /close
 * @param s Message à vérifier
 * @return 1 si c'est un message privé, 2 si c'est la commande /list, 3 si c'est la commande /close, sinon 0
 */
int check_message(char *s)
{
  char *input_msg = strdup(s);
  char *command = strtok(input_msg, " ");
  command = strtok(NULL, " "); // Extracte le deuxième mot du message

  char *input_msg1 = strdup(s);
  char *command1 = strtok(input_msg1, " ");
  command1 = strtok(NULL, "\n");

  if (strcmp(command, "/mp") == 0) // Vérifie si c'est /mp
  {
    return 1;
  }
  else if (strcmp(command1, "/list") == 0) // Vérifie si c'est /list
  {
    return 2;
  }
  else if (strcmp(command1, "/close") == 0) // Vérifie si c'est /close
  {
    return 3;
  }
  else if (strcmp(command1, "/chaine") == 0) // Vérifie si c'est /chaine
  {
    return 4;
  }
  else
  {
    return 0;
  }
}

/**
 * Extrait le nom du destinataire et le message à envoyer
 * @param s Message contenant le nom du destinataire et le message
 * @return Un tableau de deux chaînes de caractères contenant le nom du destinataire et le message
 */
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

/**
 * Envoie un message privé à un destinataire spécifique
 * @param s Message à envoyer
 * @param destinateur Nom du destinataire
 * @param uid Identifiant unique de l'envoyeur
 */
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
  if (receiver_exist == 0)
  {
    printf("Destinataire n'existe pas\n");
    send_message_id("Destinataire n'existe pas\n", uid);
  }

  pthread_mutex_unlock(&clients_mutex);
}

// ------------------- Modification Aloïs ------------------
// Fonction qui va fermer toutes les socket client et ds la socket serveur
// void close_server()
// {
//   pthread_mutex_lock(&clients_mutex);
//   // Ferme toutes les socket client
//   for (int i = 0; i < MAX_CLIENT; ++i)
//   {
//     if (clientsTab[i])
//     {
//       if (close(clientsTab[i]->sockID) == -1)
//       {
//         perror("Failed to close client socket");
//       }
//       free(clientsTab[i]);
//       clientsTab[i] = NULL;
//     }
//   }
//   pthread_mutex_unlock(&clients_mutex);
//   // Ferme dS (socket serveur)
//   if (dS != -1)
//   {
//     if (close(dS) == -1)
//     {
//       perror("Failed to close server socket");
//     }
//   }
// }
// ----------------- Fin modification Aloïs ----------------

/**
 * Gère la réception de fichiers du client
 * @param client_sock Socket du client
 * @param filename Nom du fichier
 */
void receive_file(int client_sock, char *filename)
{
  printf("filename : %s\n", filename);

  int *file_size = (int *)malloc(sizeof(int));
  FILE *file;
  char *buffer = (char *)malloc(sizeof(char) * 1024);

  // Reçoit le nom du fichier
  filename[strcspn(filename, "\n")] = 0;

  // Reçoit la taille du fichier
  recv(client_sock, file_size, sizeof(file_size), 0);
  printf("File size : %i\n", *file_size);
  *file_size = *file_size - 4;

  // Vérifier si le fichier existe
  if (access(filename, F_OK) != -1)
  {
    printf("Le fichier existe deja.\n");
    return;
  }
  else
  {
    // Le fichier n'existe pas
    printf("Le fichier n'existe pas.\n");
  }

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
  while (total_received < *file_size)
  {
    int bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0)
    {
      break;
    }
    fwrite(buffer, 1, bytes_received, file);
    total_received += bytes_received;
    printf("Reçu: %d/%i octets\n", total_received, *file_size);
  }

  fclose(file);
  printf("Fichier %s reçu avec succès.\n", filename);
}

/**
 * Gère la réception de la taille d'un message
 * @param uid Identifiant unique du client
 * @return Taille du message
 */
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

/**
 * Gère l'envoi de la taille d'un message
 * @param uid Identifiant unique du client
 * @param sortie Chaîne de caractères à envoyer
 */
void taille_send(int uid, char *sortie)
{
  int taille = strlen(sortie);
  printf("Taille envoyé: %d\n", taille);
  send(uid, &taille + 1, sizeof(int), 0);
}

/**
 * Fonction exécutée dans un thread séparé pour chaque client pour gérer la communication
 * @param args Arguments passés au thread
 */
void *new_client(void *args)
{
  char *file = (char *)malloc(sizeof(char) * 256); // fichier entrant
  char message[MSG_SIZE];                          // Message sortant
  char nom[64];                                    // Nom du client
  int state = 0;                                   // Indique si le client quitte la convo
  nb_client++;                                     // Incrémente le nombre de clients
  client *data_client = (client *)args;            // Pointeur de type client pour accéder aux données du client

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
  memset(file, 0, MSG_SIZE);

  while (1)
  {
    if (state) // le client doit quiiter le chat
    {
      break;
    }

    // Réception d'un message du client
    int receive = recv(data_client->sockID, message, MSG_SIZE, 0);
    // int receive_file_int = recv(data_client->sockID_file, file, sizeof(file), 0);

    if (receive > 0)
    { // Messages normaux ou privés
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
        // ------------------- Modification Aloïs ----------------------
        else if (check_message(message) == 2) // c'est la commande /list
        {
          char list_message[MSG_SIZE] = "Liste des utilisateurs connectés:\n";
          pthread_mutex_lock(&clients_mutex);
          for (int i = 0; i < MAX_CLIENT; ++i)
          {
            if (clientsTab[i])
            {
              strcat(list_message, clientsTab[i]->nom);
              strcat(list_message, "\n");
            }
          }
          pthread_mutex_unlock(&clients_mutex);
          send_message_id(list_message, data_client->id_client);
        }
        else if (check_message(message) == 3) // c'est la commande /close
        {
          char *closingMessage = "Le serveur va fermer dans 5 secondes...";
          printf("%s", closingMessage);
          pthread_mutex_lock(&clients_mutex);
          // Envoyer le message à tous les utilisateurs connectés
          for (int i = 0; i < MAX_CLIENT; ++i)
          {
            if (clientsTab[i])
            {
              send_message_id(closingMessage, clientsTab[i]->id_client);
            }
          }
          pthread_mutex_unlock(&clients_mutex);

          // Wait for 5 seconds
          sleep(5);

          // Close the server
          // Assuming you have a function close_server that closes the server
          // close_server();
        }
        else if (check_message(message) == 4) // c'est la commande /chaine
        {
          // Envois des noms des chaines disponibles
          liste_chaines(data_client->sockID);
          char nom_chaine[64];
          // Réception du nom de la chaîne de discussion
          if (recv(data_client->sockID, nom_chaine, sizeof(nom_chaine), 0) > 0)
          {
            pthread_mutex_lock(&chaines_mutex);
            int i;
            for (i = 0; i < nb_chaines; ++i)
            {
              char *input_msg1 = strdup(nom_chaine);
              char *command1 = strtok(input_msg1, " ");
              command1 = strtok(NULL, "\n");
              if (strcmp(chaines[i].nom, command1) == 0 && chaines[i].nb_clients < MAX_CLIENTS_PAR_CHAINE)
              {
                chaines[i].clients[chaines[i].nb_clients++] = data_client;
                data_client->chaine_connectee = &chaines[i];
                sprintf(message, "%s a rejoint la chaîne %s\n", data_client->nom, chaines[i].nom);
                printf("%s", message);
                send_message(message, data_client->id_client);
                break;
              }
            }
            pthread_mutex_unlock(&chaines_mutex);
            if (i == nb_chaines)
            {
              printf("Chaîne non trouvée ou pleine\n");
              send_message_id("Chaîne non trouvée ou pleine\n", data_client->id_client);
              state = 0;
            }
          }
          else
          {
            printf("Erreur réception nom de la chaîne\n");
            state = 0;
          }
        }
        // ------------------- Fin modification Aloïs ------------------
        else
        {
          // Envoyer le message à tous les clients de la même chaîne si le client s'est déjà connecté
          pthread_mutex_lock(&chaines_mutex);
          chaine_discussion *c = data_client->chaine_connectee;
          if (c != NULL)
          {
            for (int j = 0; j < c->nb_clients; ++j)
            {
              printf("%d",c->clients[j]->id_client);
              if (c->clients[j] != data_client)
              {
                send_message(message, c->clients[j]->id_client);
              }
            }
            pthread_mutex_unlock(&chaines_mutex);
          }
          else
          {
            // Message normal
            send_message(message, data_client->id_client);
          }
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
      memset(message, 0, MSG_SIZE);
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

    // if (receive_file_int > 0)
    // {
    //   receive_file(data_client->sockID_file, file);
    // }

    memset(message, 0, MSG_SIZE);
  }

  // Retirer le client de la chaîne
  pthread_mutex_lock(&chaines_mutex);
  if (data_client->chaine_connectee != NULL)
  {
    chaine_discussion *c = data_client->chaine_connectee;
    for (int j = 0; j < c->nb_clients; ++j)
    {
      if (c->clients[j] == data_client)
      {
        for (int k = j; k < c->nb_clients - 1; ++k)
        {
          c->clients[k] = c->clients[k + 1];
        }
        c->nb_clients--;
        break;
      }
    }
  }
  pthread_mutex_unlock(&chaines_mutex);

  close(data_client->sockID); // Fermeture du socket du client
  strcpy(nom, "\n");
  retire_client(data_client->id_client); // Suppression du client de la file d'attente
  free(data_client);                     // Libération de la mémoire
  nb_client--;                           // Décrémentation du nombre total de clients
  sem_post(&sem_client);                 // Libère une place sur le sémaphore
  pthread_detach(pthread_self());
  return NULL;
}

// Code principal du programme
int main(int argc, char *argv[])
{
  // Initialisation du sémaphore avec la valeur MAX_CLIENT
  if (sem_init(&sem_client, 0, MAX_CLIENT) != 0)
  {
    perror("Erreur lors de l'initialisation du sémaphore");
    exit(EXIT_FAILURE);
  }

  init_chaines(); // Initialiser les chaînes de discussion

  int port = atoi(argv[1]);
  int file_port = atoi(argv[2]);

  printf("Port : %i\n", port);
  printf("File port : %i\n", file_port);

  printf("Début programme\n");

  int dS = socket(PF_INET, SOCK_STREAM, 0);
  //---------------------- Modification Aloïs ---------------------------
  if (dS == -1)
  {
    perror("Erreur lors de la création de la socket");
    exit(EXIT_FAILURE); // Arrêter le programme en cas d'échec
  }
  //---------------------- Fin modification Aloïs -----------------------
  printf("Socket Créé\n");

  /* Socket settings */
  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY;
  ad.sin_port = htons(port);

  // Initialisation socket pour les fichiers
  int dS_file = socket(PF_INET, SOCK_STREAM, 0);
  printf("Socket fichier créé\n");

  /* Socket settings */
  struct sockaddr_in ad_file;
  ad_file.sin_family = AF_INET;
  ad_file.sin_addr.s_addr = INADDR_ANY;
  ad_file.sin_port = htons(file_port);

  /* Ignore pipe signals */
  signal(SIGPIPE, SIG_IGN);
  int option = 1;
  if (setsockopt(dS, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0)
  {
    perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
  }
  if (setsockopt(dS_file, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0)
  {
    perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
  }

  /* Bind socket */
  int res = bind(dS, (struct sockaddr *)&ad, sizeof(ad));
  if (res == -1)
  {
    perror("Erreur bind");
    exit(1);
  }
  printf("Socket nommé\n");

  /* Bind socket fichiers */
  int res_file = bind(dS_file, (struct sockaddr *)&ad_file, sizeof(ad_file));
  if (res_file == -1)
  {
    perror("Erreur bind fichier");
    exit(1);
  }
  printf("Socket fichier nommé\n");

  /* Listen */
  listen(dS, 7);
  printf("Mode écoute\n");

  listen(dS_file, 7);
  printf("Mode écoute fichier\n");

  struct sockaddr_in sock_client;
  struct sockaddr_in sock_client_file;
  pthread_t tid;

  while (1)
  {
    socklen_t lg = sizeof(sock_client);
    socklen_t lg_file = sizeof(sock_client_file);

    // Attendre qu'une place se libère
    sem_wait(&sem_client);

    client *newClient = (client *)malloc(sizeof(client));

    int request = accept(dS, (struct sockaddr *)&sock_client, &lg); // on attend une connexion
    printf("Demande connexion\n");
    if (nb_client < MAX_CLIENT)
    { // vérifie si on atteint pas le nb max
      if (request > 0)
      { // on vérifie que la connexion s'est bien faite
        printf("Connexion possible\n");

        // Alloue de la mémoire pour un nouveau client, puis initialise ses données
        newClient->address = sock_client;
        newClient->sockID = request;
        newClient->id_client = uid++;
      }
      else
      {
        printf("Erreur connexion\n");
        continue;
      }

      int request_file = accept(dS_file, (struct sockaddr *)&sock_client_file, &lg_file);
      if (request_file > 0)
      {
        newClient->address_file = sock_client_file;
        newClient->sockID_file = request_file;

        // Ajoute le nouveau client à la file d'attente des clients
        ajout_client(newClient);
        // Crée un thread pour gérer la communication avec le client
        int result = pthread_create(&tid, NULL, &new_client, (void *)newClient);
        //---------------------- Modification Aloïs ---------------------------
        if (result != 0)
        {
          fprintf(stderr, "Erreur lors de la création du thread: %s\n", strerror(result));
          exit(EXIT_FAILURE);
        }
        //---------------------- Fin modification Aloïs -----------------------
        sleep(1);
      }
      else
      {
        printf("Erreur accept fichier\n");
        close(request);
        free(newClient);
        continue;
      }
    }
    else
    {
      printf("Nombre maximal de client déjà atteint, request refusé\n");
      close(request);
      // Libère le sémaphore si la connexion est refusée
      sem_post(&sem_client);
    }
  }

  // Libère les ressources
  sem_destroy(&sem_client);
  close(dS);
  close(dS_file);
  printf("Fin du programme\n");

  return 0;
}