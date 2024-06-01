/**
 * @file client.c
 * @brief Programme client de chat multi-client
 */

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
//------------------------------------------
int flag = 0; // Gestion du signal
int dS = 0;   // Descripteur de fichier du socket
int dS_file = 0;   // Descripteur de fichier du socket
char nom[32]; // Nom du client

// pareil que pour "serveur.sh", faites "./client.sh", ca recupere automatiquement votre IP (pour les tests)
// et ca attribue tout seul un nouveau port

/**
 * @brief Gère le signal lorsque le client appuie sur Ctrl+C et quitte le programme.
 * 
 * @param sig La valeur du signal.
 */
void catch_ctrl_c_and_exit(int sig)
{
  flag = 1;
}

/**
 * @brief Lit et affiche le contenu du fichier "MANUEL.txt".
 */
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

/**
 * @brief Affiche les fichiers disponibles dans le répertoire client_folder.
 */
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

/**
 * @brief Envoie un fichier au serveur.
 */
void send_file() {

  char* filename = (char*)malloc(sizeof(char)*256);
  FILE *file;

  // Affiche les fichiers qu'on peut envoyer
  printf("Fichiers disponibles:\n");
  list_files();

  // L'utilisatuer entre le nom du fichier
  printf("Spécifiez le nom du fichier à envoyer: ");
  fgets(filename, 256, stdin);
  filename[strcspn(filename, "\n")] = 0;
  printf("Nom du fichier : %s\n", filename);

  send(dS_file, filename, strlen(filename), 0);

  char* filepath = (char*)malloc(sizeof(char)*512);

  // Etablie le lien complet du fichier
  snprintf(filepath, sizeof(char)*512, "%s/%s", FILE_DIR, filename);
  printf("filepath : %s\n", filepath);

  // Ouverture du fichier
  file = fopen(filepath, "rb");

  if (!file)
  {
    perror("Fichier ne peut être ouvert\n");
    return;
  }

  // Obteint la taille du fichier
   if (fseek(file, 0, SEEK_END) != 0) {
        perror("Erreur lors de la recherche de la fin du fichier");
        fclose(file);
        return;
    }

    // Obtenir la position actuelle du pointeur de fichier (taille du fichier)
    int file_size = ftell(file);
    if (file_size == -1) {
        perror("Erreur lors de l'obtention de la taille du fichier");
        fclose(file);
        return;
    }

    // Revenir au début du fichier
    if (fseek(file, 0, SEEK_SET) != 0) {
        perror("Erreur lors du retour au début du fichier");
        fclose(file);
        return;
    }
  printf("File size : %i\n", file_size);

  // Envoie la taille du fichier
  send(dS_file, &file_size, sizeof(int), 0);

  // Envoie le contenu du fichier
  char buffer[1024];
  size_t bytes_read;
  ssize_t bytes_sent;
  size_t total_bytes_sent;

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
      total_bytes_sent = 0;
      while (total_bytes_sent < bytes_read) {
          bytes_sent = send(dS_file, buffer + total_bytes_sent, bytes_read - total_bytes_sent, 0);
          if (bytes_sent == -1) {
              perror("Erreur pendant l'envoi du fichier");
              fclose(file);
              return;
          } else {
            printf("Envoyé: %zu/%i octets\n", total_bytes_sent, file_size);
          }
          total_bytes_sent += bytes_sent;
      }
  }

  // Ferme le fichier
  fclose(file);
  printf("Fichier envoyé avec succès.\n");
}

/**
 * @brief Gère la réception de messages du serveur.
 */
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

/**
 * @brief Gère l'envoi de messages au serveur.
 */
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

//----------------------------------------------------------
// Gère la réception de la taille
/**
 * @brief Gère la réception de la taille du fichier depuis le serveur.
 * 
 * @return int La taille du fichier.
 */
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

/**
 * @brief Envoie la taille du message sortant au serveur.
 * 
 * @param sortie Le message sortant.
 */
// Gère l'envoi de la taille
void taille_send(char *sortie)
{
  int taille = strlen(sortie);
  taille = htonl(taille);
  printf("Taille envoyé: %d\n", htonl(taille));
  send(dS, &taille, sizeof(int), 0);
}
//-------------------------------------------------------------------

int main(int argc, char *argv[]) {

  int port = atoi(argv[2]);
  int file_port = atoi(argv[3]);

  printf("Port : %i\n", port);
  printf("File port : %i\n", file_port);

  dS = socket(AF_INET, SOCK_STREAM, 0);
  dS_file = socket(AF_INET, SOCK_STREAM, 0);

  printf("Début programme\n");
  if (dS == -1) {
    perror("Erreur lors de la création de la socket");
    exit(EXIT_FAILURE);
  }
  if (dS_file == -1) {
    perror("Erreur lors de la création de la socket file");
    exit(EXIT_FAILURE);
  }
  printf("Sockets Créé\n");

  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET, argv[1], &(aS.sin_addr));
  aS.sin_port = htons(port);
  socklen_t lgA = sizeof(struct sockaddr_in);

  struct sockaddr_in aS_file;
  aS_file.sin_family = AF_INET;
  inet_pton(AF_INET, argv[1], &(aS_file.sin_addr));
  aS_file.sin_port = htons(file_port);
  socklen_t lgA_file = sizeof(struct sockaddr_in);

  /* Signal */
  signal(SIGINT, catch_ctrl_c_and_exit);

  if (connect(dS, (struct sockaddr *)&aS, lgA) == -1) // tentative connexion au serveur
  {
    printf("Pas de serveur trouvé\n"); // echec
    exit(1);
  }
  printf("Socket fichier connecté\n"); // reussite

  if (connect(dS_file, (struct sockaddr *)&aS_file, lgA_file) == -1) // tentative connexion au serveur
  {
    printf("Pas de serveur fichier trouvé\n"); // echec
    exit(1);
  }
  printf("Socket fichier connecté\n"); // reussite

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