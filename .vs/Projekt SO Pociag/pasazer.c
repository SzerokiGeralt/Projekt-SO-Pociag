#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>

#define SHM_KEY 1234
#define SEM_KEY 91011

struct SharedMemory {
    int passengerCount;
    int bikeCount;
    int platformBlocked;
};

void sem_wait(int sem_id) {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void sem_signal(int sem_id) {
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

int main(int argc, char *argv[]) {
    int id = getpid();
    int hasBike = rand() % 2;
    int shm_id = shmget(SHM_KEY, sizeof(struct SharedMemory), 0666);
    struct SharedMemory *shm_ptr = (struct SharedMemory *)shmat(shm_id, NULL, 0);
    int sem_id = semget(SEM_KEY, 1, 0666);

    printf("Nowy pasazer PID: %d\n",getpid());

    while (1) {
        sem_wait(sem_id);

        if (shm_ptr->platformBlocked) {
            printf("Pasażer %d: Peron zablokowany.\n", id);
            sem_signal(sem_id);
            sleep(1);
            continue;
        }

        if (hasBike && shm_ptr->bikeCount < 5) {
            shm_ptr->bikeCount++;
            printf("Pasażer %d: Wejście z rowerem.\n", id);
        } else if (!hasBike && shm_ptr->passengerCount < 10) {
            shm_ptr->passengerCount++;
            printf("Pasażer %d: Wejście bez roweru.\n", id);
        } else {
            printf("Pasażer %d: Brak miejsca, czekam.\n", id);
        }

        sem_signal(sem_id);
        sleep(rand() % 5 + 1);
    }

    return 0;
}