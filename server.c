#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define PORT 8080
#define THREAD_POOL_SIZE 4
#define MAX_CLIENTS 50
#define MEM_SIZE 10000

int timeout = 5; // timeout in seconds
fd_set readset;
struct timeval tv;

int workers_data[THREAD_POOL_SIZE][4]; // 0: is_free, 1: client_socket, 2: first_num, 3: second_num
int data_array[MEM_SIZE][3];		   // 0: client_socket, 1: first_num, 2: second_num
int main_index;
int master_index;

typedef struct node
{
	int first_num;
	int second_num;
	int client_socket;
	int priority;
	struct node *next;
} node;

node *new_node(int first_num, int second_num, int client_socket, int priority)
{
	node *temp = (node *)malloc(sizeof(node));
	temp->first_num = first_num;
	temp->second_num = second_num;
	temp->priority = priority;
	temp->client_socket = client_socket;
	temp->next = NULL;
	return temp;
}

void dequeue(node **head, int *first_num, int *second_num, int* client_socket)
{
	node *temp = *head;
	*first_num = temp->first_num;
	*second_num = temp->second_num;
	*client_socket = temp->client_socket;
	(*head) = (*head)->next;
	free(temp);
}

void enqueue(node **head, int first_num, int second_num, int client_socket, int priority)
{
	node *start = (*head);
	node *temp = new_node(first_num, second_num, client_socket, priority);
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

void *worker_thread(void *args)
{
	int id = *((int *)args);
	workers_data[id][0] = 1;

    while (1)
    {
        if (workers_data[id][0] == 0)
        {
            int num1 = workers_data[id][1];
            int num2 = workers_data[id][2];
			int client_socket = workers_data[id][3];
            int res = num1 + num2;
            sleep(10); // for thest priority queue

			char message[100];
			sprintf(message, "Done: %d + %d = %d\n", num1, num2, res);

			send(client_socket, message, strlen(message), 0);
			
			sprintf(message, "Done for client_socket %d : %d + %d = %d", client_socket, num1, num2, res);
			printf("%s\n", message);

            workers_data[id][0] = 1;
        }
    }
	return NULL;
}

void *master_thread(void *arg)
{
	int fcfs_counter = 0;
	pthread_t worker_thread_id[THREAD_POOL_SIZE];
	memset(worker_thread_id, 0, sizeof(worker_thread_id));

	int i;
	for (i = 0; i < THREAD_POOL_SIZE; i++)
	{
		int *id = (int *)malloc(sizeof(int));
        *id = i;
		if (pthread_create(&worker_thread_id[i], NULL, worker_thread, id) != 0)
		{
			printf("Error creating worker thread with id %d\n", i);
			i--;
		}
	}

	node *pq = NULL;
	while (1)
	{
		for (int i = 0; i < THREAD_POOL_SIZE; i++)
		{ 
			if (main_index != master_index)
			{
				int client_socket = data_array[master_index % MEM_SIZE][0];
				int num1 = data_array[master_index % MEM_SIZE][1];
				int num2 = data_array[master_index % MEM_SIZE][2];
				master_index++;
				if (is_empty(&pq))
				{
					pq = new_node(num1, num2, client_socket, fcfs_counter++);
				}
				else
				{
					enqueue(&pq, num1, num2, client_socket, fcfs_counter++);
				}
			} else{
				break;
			}
		}
		if (is_empty(&pq))
		{
			continue;
		}
		for (int i = 0; i < THREAD_POOL_SIZE; i++)
		{
			if (workers_data[i][0] == 1) // means worker is free
			{
				int num1, num2, client_socket;
				if (is_empty(&pq))
				{
					continue;
				}
				dequeue(&pq, &num1, &num2, &client_socket);
				workers_data[i][1] = num1;
				workers_data[i][2] = num2;
				workers_data[i][3] = client_socket;
				workers_data[i][0] = 0;
			}
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

	pthread_t master_thread_id;
	if (pthread_create(&master_thread_id, NULL, master_thread, NULL) != 0)
	{
		perror("Error creating master thread");
		return 1;
	}

	main_index = 0;
	master_index = 0;

	printf("Server Successfully binded to port %d and listen on it\n", PORT);

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

		printf("client %d is accepted\n", client_socket);
		FD_ZERO(&readset);
		FD_SET(client_socket, &readset);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		int retval = select(client_socket + 1, &readset, NULL, NULL, &tv);
		if (retval == -1)
		{
			printf("Error receiving data from client %d:\n", client_socket);
		}
		else if (retval)
		{
			char buffer[sizeof(int) * 2];
			recv(client_socket, buffer, sizeof(buffer), 0);

			int first_num, second_num;
			memcpy(&first_num, buffer, sizeof(int));
			memcpy(&second_num, buffer + sizeof(int), sizeof(int));

			first_num = ntohl(first_num);
			second_num = ntohl(second_num);
			printf("Received numbers from client %d:\n", client_socket);

			char message[100];
			sprintf(message, "Received numbers: %d and %d\n", first_num, second_num);

			send(client_socket, message, strlen(message), 0);

			data_array[main_index % MEM_SIZE][0] = client_socket;
			data_array[main_index % MEM_SIZE][1] = first_num;
			data_array[main_index % MEM_SIZE][2] = second_num;
			main_index++;
		}
		else
		{
			printf("Timeout happened for client %d:\n", client_socket);
		}
	}
	return 0;
}