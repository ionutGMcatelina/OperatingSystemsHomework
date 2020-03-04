#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include "a2_helper.h"
#include <fcntl.h>

sem_t sem61, sem62, sem5, sem5_11, sem5_112;
sem_t *sem34 = NULL;
sem_t *sem33 = NULL;
pthread_mutex_t mutex;
int nrTh;
int found;

void* thFunctionP6(void* arg){
    int s = (int)(size_t)arg;


    if (s == 1){
        sem_wait(&sem62);
    }
    if (s == 3){
        sem_wait(sem34);
    }

    info(BEGIN, 6, s + 1);

    if (s == 0){
        sem_post(&sem62);
        sem_wait(&sem61);
    }
    

    info(END, 6, s + 1);
    if (s == 3){
        sem_post(sem33);
    }

    if (s == 1){
        sem_post(&sem61);
    }

    return NULL;
}

void* thFunctionP5(void* arg){
    int s = (int)(size_t)arg;

    sem_wait(&sem5);
    if (s == 10){
        found = 1;
        sem_wait(&sem5_11);
    }

    info(BEGIN, 5, s + 1);          //BEGIN

    pthread_mutex_lock(&mutex);
    if (s != 10 && found == 1){
        nrTh++;
    }
    pthread_mutex_unlock(&mutex);

    if (found == 1){
        if (nrTh == 3){
            found = 0;
            sem_post(&sem5_11);
        }
        if (s != 10)
            sem_wait(&sem5_112);
    }
    

    info(END, 5, s + 1);            // END
    if (s == 10){
        for (int i = 0; i < 3; i++)
            sem_post(&sem5_112);
    }
    sem_post(&sem5);
    
    return NULL;
}

void* thFunctionP3(void* arg){
    int s = (int)(size_t)arg;
    if (s == 2){
        sem_wait(sem33);
    }
    info(BEGIN, 3, s + 1);

    info(END, 3, s + 1);
    if (s == 3){
        sem_post(sem34);
    }
    return NULL;
}

int main(){
    init();

    pid_t pid2 = -1, pid3 = -1, pid4 = -1, pid5 = -1, pid6 = -1, pid7 = -1;

    info(BEGIN, 1, 0);            // P1

    pid2 = fork();
    if (pid2 == 0){               // P2
        info(BEGIN, 2, 0);

        pid3 = fork();
        if (pid3 == 0){           // P3
            info(BEGIN, 3, 0);

            pid4 = fork();
            if (pid4 == 0){       // P4
                info(BEGIN, 4, 0);
                info(END, 4, 0);
                exit(1);
            }
            else{
                pid6 = fork();
                if (pid6 == 0){       // P6
                    info(BEGIN, 6, 0);
                    sem34 = sem_open("sem34", O_CREAT, 0644, 0);
                    sem33 = sem_open("sem33", O_CREAT, 0644, 0);
                    if(sem34 == NULL || sem33 == NULL) {
                        perror("Could not aquire the semaphore");
                        exit(1);
                    }

                    sem_init(&sem61, 0, 0);
                    sem_init(&sem62, 0, 0);

                    pthread_t th[5];
                    for (int i = 0; i < 4; i++){
                        pthread_create(&th[i], NULL, thFunctionP6, (void*)(size_t)(i));
                    }

                    for (int i = 0; i < 4; i++){
                        pthread_join(th[i], NULL);
                    }
                    info(END, 6, 0);
                    sem_close(sem33);
                    sem_unlink("sem33");
                    sem_close(sem34);
                    sem_unlink("sem34");
                    exit(1);
                } else{
                    sem34 = sem_open("sem34", O_CREAT, 0644, 0);
                    sem33 = sem_open("sem33", O_CREAT, 0644, 0);
                    if(sem34 == NULL || sem33 == NULL) {
                        perror("Could not aquire the semaphore");
                        exit(1);
                    }

                    pthread_t threads[4];
                    for (int i = 0; i < 4; i++){
                        pthread_create(&threads[i], NULL, thFunctionP3, (void*)(size_t)(i));
                    }

                    for (int i = 0; i < 4; i++){
                        pthread_join(threads[i], NULL);
                    }

                    waitpid(pid4, NULL, 0);
                    waitpid(pid6, NULL, 0);

                    info(END, 3, 0);
                    sem_close(sem33);
                    sem_close(sem34);
                    exit(1);
                }
            }
        }
        else{
            pid7 = fork();
            if (pid7 == 0){           // P7
                info(BEGIN, 7, 0);
                info(END, 7, 0);
                exit(1);
            }
            else{
                waitpid(pid3, NULL, 0);
                waitpid(pid7, NULL, 0);

                info(END, 2, 0);
                exit(1);
            }
        }
    }
    else{
        pid5 = fork();
        if (pid5 == 0){               // P5
            info(BEGIN, 5, 0);

            pthread_t ths[40];
            sem_init(&sem5, 0, 4);
            sem_init(&sem5_11, 0, 0);
            sem_init(&sem5_112, 0, 0);
            if(pthread_mutex_init(&mutex, NULL) != 0) {
                perror("error initializing the mutex");
                return 1;
            }
            nrTh = 0;
            found = 0;

            pthread_create(&ths[10], NULL, thFunctionP5, (void*)(size_t)(10));
            for (int i = 0; i < 40; i++){
                if (i != 10)
                    pthread_create(&ths[i], NULL, thFunctionP5, (void*)(size_t)(i));
            }

            for (int i = 0; i < 40; i++){
                pthread_join(ths[i], NULL);
            }
            sem_destroy(&sem5);
            sem_destroy(&sem5_11);
            sem_destroy(&sem5_112);

            info(END, 5, 0);
            exit(1);
        }
        else{

            waitpid(pid2, NULL, 0);
            waitpid(pid5, NULL, 0);

            info(END, 1, 0);
        }
    }
    return 0;
}
