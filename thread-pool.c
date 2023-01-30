#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define ThreadNumber 4

int working[ThreadNumber] = {0, 0, 0, 0};

typedef struct {
        int m;
        int n;
} ackermann_args;

ackermann_args pairs[ThreadNumber] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};




typedef struct {
        ackermann_args *args;
        int size;
        int capacity;
} ackermann_queue;

void init_queue(ackermann_queue *queue){
        queue->args = malloc(sizeof(ackermann_args) * 100);
        queue->size = 0;
        queue->capacity = 100;
}

void add_queue(ackermann_queue *queue, ackermann_args args){
        if (queue->size == queue->capacity){
                queue->capacity *= 2;
                queue->args = realloc(queue->args, sizeof(ackermann_args) * queue->capacity);
        }
        int i;
        for (i = 0; i < queue->size; i++){
                if (queue->args[i].m + queue->args[i].n > args.m + args.n){
                    memmove(queue->args + i + 1, queue->args + i, sizeof(ackermann_args) * (queue->size - i));
                    queue->args[i] = args;
                    queue->size++;
                    return;
                }
        }
        queue->args[i] = args;
        queue->size++;
}

ackermann_args get_queue(ackermann_queue *queue){
        ackermann_args args = queue->args[0];
        memmove(queue->args, queue->args + 1, sizeof(ackermann_args) * (queue->size - 1));
        queue->size--;
        return args;
}


void *assign_thread(void *args){
        ackermann_queue *queue = (ackermann_queue *)args;
        while (1){
                for (int i = 0; i < ThreadNumber; i++){
                    if (!working[i] && queue->size){
                        pairs[i] = get_queue(queue);
                        working[i] = 1;
                    }
                }
        }
        return NULL;
}

int ackermann(int m, int n){
        if (!m) return n + 1;
        if (!n) return ackermann(m - 1, 1);
        return ackermann(m - 1, ackermann(m, n - 1));
}

void *ackermann_thread(void *args){
        int thread_id = *(int *)args;
        while (1){
            if (working[thread_id]){
                int m = pairs[thread_id].m;
                int n = pairs[thread_id].n;
                printf("A(%d, %d) = %d\n", m, n, ackermann(m, n));
                working[thread_id] = 0;
            }
        }
}



int main() {
        int m, n;

        ackermann_queue queue;
        init_queue(&queue);

        pthread_t assign;
        pthread_create(&assign, NULL, assign_thread, &queue);

        pthread_t threads[ThreadNumber];
        int thread_ids[ThreadNumber] = {0, 1, 2, 3};
        for (int i = 0; i < ThreadNumber; i++){
                pthread_create(&threads[i], NULL, ackermann_thread, &thread_ids[i]);
        }

        while (1) {
            scanf("%d %d", &m, &n);
            if (!m && !n) {
                printf("Invalid input\n");
                continue;
            }
            ackermann_args args = {m, n};
            add_queue(&queue, args);
        }

        return 0;
}