#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>

#define PORT 8080
#define THREAD_POOL_SIZE 4
#define MAX_CLIENTS 50

int workers_data[THREAD_POOL_SIZE][4]; // 0: is_free, 1: client_socket, 2: first_num, 3: second_num 
int data_array[10000][3]; // 0: client_socket, 1: first_num, 2: second_num 
int main_index;
int master_index;


int timeout = 5; // timeout in seconds
fd_set readset;
struct timeval tv;

struct client_request
{
    int client_socket;
    struct sockaddr_in client_address;
};

typedef struct node
{
    int first_num;
    int second_num;
    int priority;
    struct node *next;
} node;

node *new_node(int first_num, int second_num, int priority)
{
    node *temp = (node *)malloc(sizeof(node));
    temp->first_num = first_num;
    temp->second_num = second_num;
    temp->priority = priority;
    temp->next = NULL;
    return temp;
}

void dequeue(node **head, int *first_num, int *second_num)
{
    node *temp = *head;
    *first_num = temp->first_num;
    *second_num = temp->second_num;
    (*head) = (*head)->next;
    free(temp);
}

void enqueue(node **head, int first_num, int second_num, int priority)
{
    node *start = (*head);
    node *temp = new_node(first_num, second_num, priority);
    if ((*head)->priority > priority)
    {
        temp->next = *head;
        (*head) = temp;
    }
    else
    {
        while (start->next != NULL && start->next->priority <= priority)
        {
            start = start->next;
        }
        temp->next = start->next;
        start->next = temp;
    }
}

int is_empty(node **head)
{
    return (*head) == NULL;
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
        
        printf("client %d accepted:\n", client_socket);
        int answer;
        FD_ZERO(&readset);
        FD_SET(client_socket, &readset);
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        int retval = select(client_socket + 1, &readset, NULL, NULL, &tv);
        if (retval == -1) {
            printf("Error receiving data from client %d:\n", client_socket);
        } else if (retval) {
            recv(client_socket, &answer, sizeof(answer), 0);
            printf("Answer of received from client %d:\n", client_socket);
            printf("Answer is : %d\n", answer);
        } else {
            printf("Timeout happened for client %d:\n", client_socket);
        }
    }
    return 0;
}