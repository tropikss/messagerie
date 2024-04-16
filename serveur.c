#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MSG_SIZE 500
#define MAX_CLIENTS 2

int client_sockets[MAX_CLIENTS];
pthread_t client_threads[MAX_CLIENTS];

// Fonction pour relayer les messages entre les clients
void *relay_messages(void *arg) {
    int sender_index = *((int *) arg);
    int receiver_index = (sender_index + 1) % MAX_CLIENTS;

    char message[MSG_SIZE];
    int bytes_received;

    while ((bytes_received = recv(client_sockets[sender_index], message, MSG_SIZE, 0)) > 0) {
        message[bytes_received] = '\0';

        send(client_sockets[receiver_index], message, strlen(message), 0);
    }

    // Si le client se déconnecte, fermer le socket et arrêter le thread
    close(client_sockets[sender_index]);
    client_sockets[sender_index] = -1;
    printf("Client %d disconnected.\n", sender_index + 1);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(atoi(argv[1]));

    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
        perror("bind");
        close(server_socket);
        return EXIT_FAILURE;
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Server listening on port %s...\n", argv[1]);

    // Accepter les connexions des clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        client_sockets[i] = accept(server_socket, (struct sockaddr *) &client_address, &client_address_len);
        if (client_sockets[i] == -1) {
            perror("accept");
            close(server_socket);
            return EXIT_FAILURE;
        }
        printf("Client %d connected.\n", i + 1);

        // Créer un thread pour chaque client
        int *client_index_ptr = malloc(sizeof(int));
        *client_index_ptr = i;
        pthread_create(&client_threads[i], NULL, relay_messages, client_index_ptr);
    }

    // Attendre la fin de tous les threads clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        pthread_join(client_threads[i], NULL);
    }

    close(server_socket);
    return EXIT_SUCCESS;
}
