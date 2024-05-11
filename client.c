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
int flag = 0;        // Gestion du signal
int dS = 0; // Descripteur de fichier du socket
char nom[32];        // Nom du client

// pareil que pour "serveur.sh", faites "./client.sh", ca recupere automatiquement votre IP (pour les tests)
// et ca attribue tout seul un nouveau port

// Client appuie sur Ctrl+C pour terminer le programme
void catch_ctrl_c_and_exit(int sig)
{
  flag = 1;
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
    if (strcmp(msg, "fin") == 0)
    {
      break;
    }
    else
    {
      sprintf(buffer, "%s: %s\n", nom, msg);
      send(dS, buffer, strlen(buffer), 0);
      printf("Test\n");
    }
    memset(msg, 0, MSG_SIZE);
    memset(buffer, 0, MSG_SIZE + 32);
  }
  catch_ctrl_c_and_exit(2);
}

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
  send(dS, nom, 32, 0); // Envoie le nom au serveur

  printf("*** Bienvenue ***\n");

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