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

    printf("Nowy zawiadowca PID: %d\n",getpid());

    while (1) {
        sem_wait(sem_id);

        if (shm_ptr->passengerCount == 0 && shm_ptr->bikeCount == 0) {
            shm_ptr->platformBlocked = 1;
            printf("Zawiadowca: Blokuję peron.\n");
            struct Message msg = {2, "Peron zablokowany"};
            msgsnd(msg_id, &msg, sizeof(msg.text), 0);
        } else {
            shm_ptr->platformBlocked = 0;
            printf("Zawiadowca: Odblokowuję peron.\n");
            struct Message msg = {1, "Peron odblokowany"};
            msgsnd(msg_id, &msg, sizeof(msg.text), 0);
        }

        sem_signal(sem_id);
        sleep(2);
    }

    return 0;
}