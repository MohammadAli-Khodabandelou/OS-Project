#include <stdio.h> 
#include <string.h> 
#include <winsock2.h>
#include <unistd.h> 
#include <pthread.h> 


#define PORT 8080
#define SERVER_ADDRESS "127.0.0.1"

void* send_data(void* data) 
{ 
    int* numbers = (int*)data; 
    int c; 
    struct sockaddr_in server; 

    WSADATA wsa;
	
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		printf("Failed. Error Code : %d",WSAGetLastError());
		return 1;
	}

    SOCKET socket_desc = socket(AF_INET, SOCK_STREAM, 0); 
    if (socket_desc ==  INVALID_SOCKET) 
    { 
        printf("Can not create socket.\n"); 
        return 0; 
    } 

    server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS); 
    server.sin_family = AF_INET; 
    server.sin_port = htons(PORT); 

    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) 
    { 
        printf("Connection failed.\n"); 
        return 0; 
    } 

    printf("Client Connected to server.\n"); 


    char buffer[sizeof(int) * 5];
    for (int i = 0; i < 5; i++)
    {
        int num = htonl(numbers[i]);
        memcpy(buffer + (i * sizeof(int)), &num, sizeof(int));
    }
    
    send(socket_desc, buffer, sizeof(buffer), 0); 
    printf("Numbers sent to server.\n"); 

    char message[100];
    memset(message, 0, sizeof(message));
    recv(socket_desc, message, sizeof(message), 0);

    printf("Received message from server: %s\n", message);

	// received answer from server

	memset(message, 0, sizeof(message));
    recv(socket_desc, message, sizeof(message), 0);

    printf("Received message from server: %s\n", message);

    return 0; 
} 

int main(int argc, char *argv[]) 
{ 
    int nums[5]; 
    pthread_t thread; 

    printf("Enter two numbers:\n"); 
    scanf("%d %d", &nums[0], &nums[1]); 

    printf("Enter executation time:\n"); 
    scanf("%d", &nums[2]);
    
    printf("Enter deadline of job:\n"); 
    scanf("%d", &nums[3]); 

    printf("Enter Priority (lower number means higher priority):\n"); 
    scanf("%d", &nums[4]); 

    if (pthread_create(&thread, NULL, send_data, (void*)nums) < 0) 
    { 
        printf("Can not create a new thread.\n"); 
        return 1; 
    } 

    pthread_join(thread, NULL); 

    return 0; 
} 
