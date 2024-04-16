#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MSG_SIZE 500

int client_socket;

// Fonction pour recevoir et afficher les messages du serveur
void *receive_message(void *arg) {
    while (1) {
        char message[MSG_SIZE];
        int bytes_received = recv(client_socket, message, MSG_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected.\n");
            break;
        }
        message[bytes_received] = '\0';
        printf("< %s", message);
    }
    return NULL;
}

// Fonction pour la saisie et l'envoi de messages au serveur
void *send_message(void *arg) {
    while (1) {
        char message[MSG_SIZE];
        printf("> ");
        fgets(message, MSG_SIZE, stdin);
        send(client_socket, message, strlen(message), 0);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(argv[1]);
    server_address.sin_port = htons(atoi(argv[2]));

    if (connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
        perror("connect");
        close(client_socket);
        return EXIT_FAILURE;
    }

    printf("Connected to server. Start typing messages...\n");

    // Création des threads pour la réception et l'envoi de messages
    pthread_t receive_tid, send_tid;
    pthread_create(&receive_tid, NULL, receive_message, NULL);
    pthread_create(&send_tid, NULL, send_message, NULL);

    // Attente de la fin des threads
    pthread_join(receive_tid, NULL);
    pthread_join(send_tid, NULL);

    close(client_socket);
    return EXIT_SUCCESS;
}
