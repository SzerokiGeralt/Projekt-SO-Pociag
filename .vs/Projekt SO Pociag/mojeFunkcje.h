#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_CONTENT 256

struct message {
    long mtype;
    long ktype;
};


// Tworzy lub uzyskuje dostęp do kolejki komunikatów, zwraca message queue ID
int get_message_queue(const char *path, int proj_ID) {
    key_t key = ftok(path, proj_ID);
    if (key == -1) {
        perror("key_message_queue");
        return -1;
    }

    int msq_ID = msgget(key, IPC_CREAT | 0600);
    if (msq_ID == -1) {
        perror("get_message_queue");
        return -1;
    }

    return msq_ID;
}

// Wysyła komunikat
int send_message(int msq_ID, struct message *msg) {

    if (msgsnd(msq_ID, msg, sizeof(*msg) - sizeof(long), 0) == -1) {
        perror("send_message");
        return -1;
    }
    return 0;
}

// Niszczy kolejkę komunikatów
void destroy_message_queue(int msq_ID) {
    if (msgctl(msq_ID, IPC_RMID, NULL) == -1) {
        perror("destroy_message_queue");
    }
}

//Odbiera pierwszy komunikat typu msgtype
int receive_message(int msq_ID, long msgtype, struct message *msg) {
    if (msgrcv(msq_ID, msg, sizeof(*msg) - sizeof(long), msgtype, 0) == -1) {
        if (errno == ENOMSG) {
            //Brak komunikatu
            return 0;
        } else {
            //Błąd
            perror("recieve_message");
            return -1;
        }
    }
    //Sukces
    return 1;
}

//Odbiera pierwszy komunikat typu msgtype bez czekania
int receive_message_no_wait(int msq_ID, long msgtype, struct message *msg) {
    if (msgrcv(msq_ID, msg, sizeof(*msg) - sizeof(long), msgtype, IPC_NOWAIT) == -1) {
        if (errno == ENOMSG) {
            //Brak komunikatu
            return 0;
        } else {
            //Błąd
            perror("recieve_message");
            return -1;
        }
    }
    //Sukces
    return 1;
}

// Tworzy semafor z klucza "unique_path" oraz "project_name" o liczbie semaforów "nsems"
int sem_create(char* unique_path, int project_name, int nsems) {
    key_t sem_key = ftok(unique_path, project_name);
    if (sem_key == -1) {
        perror("Blad generowania klucza dla semaforow");
        exit(1);
    }
    int sem_ID = semget(sem_key, nsems, 0666 | IPC_CREAT);
    if (sem_ID == -1) {
        perror("Blad tworzenia semafora");
        exit(1);
    }
    return sem_ID;
}

//Pobiernie semafora z klucza "unique_path" oraz "project_name" o liczbie semaforów "nsems"
int sem_get(char* unique_path, int project_name, int nsems) {
    key_t sem_key = ftok(unique_path, project_name);
    if (sem_key == -1) {
        perror("Blad generowania klucza dla semaforow");
        exit(1);
    }
    int sem_ID = semget(sem_key, nsems, 0666);
    if (sem_ID == -1) {
        perror("Blad dostawania semafora");
        exit(1);
    }
    return sem_ID;
}

// Ustawianie "force" wartosci semafora
void sem_set_value(int sem_ID, int num, int value) {
    if (semctl(sem_ID, num, SETVAL, value) == -1) {
        printf("Blad zmiany wartosci semafora %d %d %d \n", sem_ID, num, value);
        perror("Blad zmiany wartosci semafora");
        exit(1);
    }
}

// Operacja P (czekanie na semafor)
void sem_wait(int sem_ID, int num) {
    struct sembuf sops = {num, -1, 0};
    while(1) {
        if (semop(sem_ID, &sops, 1) == -1) {
            if (errno == EINTR) {
                // Przerwane przez sygnał – ponawiamy
                continue;
            }
            printf("Blad wait %d %d\n", sem_ID, num);
            perror("Blad wait");
            exit(1);
        }
        break;
    }
}

// Operacja P (czekanie na semafor) z możliwością przerwania przez sygnał
// Zwraca 0 jeśli przerwane, 1 jeśli wykonane
int sem_wait_interruptible(int sem_ID, int num) {
    struct sembuf sops = {num, -1, 0};
    while(1) {
        if (semop(sem_ID, &sops, 1) == -1) {
            if (errno == EINTR) {
                return 0;
            }
            printf("Blad wait %d %d\n", sem_ID, num);
            perror("Blad wait");
            exit(1);
        }
        break;
    }
    return 1;
}


// Operacja V (podnoszenie semafora)
void sem_raise(int sem_ID, int num) {
    struct sembuf sops = {num, 1, 0};
    if (semop(sem_ID, &sops, 1) == -1) {
        printf("Blad raise %d %d\n", sem_ID, num);
        perror("Blad raise");
        exit(1);
    }
}

// Opuszczenie semafora
void sem_lower_no_wait(int sem_ID, int num) {
    struct sembuf sops = {num, -1, IPC_NOWAIT};
    if (semop(sem_ID, &sops, 1) == -1) {
        printf("Blad wait %d %d\n", sem_ID, num);
        perror("Blad wait");
        exit(1);
    }
}

// Tworzenie pamięci współdzielonej o pojemnosci x * int
int shared_mem_create(char* unique_path, int project_name, int x) {
    key_t mem_key = ftok(unique_path, project_name);
    if (mem_key == -1) {
        perror("Blad generowania klucza do pamieci dzielonej");
        exit(1);
    }
    int mem_ID = shmget(mem_key, x * sizeof(int), 0666 | IPC_CREAT);
    if (mem_ID == -1) {
        perror("Blad tworzenia pamieci wspoldzielonej");
        exit(1);
    }
    return mem_ID;
}

// Uzyskiwanie dostępu do pamięci współdzielonej o pojemnosci x * int
int shared_mem_get(char* unique_path, int project_name, int x) {
    key_t mem_key = ftok(unique_path, project_name);
    if (mem_key == -1) {
        perror("Blad generowania klucza do pamieci dzielonej");
        exit(1);
    }
    int mem_ID = shmget(mem_key, x * sizeof(int), 0666);
    if (mem_ID == -1) {
        perror("Blad dostepu pamieci wspoldzielonej");
        exit(1);
    }
    return mem_ID;
}

// Dołączanie do pamięci współdzielonej
char* shared_mem_attach(int mem_ID) {
    char *shared_mem = (char *)shmat(mem_ID, NULL, 0);
    if (shared_mem == (char *)-1) {
        perror("Blad dolaczania pamieci wspoldzielonej");
        exit(1);
    }
    return shared_mem;
}

// Usuwanie semaforów
void sem_destroy(int sem_ID) {
    if (semctl(sem_ID, 0, IPC_RMID) == -1) {
        printf("Blad usuwania semaforow %d\n", sem_ID);
        perror("Blad usuwania semaforow");
        exit(1);
    }
}