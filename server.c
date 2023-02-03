#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define THREAD_POOL_SIZE 4

struct client_request
{
    int client_socket;
    struct sockaddr_in client_address;
};

// Function to be executed by each worker thread
void *worker_thread(void *arg)
{
    int client_socket = *((int *)arg);
    char buffer[1024];
    int read_bytes = read(client_socket, buffer, sizeof(buffer));
    buffer[read_bytes] = '\0';
    printf("Received from client: %s\n", buffer);

    const char *response = "Hello from server";
    write(client_socket, response, strlen(response));

    close(client_socket);
    pthread_exit(NULL);
}

// Function to be executed by the master thread
void *master_thread(void *arg)
{
    struct client_request *requests = (struct client_request *)arg;
    int thread_count = 0;
    pthread_t threads[THREAD_POOL_SIZE];

    while (1)
    {
        // Wait for a client request to be added to the queue
        while (requests[0].client_socket == 0)
        {
            sleep(1);
        }

        // Create a worker thread for the client request
        int client_socket = requests[0].client_socket;
        if (pthread_create(&threads[thread_count], NULL, worker_thread, &client_socket) != 0)
        {
            perror("Error creating worker thread");
        }
        thread_count = (thread_count + 1) % THREAD_POOL_SIZE;

        // Shift the queue to the left to remove the processed client request
        int i;
        for (i = 0; i < MAX_CLIENTS - 1; i++)
        {
            requests[i] = requests[i + 1];
        }
        memset(&requests[MAX_CLIENTS - 1], 0, sizeof(struct client_request));
    }

    pthread_exit(NULL);
}

void *master_thread(void *requests_ptr) {
    struct client_request *requests = (struct client_request *) requests_ptr;

    pthread_t worker_thread_id[MAX_CLIENTS];
    memset(worker_thread_id, 0, sizeof(worker_thread_id));

    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (pthread_create(&worker_thread_id[i], NULL, worker_thread, requests) != 0) {
            perror("Error creating worker thread");
        }
    }

    int next_request = 0;
    while (1) {
        if (requests[next_request].client_socket != 0) {
            pthread_join(worker_thread_id[next_request % MAX_CLIENTS], NULL);
            next_request = (next_request + 1) % MAX_CLIENTS;
        }
    }

    return NULL;
}

int main(int argc, char const *argv[])
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Error creating socket");
        return 1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Error binding socket");
        return 1;
    }
    if (listen(server_socket, MAX_CLIENTS) < 0)
    {
        perror("Error listening on socket");
        return 1;
    }
    struct client_request requests[MAX_CLIENTS];
    memset(requests, 0, sizeof(requests));

    pthread_t master_thread_id;
    if (pthread_create(&master_thread_id, NULL, master_thread, requests) != 0)
    {
        perror("Error creating master thread");
        return 1;
    }

    while (1)
    {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket < 0)
        {
            perror("Error accepting connection");
            continue;
        }

        int i;
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (requests[i].client_socket == 0)
            {
                requests[i].client_socket = client_socket;
                requests[i].client_address = client_address;
                break;
            }
        }
    }
    return 0;
}