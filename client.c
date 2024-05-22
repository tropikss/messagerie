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

//------------------------------------------
#define MSG_SIZE 100 // Taille max du message
//------------------------------------------
int flag = 0; // Gestion du signal
int dS = 0;   // Descripteur de fichier du socket
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

//***********************OUTDATE**************************
// typedef struct
// {
//   int type;
//   int taille;
//   union
//   {
//     int i;
//     char msg[MSG_SIZE];
//   } info;
// } data;

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
    return taille+1;
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

  //***********************OUTDATE**************************
  // pthread_t sender;
  // pthread_t receiver;
  // int state = 1;
  // data recv_data;

  // if (pthread_create(&sender, NULL, msg_send, (void *)&dS) != 0)
  // {
  //   printf("erreur thread sender\n");
  // }

  // if (pthread_create(&receiver, NULL, msg_recv, (void *)&dS) != 0)
  // {
  //   printf("erreur thread receiver\n");
  // }

  // if (pthread_join(receiver, NULL) != 0)
  // {
  //   printf("Erreur lors de l'attente du thread 'receiver'");
  //   exit(EXIT_FAILURE);
  // }

  // if (pthread_cancel(sender) != 0)
  // {
  //   perror("Erreur lors de l'annulation du thread 'sender'");
  //   exit(EXIT_FAILURE);
  // }

  while (1)
  {
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

    // 1 - Envoie de la taille du message
    taille_send(nom);

    // 2 - Envoie le nom au serveur
    int taille = strlen(nom);
    send(dS, nom, taille*sizeof(char), 0);

    // 3 - Réception la taille du message
    taille = taille_recv();

    // 4 - Réception du message du serveur
    char msg_recv = (char *)malloc(taille * sizeof(char));
    recv(dS, msg_recv, taille*sizeof(char), 0); // Attends la validation du serveur
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