#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>

//------------------------------------------
#define MSG_SIZE 100               // Taille max du message
#define FILE_DIR "./client_folder" // Dossier clients pour stocker les fichiers

#define PORT_FILE "1234"
//------------------------------------------
int flag = 0;       // Gestion du signal
int dS = 0;         // Descripteur de fichier du socket
int dS_fichier = 0; // Descripteur de fichier du socket destiné aux fichiers
char nom[32];       // Nom du client

// pareil que pour "serveur.sh", faites "./client.sh", ca recupere automatiquement votre IP (pour les tests)
// et ca attribue tout seul un nouveau port

// Client appuie sur Ctrl+C pour terminer le programme
void catch_ctrl_c_and_exit(int sig)
{
  flag = 1;
}

// Lecture du fichier texte
void readFile()
{
  char *filename = "MANUEL.txt";
  FILE *fp = fopen(filename, "r");

  if (fp == NULL)
  {
    printf("Error: could not open file %s", filename);
  }

  // reading line by line, max 256 bytes
  const unsigned MAX_LENGTH = 256;
  char buffer[MAX_LENGTH];

  while (fgets(buffer, MAX_LENGTH, fp))
    printf("%s", buffer);

  printf("\n");

  // close the file
  fclose(fp);
}

//-----------------MODIFICATION POOMEDY------------------------------
// Fonctions qui affiche les fichiers disponible dans le dossier client
void list_files()
{
  DIR *d;
  struct dirent *dir;
  d = opendir(FILE_DIR);
  if (d)
  {
    int i = 1;
    while ((dir = readdir(d)) != NULL)
    {
      if (dir->d_type == DT_REG)
      {
        printf("%d: %s\n", i, dir->d_name);
        i++;
      }
    }
    closedir(d);
  }
  else
  {
    perror("Could not open directory");
  }
}

// Fonction qui envoie le fichier au serveur
void send_file()
{
  char filepath[512];
  char filename[256];
  char file_size_str[16];
  int file_size;
  FILE *file;

  // Affiche les fichiers qu'on peut envoyer
  printf("Fichiers disponibles:\n");
  list_files();

  // L'utilisatuer entre le nom du fichier
  printf("Spécifiez le nom du fichier à envoyer: ");
  fgets(filename, 256, stdin);
  filename[strcspn(filename, "\n")] = 0;

  // Etablie le lien complet du fichier
  snprintf(filepath, sizeof(filepath), "%s/%s", FILE_DIR, filename);

  // Ouverture du fichier
  file = fopen(filepath, "rb");
  if (!file)
  {
    perror("Fichier ne peut être ouvert\n");
    return;
  }

  // Obteint la taille du fichier
  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Envoie le nom du fichier au serveur
  send(dS_fichier, filename, strlen(filename), 0);
  send(dS_fichier, "\n", 1, 0);

  // Envoie la taille du fichier
  snprintf(file_size_str, sizeof(file_size_str), "%d", file_size);
  send(dS_fichier, file_size_str, strlen(file_size_str), 0);
  send(dS_fichier, "\n", 1, 0);

  // Envoie le contenu du fichier
  char buffer[1024];
  int bytes_read;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
  {
    if (send(dS_fichier, buffer, bytes_read, 0) == -1)
    {
      perror("Erreur pendant l'envoie du fichier");
      fclose(file);
      return;
    }
  }

  // Ferme le fichier
  fclose(file);
  printf("Fichier envoyé avec succès.\n");
}
//-------------------------------------------------------------------

// Gère la réception de messages
void msg_recv()
{
  char msg[MSG_SIZE] = {};

  while (1)
  {
    int response = recv(dS, msg, MSG_SIZE, 0);
    if (response > 0)
    {
      printf("%s", msg);
    }
    else if (response == 0)
    {
      break;
    }
    else
    {
      // -1
    }
    memset(msg, 0, sizeof(msg));
  }
}

// Gère l'envoi de messages
void msg_send()
{
  char msg[MSG_SIZE] = {};
  char buffer[MSG_SIZE + 64] = {};

  while (1)
  {
    printf("%s", "> ");
    fgets(msg, MSG_SIZE, stdin);
    int i;
    for (i = 0; i < MSG_SIZE; i++)
    { // trim \n
      if (msg[i] == '\n')
      {
        msg[i] = '\0';
        break;
      }
    }
    if (strcmp(msg, "/exit") == 0)
    {
      break;
    }
    else if (strcmp(msg, "/help") == 0)
    {
      readFile();
    }
    else if (strcmp(msg, "/file") == 0)
    {
      sprintf(buffer, "%s: %s\n", nom, msg);
      send(dS, buffer, strlen(buffer), 0);
      send_file();
    }
    else
    {
      sprintf(buffer, "%s: %s\n", nom, msg);
      send(dS, buffer, strlen(buffer), 0);
    }
    memset(msg, 0, MSG_SIZE);
    memset(buffer, 0, MSG_SIZE + 32);
  }
  catch_ctrl_c_and_exit(2);
}

//-----------------MODIFICATION POOMEDY------------------------------
// Gère la réception de la taille
int taille_recv()
{
  int taille;
  int response = recv(dS, &taille, sizeof(int), 0);
  printf("Taille reçu: %d\n", taille);
  if (response > 0)
  {
    taille = ntohl(taille);
    return taille + 1;
  }
}

// Gère l'envoi de la taille
void taille_send(char *sortie)
{
  int taille = strlen(sortie);
  taille = htonl(taille);
  printf("Taille envoyé: %d\n", htonl(taille));
  send(dS, &taille, sizeof(int), 0);
}
//-------------------------------------------------------------------

int main(int argc, char *argv[])
{
  /* ------------------------SOCKET MESSAGES------------------------------------*/
  dS = socket(AF_INET, SOCK_STREAM, 0);
  printf("Début programme\n");
  if (dS == -1)
  {
    perror("Erreur lors de la création de la socket");
    exit(EXIT_FAILURE);
  }
  printf("Socket Créé\n");

  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET, argv[1], &(aS.sin_addr));
  aS.sin_port = htons(atoi(argv[2]));
  socklen_t lgA = sizeof(struct sockaddr_in);

  /* Signal */
  signal(SIGINT, catch_ctrl_c_and_exit);

  if (connect(dS, (struct sockaddr *)&aS, lgA) == -1) // tentative connexion au serveur
  {
    printf("Pas de serveur trouvé\n"); // echec
    exit(1);
  }
  printf("Socket Connecté\n"); // reussite
  /* ------------------------SOCKET MESSAGES------------------------------------*/

  /* ------------------------SOCKET FICHIERS------------------------------------*/
  dS_fichier = socket(AF_INET, SOCK_STREAM, 0);
  if (dS_fichier == -1)
  {
    perror("Erreur lors de la création de la socket fichier");
    exit(EXIT_FAILURE);
  }
  printf("Socket fichier Créé\n");

  struct sockaddr_in aS_fichier;
  aS_fichier.sin_family = AF_INET;
  inet_pton(AF_INET, argv[1], &(aS_fichier.sin_addr));
  aS_fichier.sin_port = htons(atoi(PORT_FILE));
  socklen_t lgA_fichier = sizeof(struct sockaddr_in);

  /* Signal */
  signal(SIGINT, catch_ctrl_c_and_exit);

  if (connect(dS_fichier, (struct sockaddr *)&aS_fichier, lgA_fichier) == -1) // tentative connexion au serveur
  {
    printf("Pas de serveur trouvé\n"); // echec
    exit(1);
  }
  printf("Socket Fichier Connecté\n"); // reussite
  /* ------------------------SOCKET FICHIERS------------------------------------*/

  // Validation du pseudo client
  while (1)
  {
    char msg_recv[32];
    //-----------------MODIFICATION POOMEDY------------------------------
    printf("Veuillez entrer votre nom: ");
    fgets(nom, 32, stdin);
    int i;
    for (i = 0; i < strlen(nom); i++)
    { // trim \n
      if (nom[i] == '\n')
      {
        nom[i] = '\0';
        break;
      }
    }

    // Envoie le nom au serveur
    send(dS, nom, MSG_SIZE, 0);

    // Réception du message du serveur
    memset(msg_recv, 0, MSG_SIZE);
    recv(dS, msg_recv, MSG_SIZE, 0); // Attends la validation du serveur
    printf("%s\n", msg_recv);
    if (strcmp(msg_recv, "Nom correcte") == 0)
    {
      break;
    }

    memset(msg_recv, 0, MSG_SIZE);
  }
  //-------------------------------------------------------------------

  printf("*** Bienvenue ***\n");
  printf("*** Tapez /help pour voir toutes les commandes ***\n");

  pthread_t send_msg_thread;
  if (pthread_create(&send_msg_thread, NULL, (void *)msg_send, NULL) != 0)
  {
    printf("ERREUR: pthread\n");
    return EXIT_FAILURE;
  }

  pthread_t recv_msg_thread;
  if (pthread_create(&recv_msg_thread, NULL, (void *)msg_recv, NULL) != 0)
  {
    printf("ERREUR: pthread\n");
    return EXIT_FAILURE;
  }

  while (1)
  {
    if (flag)
    {
      break;
    }
  }

  shutdown(dS, 2);
  printf("Fin du programme\n");
}