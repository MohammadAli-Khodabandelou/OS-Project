#include <stdio.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <pthread.h> 

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

    printf("Numbers sent successfully to server.\n"); 

	// receive answer from server
	int answer;
	recv(socket_desc, &answer, sizeof(answer), 0);
	printf("Answer of sum received from server\n");
	printf("Answer is : %d\n", answer);

    return 0; 
} 

int main(int argc, char *argv[]) 
{ 
    int nums[2]; 
    pthread_t thread; 

    printf("Enter two numbers: "); 
    scanf("%d %d", &nums[0], &nums[1]); 

    if (pthread_create(&thread, NULL, send_data, (void*)nums) < 0) 
    { 
        printf("Can not create a new thread.\n"); 
        return 1; 
    } 

    pthread_join(thread, NULL); 

    return 0; 
} 
