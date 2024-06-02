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
void send_file() {

  char* filename = (char*)malloc(sizeof(char)*256);
  FILE *file;

  // Affiche les fichiers qu'on peut envoyer
  printf("Fichiers disponibles:\n");
  list_files();

  // L'utilisateur entre le nom du fichier
  printf("Spécifiez le nom du fichier à envoyer: ");
  fgets(filename, 256, stdin);
  filename[strcspn(filename, "\n")] = 0;
  printf("Nom du fichier : %s\n", filename);

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

  // Envoie l'info au serveur que l'on veut lui envoyer un fichier
  int tosend = 1;
  send(dS_file, &tosend, sizeof(int), 0);

  // Envoie la taille du nom du fichier
  int filename_length = strlen(filename);
  printf("Taille du nom du fichier : %i\n", filename_length);

  send(dS_file, &filename_length, sizeof(int), 0);

  // Envoie le nom du fichier
  send(dS_file, filename, filename_length, 0);

  int response;
  recv(dS_file, &response, sizeof(int), 0);
  if(response) {
    printf("%s\n","Nom ok");
  } else {
    printf("%s\n", "Nom deja existant");

    int response = 0;

    // Demande à l'utilisateur s'il veut remplacer le fichier
    char resp[3];  // Tampon pour la réponse de l'utilisateur
    printf("Voulez-vous remplacer le fichier ? (Y/N) : ");
    if (fgets(resp, sizeof(resp), stdin) != NULL) {
        // Enlever le caractère de nouvelle ligne, s'il existe
        size_t len = strlen(resp);
        if (len > 0 && resp[len-1] == '\n') {
            resp[len-1] = '\0';
        }

        // Vérifier la réponse de l'utilisateur
        if (strcmp(resp, "Y") == 0 || strcmp(resp, "y") == 0) {
            response = 1;
            printf("Le fichier sera remplacé.\n");
            // Ajouter le code pour remplacer le fichier ici
        } else if (strcmp(resp, "N") == 0 || strcmp(resp, "n") == 0) {
            printf("Le fichier ne sera pas remplacé.\n");
            // Ajouter le code pour ne pas remplacer le fichier ici
        } else {
            printf("Réponse non valide. Le fichier ne sera pas remplacé.\n");
            // Ajouter le code pour gérer une réponse non valide ici
        }

        send(dS_file, &response, sizeof(int), 0);
        if(!response) {
          return;
        }
    } else {
        fprintf(stderr, "Erreur de lecture de la réponse.\n");
        return;
    }
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

void receive_file() {

  int file_size;
  FILE *file;
  char *buffer = (char *)malloc(sizeof(char) * 1024);
  char filename[256];

  // L'utilisateur entre le nom du fichier
  printf("Spécifiez le nom du fichier à envoyer: ");
  fgets(filename, 256, stdin);
  filename[strcspn(filename, "\n")] = 0;
  printf("Nom du fichier : %s\n", filename);

  const char *base_path = "./client_folder/";
    
    // Allouer un tampon suffisamment grand pour contenir le chemin complet
    char *full_path = malloc(strlen(base_path) + strlen(filename) + 1);
    if (full_path == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire\n");
        return;
    }
    
    // Copier la chaîne de base dans le tampon
    strcpy(full_path, base_path);
    
    // Concaténer la chaîne filename au tampon
    strcat(full_path, filename);
    
    // Afficher le chemin complet
    printf("Chemin : %s\n", full_path);

  // Vérifier si le fichier existe
  if (access(full_path, F_OK) != -1) {
    printf("Le fichier existe deja.\n");

    int response = 0;

    // Demande à l'utilisateur s'il veut remplacer le fichier
    char resp[3];  // Tampon pour la réponse de l'utilisateur
    printf("Voulez-vous remplacer le fichier ? (Y/N) : ");
    if (fgets(resp, sizeof(resp), stdin) != NULL) {
        // Enlever le caractère de nouvelle ligne, s'il existe
        size_t len = strlen(resp);
        if (len > 0 && resp[len-1] == '\n') {
            resp[len-1] = '\0';
        }

        // Vérifier la réponse de l'utilisateur
        if (strcmp(resp, "Y") == 0 || strcmp(resp, "y") == 0) {
            response = 1;
            printf("Le fichier sera remplacé.\n");
            // Ajouter le code pour remplacer le fichier ici
        } else if (strcmp(resp, "N") == 0 || strcmp(resp, "n") == 0) {
            printf("Le fichier ne sera pas remplacé.\n");
            // Ajouter le code pour ne pas remplacer le fichier ici
        } else {
            printf("Réponse non valide. Le fichier ne sera pas remplacé.\n");
            // Ajouter le code pour gérer une réponse non valide ici
        }
    } else {
        fprintf(stderr, "Erreur de lecture de la réponse.\n");
        return;
    }

    printf("response : %d\n", response);
    if(!response) {
      return;
    }
  } else {
    // Le fichier n'existe pas
    int response = 1;

    printf("Le fichier n'existe pas.\n");
  }

  // Envoie au serveur que le client est prêt à recevoir un fichier
  // 0 = client prêt à recevoir un fichier
  int tosend = 0;
  send(dS_file, &tosend, sizeof(int), 0);

  int name_size = strlen(filename);
  send(dS_file, &name_size, sizeof(int), 0);
  printf("name_size : %d\n", name_size);

  send(dS_file, filename, name_size, 0);
  printf("filename : %s\n", filename);

  int response;
  recv(dS_file, &response, sizeof(int), 0);

  if(response == 1) {
    printf("Nom correcte\n");
  } else {
    printf("Le fichier n'existe pas.\n");
    return;
  }

  // Reçoit la taille du fichier
  recv(dS_file, &file_size, sizeof(int), 0);
  printf("File size : %i\n", file_size);

  // Ouverture du fichier pour l'écriture
  char filepath[512];
  snprintf(filepath, sizeof(filepath), "./client_folder/%s", filename);
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
    int bytes_received = recv(dS_file, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
      break;
    }
    fwrite(buffer, 1, bytes_received, file);
    total_received += bytes_received;
    printf("Reçu: %d/%i octets\n", total_received, file_size);
  }

  fclose(file);
  printf("Fichier %s reçu avec succès.\n", filename);
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
      int choice;
      printf("Voulez-vous envoyer (1) ou recevoir (2) un fichier ? ");
      fgets(buffer, sizeof(buffer), stdin);
      sscanf(buffer, "%d", &choice);

      if (choice == 1) {
        send_file();
      } else if (choice == 2) {
        receive_file();
      } else {
        printf("Choix invalide.\n");
      }
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
  printf("Socket client connecté\n"); // reussite

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