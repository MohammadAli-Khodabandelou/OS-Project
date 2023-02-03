#include <stdio.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <pthread.h> 
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_ADDRESS "127.0.0.1"

void* send_data(void* data) 
{ 
    int* numbers = (int*)data; 
    int socket_desc, c; 
    struct sockaddr_in server; 

    socket_desc = socket(AF_INET, SOCK_STREAM, 0); 
    if (socket_desc == -1) 
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

    int num1 = numbers[0]; 
    int num2 = numbers[1];
    char buffer[sizeof(int) * 2];
    num1 = htonl(num1);
    num2 = htonl(num2);
    memcpy(buffer, &num1, sizeof(int));
    memcpy(buffer + sizeof(int), &num2, sizeof(int));


    send(socket_desc, buffer, sizeof(buffer), 0); 

    printf("Numbers sent to server.\n"); 

    char message[100];
    memset(message, 0, sizeof(message));
    recv(socket_desc, message, sizeof(message), 0);

    printf("Received message from server: %s\n", message);

	// receive answer from server

	memset(message, 0, sizeof(message));
    recv(socket_desc, message, sizeof(message), 0);

    printf("Received message from server: %s\n", message);

    return 0; 
} 

int main(int argc, char *argv[]) 
{ 
    int nums[2]; 
    pthread_t thread; 

    printf("Enter two numbers:\n"); 
    scanf("%d %d", &nums[0], &nums[1]); 

    if (pthread_create(&thread, NULL, send_data, (void*)nums) < 0) 
    { 
        printf("Can not create a new thread.\n"); 
        return 1; 
    } 

    pthread_join(thread, NULL); 

    return 0; 
} 
