#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

int counter = 0;

void *worker1(void *arg){
(void)arg;
printf("worker1: trying to lock lock1\n");
pthread_mutex_lock(&lock1);
printf("worker1: lock1 acquired\n");
sleep(1);
printf("worker1: trying to lock lock2\n");
pthread_mutex_lock(&lock2);
printf("worker1: lock2 acquired\n");
counter++;
pthread_mutex_unlock(&lock2);
pthread_mutex_unlock(&lock1);
return NULL;
}
void *worker2(void *arg){
(void)arg;
printf("worker2: trying to lock lock2\n");
pthread_mutex_lock(&lock2);
printf("worker2: lock2 acquired\n");
sleep(1);
printf("worker2: trying to lock lock1\n");
pthread_mutex_lock(&lock1);
printf("worker2: lock1 acquired\n");
counter++;
pthread_mutex_unlock(&lock1);
pthread_mutex_unlock(&lock2);
return NULL;
}
int main(void){
pthread_t t1;
pthread_t t2;
int status;
status = pthread_create(&t1, NULL, worker1, NULL);
if (status != 0) {
printf("thread 1 creation failed\n");
return 1;
}
status = pthread_create(&t2, NULL, worker2, NULL);
if (status != 0) {
printf("thread 2 creation failed\n");
return 1;
}
pthread_join(t1, NULL);
pthread_join(t2, NULL);
pthread_mutex_destroy(&lock1);
pthread_mutex_destroy(&lock2);
printf("final counter: %d\n", counter);
return 0;
}
