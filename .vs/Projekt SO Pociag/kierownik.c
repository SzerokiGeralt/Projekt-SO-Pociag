#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define SHM_KEY 1234
#define SEM_KEY 91011
#define MSG_KEY 5678

struct SharedMemory {
    int passengerCount;
    int bikeCount;
    int platformBlocked;
};

struct Message {
    long mtype;
    char text[100];
};

void sem_wait(int sem_id) {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void sem_signal(int sem_id) {
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

int main() {
    int shm_id = shmget(SHM_KEY, sizeof(struct SharedMemory), 0666);
    struct SharedMemory *shm_ptr = (struct SharedMemory *)shmat(shm_id, NULL, 0);
    int sem_id = semget(SEM_KEY, 1, 0666);
    int msg_id = msgget(MSG_KEY, 0666);

    printf("Nowy kierownik PID: %d\n",getpid());

    while (1) {
        struct Message msg;
        msgrcv(msg_id, &msg, sizeof(msg.text), 1, 0);
        printf("Kierownik: Otrzymano komunikat: %s\n", msg.text);

        sem_wait(sem_id);

        if (shm_ptr->platformBlocked) {
            printf("Kierownik: Peron zablokowany.\n");
            sem_signal(sem_id);
            sleep(1);
            continue;
        }

        printf("Kierownik: Załadunek %d pasażerów i %d rowerów.\n", shm_ptr->passengerCount, shm_ptr->bikeCount);
        shm_ptr->passengerCount = 0;
        shm_ptr->bikeCount = 0;
        printf("Kierownik: Pociąg odjeżdża!\n");

        sem_signal(sem_id);
        sleep(3);
    }

    return 0;
}