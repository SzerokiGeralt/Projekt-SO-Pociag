#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdarg.h>


// Stałe używane do kontrolowania projektu
// Prawa dostepu do elementów IPC
#define ACCESS_RIGHTS 0600
// Skala czasu symulacji
#define TIME_SCALE 100
// Czas podróży pociągu
#define TRAVEL_TIME 10
// Maksymalny czas oczekiwania na załadunek
#define MAX_WAITTIME 5
// Maksymalna liczba pociągów
#define MAX_TRAINS 3
// Maksymalna liczba pasażerów
#define MAX_PASSANGERS 200
// Maksymalna liczba rowerów
#define MAX_BIKES 50
// Częstotliwość generowania pasażerów (używane w master.c)
#define PASSANGER_SPAWNRATE 5
// Używane w pętlach aby oszczędzić zasoby procesora
#define INTERVAL_TIME 0.2
// Ilość tworzonych pasażerów (używane w master.c)
#define SPAWN_PASSANGERS 300
// Czy generować pliki log dla kazdego procesu
#define LOG_FILES_ENABLED 1

// Struktura komunikatu
struct message {
    long mtype;
    long ktype;
};

void log_to_file(const char *format, ...);
void my_error(const char *msg, int id);

// Funkcja do sprawdzania błędów
void my_error(const char *msg, int id) {
    if (errno != 0) {
        char my_perror_msg[256];
        snprintf(my_perror_msg, sizeof(my_perror_msg), "PID: %d - ID: %d - %s", getpid(), id, msg);
        perror(my_perror_msg);
    }
}

// Funkcja do zapisywania logów do pliku
void log_to_file(const char *format, ...) {
    if (LOG_FILES_ENABLED) {
        char filename[256];
        snprintf(filename, sizeof(filename), "log_%d.txt", getpid());

        FILE *file = fopen(filename, "a");
        if (file == NULL) {
            my_error("Blad otwierania pliku logu", -1);
            return;
        }

        va_list args;
        va_start(args, format);
        vfprintf(file, format, args);
        va_end(args);

        fprintf(file, "\n");
        fclose(file);
    }
}

// Tworzy lub uzyskuje dostęp do kolejki komunikatów, zwraca message queue ID
int get_message_queue(const char *path, int proj_ID) {
    key_t key = ftok(path, proj_ID);
    if (key == -1) {
        my_error("Blad key_message_queue", -1);
        return -1;
    }

    int msq_ID = msgget(key, IPC_CREAT | ACCESS_RIGHTS);
    if (msq_ID == -1) {
        my_error("Blad get_message_queue", msq_ID);
        return -1;
    }

    return msq_ID;
}

// Wysyła komunikat
int send_message(int msq_ID, struct message *msg) {
    while (1) {
        log_to_file("\nWysyłanie komunikatu o typie: %ld do kolejki: %d", msg->mtype, msq_ID);
        printf("\nWysyłanie komunikatu o typie: %ld do kolejki: %d", msg->mtype, msq_ID);
        if (msgsnd(msq_ID, msg, sizeof(*msg) - sizeof(long), 0) == -1) {
            if (errno == EINTR) {
                log_to_file("Przerwano przez sygnał podczas wysyłania komunikatu");
                printf("Przerwano przez sygnał podczas wysyłania komunikatu\n");
                // Przerwane przez sygnał – ponawiamy
                continue;
            }
            my_error("Blad send_message", msq_ID);
            return -1;
        }
        // Sukces
        return 0;
    }
}

// Niszczy kolejkę komunikatów
void destroy_message_queue(int msq_ID) {
    if (msgctl(msq_ID, IPC_RMID, NULL) == -1) {
        my_error("Blad destroy_message_queue", msq_ID);
    }
}

// Odbiera pierwszy komunikat typu msgtype bez możliwości przerwania przez sygnał
int receive_message(int msq_ID, long msgtype, struct message *msg) {
    while (1) {
        if (msgrcv(msq_ID, msg, sizeof(*msg) - sizeof(long), msgtype, 0) == -1) {
            if (errno == EINTR) {
                log_to_file("Przerwano przez sygnał podczas odbierania komunikatu o typie: %ld", msgtype);
                printf("Przerwano przez sygnał podczas odbierania komunikatu o typie: %ld\n", msgtype);
                // Przerwane przez sygnał – ponawiamy
                continue;
            } else if (errno == ENOMSG) {
                // Brak komunikatu
                log_to_file("Brak komunikatu w kolejce o typie: %ld", msgtype);
                printf("Brak komunikatu w kolejce o typie: %ld\n", msgtype);
                continue;
            } else {
                // Błąd
                log_to_file("Blad recieve_message: %ld", msgtype);
                printf("Blad recieve_message: %ld\n", msgtype);
                my_error("Blad recieve_message", msq_ID);
                return -1;
            }
        }
        // Sukces
        return 1;
    }
}

// Odbiera pierwszy komunikat typu msgtype z możliwością przerwania przez sygnał
// Zwraca 0 jeśli przerwane, 1 jeśli wykonane
int receive_message_interruptible(int msq_ID, long msgtype, struct message *msg) {
    if (msgrcv(msq_ID, msg, sizeof(*msg) - sizeof(long), msgtype, 0) == -1) {
        if (errno == EINTR) {
            // Przerwane przez sygnał – ponawiamy
            return -2;
        } else if (errno == ENOMSG) {
            // Brak komunikatu
            return 0;
        } else {
            // Błąd
            my_error("Blad recieve_message_interruptible", msq_ID);
            return -1;
        }
    }
    return 1;
}

//Odbiera pierwszy komunikat typu msgtype bez czekania
int receive_message_no_wait(int msq_ID, long msgtype, struct message *msg) {
    while (1) {
        if (msgrcv(msq_ID, msg, sizeof(*msg) - sizeof(long), msgtype, IPC_NOWAIT) == -1) {
            if (errno == EINTR) {
                // Przerwane przez sygnał – ponawiamy
                continue;
            } else if (errno == ENOMSG) {
                // Brak komunikatu
                return 0;
            } else {
                // Błąd
                my_error("Blad recieve_message_no_wait", msq_ID);
                return -1;
            }
        }
        // Sukces
        return 1;
    }
}

// Tworzy semafor z klucza "unique_path" oraz "project_name" o liczbie semaforów "nsems"
int sem_create(char* unique_path, int project_name, int nsems) {
    key_t sem_key = ftok(unique_path, project_name);
    if (sem_key == -1) {
        my_error("Blad generowania klucza dla semaforow", -1);
        exit(1);
    }
    int sem_ID = semget(sem_key, nsems, ACCESS_RIGHTS | IPC_CREAT);
    if (sem_ID == -1) {
        my_error("Blad tworzenia semafora", sem_ID);
        exit(1);
    }
    return sem_ID;
}

// Tworzy semafor z klucza "unique_path" oraz "project_name" o liczbie semaforów "nsems"
// Zwraca ID semafora lub -1 w przypadku błędu gdy takie semafory już istnieją
int sem_create_once(char* unique_path, int project_name, int nsems) {
    key_t sem_key = ftok(unique_path, project_name);
    if (sem_key == -1) {
        my_error("Blad generowania klucza dla semaforow", -1);
        exit(1);
    }
    int sem_ID = semget(sem_key, nsems, ACCESS_RIGHTS | IPC_CREAT | IPC_EXCL);
    if (sem_ID == -1) {
        return -1;
    }
    return sem_ID;
}

//Pobiernie semafora z klucza "unique_path" oraz "project_name" o liczbie semaforów "nsems"
int sem_get(char* unique_path, int project_name, int nsems) {
    key_t sem_key = ftok(unique_path, project_name);
    if (sem_key == -1) {
        my_error("Blad generowania klucza dla semaforow", -1);
        exit(1);
    }
    int sem_ID = semget(sem_key, nsems, ACCESS_RIGHTS);
    if (sem_ID == -1) {
        my_error("Blad dostawania semafora", sem_ID);
        exit(1);
    }
    return sem_ID;
}

// Pobieranie semafora z klucza "unique_path" oraz "project_name" o liczbie semaforów "nsems"
// Zwraca ID semafora lub -1 w przypadku błędu gdy taki semafor nie istnieje
int sem_get_return(char* unique_path, int project_name, int nsems) {
    key_t sem_key = ftok(unique_path, project_name);
    if (sem_key == -1) {
        return -1;
    }
    int sem_ID = semget(sem_key, nsems, ACCESS_RIGHTS);
    if (sem_ID == -1) {
        return -1;
    }
    return sem_ID;
}

// Sprawdza, ile czeka na semaforze
int sem_waiters(int sem_ID, int num) {
    // Pobranie liczby oczekujących procesów na semaforze
    int waiters = semctl(sem_ID, num, GETNCNT);
    if (waiters == -1) {
        perror("Blad uzyskiwania liczby oczekujacych na semaforze");
        exit(1);
    }
    return waiters;
}


// Ustawianie "force" wartosci semafora
void sem_set_value(int sem_ID, int num, int value) {
    if (semctl(sem_ID, num, SETVAL, value) == -1) {
        printf("Blad zmiany wartosci semafora %d %d %d \n", sem_ID, num, value);
        my_error("Blad zmiany wartosci semafora", sem_ID);
        exit(1);
    }
}

// Operacja P (czekanie na semafor)
void sem_wait(int sem_ID, int num) {
    struct sembuf sops = {num, -1, 0};
    while(1) {
        if (semop(sem_ID, &sops, 1) == -1) {
            if (errno == EINTR) {
                log_to_file("Przerwano przez sygnał podczas oczekiwania na semafor");
                printf("Przerwano przez sygnał podczas oczekiwania na semafor\n");
                // Przerwane przez sygnał – ponawiamy
                continue;
            }
            printf("Blad wait %d %d\n", sem_ID, num);
            my_error("Blad wait", sem_ID);
            exit(1);
        }
        break;
    }
}


// Operacja P (czekanie na semafor) z możliwością przerwania przez sygnał
// Zwraca 0 jeśli przerwane, 1 jeśli wykonane
int sem_wait_interruptible(int sem_ID, int num) {
    log_to_file("\nsem_wait_interruptible %d",getpid());
    printf("\nsem_wait_interruptible %d",getpid());
    struct sembuf sops = {num, -1, 0};
    if (semop(sem_ID, &sops, 1) == -1) {
        if (errno == EINTR) {
            return 0;
        }
        printf("Blad wait %d %d\n", sem_ID, num);
        my_error("Blad wait", sem_ID);
        exit(1);
    }
    return 1;
}

// Operacja V (podnoszenie semafora)
void sem_raise(int sem_ID, int num) {
    struct sembuf sops = {num, 1, 0};
    while (1) {
        if (semop(sem_ID, &sops, 1) == -1) {
            if (errno == EINTR) {
                log_to_file("Przerwano przez sygnał podczas podnoszenia semafora");
                printf("Przerwano przez sygnał podczas podnoszenia semafora\n");
                // Przerwane przez sygnał – ponawiamy
                continue;
            }
            printf("Blad raise %d %d\n", sem_ID, num);
            my_error("Blad raise", sem_ID);
            exit(1);
        }
        break;
    }
}

// Opuszczenie semafora
void sem_lower_no_wait(int sem_ID, int num) {
    struct sembuf sops = {num, -1, IPC_NOWAIT};
    if (semop(sem_ID, &sops, 1) == -1) {
        printf("Blad wait %d %d\n", sem_ID, num);
        my_error("Blad wait", sem_ID);
        exit(1);
    }
}

// Sprawdzenie czy semafor istnieje
int sem_exists(char* unique_path, int project_name, int nsems) {
    key_t sem_key = ftok(unique_path, project_name);
    if (sem_key == -1) {
        my_error("Blad generowania klucza dla semaforow", -1);
        exit(1);
    }
    int sem_ID = semget(sem_key, nsems, ACCESS_RIGHTS);
    if (sem_ID == -1) {
        if (errno == ENOENT) {
            // Nie istnieje
            return 0;
        } else {
            // Error
            my_error("Błąd semget", sem_ID);
            return -1;
        }
    }
    //Istnieje
    return 1;
}

// Tworzenie pamięci współdzielonej o pojemnosci size
int shared_mem_create(char* unique_path, int project_name, size_t size) {
    key_t mem_key = ftok(unique_path, project_name);
    if (mem_key == -1) {
        my_error("Blad generowania klucza do pamieci dzielonej", -1);
        exit(1);
    }
    int mem_ID = shmget(mem_key, size, ACCESS_RIGHTS | IPC_CREAT);
    if (mem_ID == -1) {
        my_error("Blad tworzenia pamieci wspoldzielonej", mem_ID);
        exit(1);
    }
    return mem_ID;
}

// Uzyskiwanie dostępu do pamięci współdzielonej
int shared_mem_get(char* unique_path, int project_name) {
    key_t mem_key = ftok(unique_path, project_name);
    if (mem_key == -1) {
        my_error("Blad generowania klucza do pamieci dzielonej", -1);
        exit(1);
    }
    int mem_ID = shmget(mem_key, 0, ACCESS_RIGHTS);
    if (mem_ID == -1) {
        my_error("Blad dostepu pamieci wspoldzielonej", mem_ID);
        exit(1);
    }
    return mem_ID;
}

// Zwraca liczbę bajtów w danej pamięci współdzielonej
int shared_mem_size(int mem_ID) {
    struct shmid_ds buf;
    if (shmctl(mem_ID, IPC_STAT, &buf) == -1) {
        my_error("Blad uzyskiwania informacji o pamieci wspoldzielonej", mem_ID);
        exit(1);
    }
    return buf.shm_segsz;
}

// Uzyskiwanie dostępu do pamięci współdzielonej zwraca ID lub -1 w przypadku błędu
int shared_mem_get_return(char* unique_path, int project_name) {
    key_t mem_key = ftok(unique_path, project_name);
    if (mem_key == -1) {
        return -1;
    }
    int mem_ID = shmget(mem_key, 0, ACCESS_RIGHTS);
    if (mem_ID == -1) {
        return -1;
    }
    return mem_ID;
}

// Dołączanie do pamięci współdzielonej jako znak
char* shared_mem_attach_char(int mem_ID) {
    char *shared_mem = (char *)shmat(mem_ID, NULL, 0);
    if (shared_mem == (void *)-1) {
        my_error("Blad dolaczania pamieci wspoldzielonej", mem_ID);
        exit(1);
    }
    return shared_mem;
}

// Dołączanie do pamięci współdzielonej jako int
int* shared_mem_attach_int(int mem_ID) {
    int *shared_mem = (int *)shmat(mem_ID, NULL, 0);
    if (shared_mem == (void *)-1) {
        my_error("Blad dolaczania pamieci wspoldzielonej", mem_ID);
        exit(1);
    }
    return shared_mem;
}

// Odłączanie od pamięci współdzielonej
void shared_mem_detach(char *shared_mem) {
    if (shmdt(shared_mem) == -1) {
        my_error("Blad odlaczania pamieci wspoldzielonej", -1);
        exit(1);
    }
}

// Usuwanie pamięci współdzielonej
void shared_mem_destroy(int mem_ID) {
    if (shmctl(mem_ID, IPC_RMID, NULL) == -1) {
        my_error("Blad usuwania pamieci wspoldzielonej", mem_ID);
        exit(1);
    }
}

// Usuwanie semaforów
void sem_destroy(int sem_ID) {
    if (semctl(sem_ID, 0, IPC_RMID) == -1) {
        printf("Blad usuwania semaforow %d\n", sem_ID);
        my_error("Blad usuwania semaforow", sem_ID);
        exit(1);
    }
}
