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
#include <time.h>

#define PORT 8080
#define THREAD_POOL_SIZE 2
#define MAX_CLIENTS 50
#define MEM_SIZE 10000
#define LOG_FILE_PATH "server_log.txt"

int timeout = 5; // timeout in seconds
fd_set readset;
struct timeval tv;

int workers_data[THREAD_POOL_SIZE][5]; // 0: is_free, 1: client_socket, 2: first_num, 3: second_num, 4: execution_time
float workers_time_data[MEM_SIZE][2];  // 0: arriving time, 1: waiting_time
int data_array[MEM_SIZE][6];		   // 0: client_socket, 1: first_num, 2: second_num 3: execution time, 4: deadline, 5: priority
float arriving_times[MEM_SIZE];
int main_index;
int master_index;
int strategy;
clock_t start;
pthread_mutex_t log_file_lock;

typedef struct node
{
	int first_num;
	int second_num;
	int client_socket;
	int execution_time;
	int deadline;
	int priority;
	double arrived_time;
	struct node *next;
} node;

node *new_node(int first_num, int second_num, int client_socket, int execution_time, int deadline, int priority, double arrived_time)
{
	node *temp = (node *)malloc(sizeof(node));
	temp->first_num = first_num;
	temp->second_num = second_num;
	temp->execution_time = execution_time;
	temp->deadline = deadline;
	temp->priority = priority;
	temp->client_socket = client_socket;
	temp->arrived_time = arrived_time;
	temp->next = NULL;
	return temp;
}

void dequeue(node **head, int *first_num, int *second_num, int *client_socket, int *execution_time, int *deadline, double *arrived_time)
{
	node *temp = *head;
	*first_num = temp->first_num;
	*second_num = temp->second_num;
	*client_socket = temp->client_socket;
	*execution_time = temp->execution_time;
	*deadline = temp->deadline;
	*arrived_time = temp->arrived_time;
	(*head) = (*head)->next;
	free(temp);
}

void enqueue(node **head, int first_num, int second_num, int client_socket, int execution_time, int deadline, int priority, double arrived_time)
{
	node *start = (*head);
	node *temp = new_node(first_num, second_num, client_socket, execution_time, deadline, priority, arrived_time);
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

FILE *open_file(char *address)
{
	FILE *file_p = fopen(address, "a");
	if (file_p == NULL)
	{
		printf("error happened in opening file\n");
		exit(1);
	}
	return file_p;
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
			int execution_time = workers_data[id][4];
			int res = num1 + num2;
			printf("Client with id %d is assigned to worker with id %d\n", client_socket, id + 1);
			sleep(execution_time);

			char message[100];
			sprintf(message, "Done: %d + %d = %d\n", num1, num2, res);

			send(client_socket, message, strlen(message), 0);

			clock_t end = clock();
			double total_time = (double)(end - start) / CLOCKS_PER_SEC;
			double total_time_in_system = total_time - arrived_time;

			sprintf(message, "Done for client_socket %d : %d + %d = %d", client_socket, num1, num2, res);
			printf("%s\n", message);

			pthread_mutex_lock(&log_file_lock);
			FILE *fp = open_file(LOG_FILE_PATH);
			fprintf(fp, "---------------------------------------------\n
							Request with client socket %d assigned to thread with id %d finished
					: Arriving Time
					: %f\r Waiting Time
					: %f\r Total Time
					: %f\n Task is %d + %d = %d\n---------------------------------------------\n ", 
					client_socket, id + 1, workers_time_data[0], workers_time_data[1], total_time_in_system, num1, num2, res);
			fclose(fp);
			pthread_mutex_unlock(&log_file_lock);

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
				int execution_time = data_array[master_index % MEM_SIZE][3];
				int deadline = data_array[master_index % MEM_SIZE][4];
				int priority = data_array[master_index % MEM_SIZE][5];
				double arrived_time = arriving_times[master_index % MEM_SIZE];
				master_index++;

				int final_priority;
				if (strategy == 1)
				{
					final_priority = fcfs_counter;
					fcfs_counter++;
				}
				else if (strategy == 2)
				{
					final_priority = priority;
				}
				else if (strategy == 3)
				{
					final_priority = execution_time;
				}
				else if (strategy == 4)
				{
					final_priority = deadline;
				}

				if (is_empty(&pq))
				{
					pq = new_node(num1, num2, client_socket, execution_time, deadline, final_priority, arrived_time);
				}
				else
				{
					enqueue(&pq, num1, num2, client_socket, execution_time, deadline, final_priority, arrived_time);
				}
			}
			else
			{
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
				int num1, num2, client_socket, execution_time, deadline;
				double arrived_time;
				if (is_empty(&pq))
				{
					continue;
				}
				if (strategy == 4) // strategy based on deadline (EDF)
				{
					while (1)
					{
						if (is_empty(&pq))
						{
							break;
						}
						dequeue(&pq, &num1, &num2, &client_socket, &execution_time, &deadline, &arrived_time);
						clock_t end = clock();
						int elapsed_time = (int)((double)(end - start) / CLOCKS_PER_SEC);
						if (deadline > elapsed_time + execution_time)
						{
							continue;
						}
						else
						{
							clock_t end = clock();
							double assigned_time = (double)(end - start) / CLOCKS_PER_SEC;
							workers_time_data[i][0] = arrived_time;
							workers_time_data[i][1] = assigned_time - arrived_time;
							workers_data[i][1] = num1;
							workers_data[i][2] = num2;
							workers_data[i][3] = client_socket;
							workers_data[i][4] = execution_time;
							workers_data[i][0] = 0;
							break;
						}
					}
				}
				else
				{
					dequeue(&pq, &num1, &num2, &client_socket, &execution_time, &deadline, &arrived_time);
					clock_t end = clock();
					double assigned_time = (double)(end - start) / CLOCKS_PER_SEC;
					workers_time_data[i][0] = arrived_time;
					workers_time_data[i][1] = assigned_time - arrived_time;
					workers_data[i][1] = num1;
					workers_data[i][2] = num2;
					workers_data[i][3] = client_socket;
					workers_data[i][4] = execution_time;
					workers_data[i][0] = 0;
				}
			}
		}
	}

	return NULL;
}

int main(int argc, char const *argv[])
{
	printf("Select your scheduling algorithm:\n1- FCFS\n2- Priority\n3- SJF\n4- EDF\n");
	scanf("%d", &strategy);

	if (pthread_mutex_init(&log_file_lock, NULL) != 0)
	{
		printf("\n mutex init for file has failed\n");
		return 1;
	}

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
	start = clock();

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
			char buffer[sizeof(int) * 5];
			recv(client_socket, buffer, sizeof(buffer), 0);

			clock_t request_received = clock();
			double elapsed_time = (double)(request_received - start) / CLOCKS_PER_SEC;

			int first_num, second_num, priority, deadline, execution_time;
			memcpy(&first_num, buffer, sizeof(int));
			memcpy(&second_num, buffer + sizeof(int), sizeof(int));
			memcpy(&execution_time, buffer + (sizeof(int) * 2), sizeof(int));
			memcpy(&deadline, buffer + (sizeof(int) * 3), sizeof(int));
			memcpy(&priority, buffer + (sizeof(int) * 4), sizeof(int));

			first_num = ntohl(first_num);
			second_num = ntohl(second_num);
			execution_time = ntohl(execution_time);
			deadline = ntohl(deadline);
			priority = ntohl(priority);
			printf("Received numbers from client %d.\n", client_socket);

			char message[100];
			sprintf(message, "Numbers Received Successfully, your client id is %d\n", client_socket);

			send(client_socket, message, strlen(message), 0);

			arriving_times[main_index % MEM_SIZE] = elapsed_time;
			data_array[main_index % MEM_SIZE][0] = client_socket;
			data_array[main_index % MEM_SIZE][1] = first_num;
			data_array[main_index % MEM_SIZE][2] = second_num;
			data_array[main_index % MEM_SIZE][3] = execution_time;
			data_array[main_index % MEM_SIZE][4] = deadline;
			data_array[main_index % MEM_SIZE][5] = priority;
			main_index++;
		}
		else
		{
			printf("Timeout happened for client %d:\n", client_socket);
		}
	}
	pthread_mutex_destroy(&log_file_lock);
	return 0;
}