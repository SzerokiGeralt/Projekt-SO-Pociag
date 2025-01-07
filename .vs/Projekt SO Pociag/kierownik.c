#include "mojeFunkcje.h"

int skip_loading = 0;

// Obsługa sygnału SIGINT
void handle_sigint(int sig) {
    printf("\nKierownik: Odebrano sygnal SIGINT");
    int register_sem = sem_get_return(".", 3, 1);
    if (register_sem == -1) {
        exit(0);
    }
    sem_wait(register_sem, 0);
    int register_shm = shared_mem_get_return(".", 1);
    if (register_shm == -1) {
        sem_raise(register_sem, 0);
        exit(0);
    }
    int* register_shm_pointer = shared_mem_attach_int(register_shm);
    // Tutaj pozostał fajny kawałek kodu ale już niepotrzebny ze względu na definicje w mojeFunkcje.h
    //int max_trains = shared_mem_size(register_shm)/ sizeof(int);

    // Sprawdzenie rejestru pociągów i usunięcie PID pociągu
    for (int i = 0; i < MAX_TRAINS; i++) {
        if (register_shm_pointer[i] == getpid()) {
            register_shm_pointer[i] = 0;
            printf("\nKierownik: Usuwam pociąg %d z rejestru.", getpid());
            break;
        }
    }
    sem_raise(register_sem, 0);
    exit(0);
}

void handle_sigusr1(int sig) {
    printf("\nKierownik: Odebrano sygnal SIGUSR1");
    skip_loading = 1;
}

int main() {
    setbuf(stdout, NULL);
    signal(SIGINT, handle_sigint);
    signal(SIGUSR1, handle_sigusr1);

    int train_ID = getpid();
    printf("\nNowy kierownik pociagu PID: %d", train_ID);

    
    int register_sem;
    while ((register_sem = sem_get_return(".", 3, 1)) == -1) {
        printf("\nKierownik: Brak zawiadowcy. Czekam %d ...", train_ID);
        usleep(INTERVAL_TIME*TIME_SCALE);
        
    }
    // Oczekiwanie na dostepnosc rejestru zawiadowcy
    sem_wait(register_sem, 0);
    // Sprawdzenie rejestru pociągów
    int register_shm = shared_mem_get(".", 1);
    int* register_shm_pointer = shared_mem_attach_int(register_shm);
    int train_registered = 0;
    
    for (int i = 0; i < MAX_TRAINS; i++) {
        if (register_shm_pointer[i] == 0) {
            register_shm_pointer[i] = train_ID;
            train_registered = 1;
            printf("\nKierownik: Zarejestrowano pociąg %d.", train_ID);
            break;
        }
    }

    sem_raise(register_sem, 0);

    if (!train_registered) {
        printf("\nKierownik: Maksymalna liczba pociągów osiągnięta. Kończę proces.");
        raise(SIGINT);
    }
    

    // Inicjalizacja zmiennych
    int passengers = 0;
    int bikes = 0;
    int passanger_pid;
    
    pid_t* train = malloc(sizeof(int)*(MAX_PASSANGERS));
    for (int i = 0; i < MAX_PASSANGERS; i++){
        train[i] = 0;
    }
    

    // Przygotowanie wiadomości i kolejek
    struct message* entrance_message = malloc(sizeof(struct message));
    struct message* train_message = malloc(sizeof(struct message));
    int msq0 = get_message_queue(".", 0);
    int msq1 = get_message_queue(".", 1);
    int train_msq = get_message_queue(".", 2); //Kolejka zawiadowcy
    int my_msq = get_message_queue(".", 3); // Kolejka prywatna kierownika

    // Przygotowanie semaforów
    int entrance_sem = sem_get(".", 2, 2);

    while (1) {
        // Powiadomienie zawiadowcy, że pociąg czeka na wjazd
        train_message->ktype = train_ID;
        train_message->mtype = 1; // Gotowy do załadunku
        send_message(train_msq, train_message);

        // Czekanie na zgodę na wjazd
        receive_message(my_msq, 1, train_message);

        printf("\nKierownik: pociąg %d rozpoczyna załadunek.", train_ID);

        // Pętla ładowania pociągu
        // Warunek ładuj dopóki nie ma maksymalnej liczby pasażerów i rowerów lub nie otrzyma komunikatu o wyjeździe
        while (passengers < MAX_PASSANGERS && skip_loading == 0) {
            if (passengers < MAX_PASSANGERS) {
                if (receive_message_no_wait(msq0, 1, entrance_message)) {
                    sem_raise(entrance_sem, 0);
                    train[passengers] = entrance_message->ktype;
                    passengers++;
                    passanger_pid = entrance_message->ktype;
                    printf("\nKierownik: pasażer %d wsiadł do pociągu %d.", passanger_pid, train_ID);
                }
            }
            if (bikes < MAX_BIKES && passengers < MAX_PASSANGERS) {
                if (receive_message_no_wait(msq1, 1, entrance_message)) {
                    sem_raise(entrance_sem, 1);
                    train[passengers] = entrance_message->ktype;
                    passengers++;
                    bikes++;
                    passanger_pid = entrance_message->ktype;
                    printf("\nKierownik: pasażer %d wsiadł do pociągu %d z rowerem.", passanger_pid, train_ID);
                }
            }
            usleep(INTERVAL_TIME*TIME_SCALE);
        }

        if (skip_loading == 0) {
            // Powiadomienie zawiadowcy, że pociąg jest pełny
            train_message->ktype = train_ID;
            train_message->mtype = 2; // Gotowość do odjazdu
            send_message(my_msq, train_message);

            // Czekanie na zgodę na wyjazd
            receive_message(my_msq, 2, train_message);
        } else {
            printf("\nKierownik: pominięto załadunek pociągu %d.", train_ID);
            skip_loading = 0;
            if (passengers == 0) {
                printf("\nKierownik: pociąg %d jest pusty znaczy ze nie ma nikogo do odwiezienia KONIEC PRACY", train_ID);
                raise(SIGINT);
            }
        }
        
        // Wyjazd pociągu
        printf("\nKierownik: pociąg %d odjeżdża.", train_ID);
        usleep(TRAVEL_TIME*TIME_SCALE/2);
        // Pociag dotarł do celu
        printf("\nKierownik: pociąg %d dotarł do celu.", train_ID);
        // Zerowanie liczników
        passengers = 0;
        bikes = 0;
        for (int i = 0; i < MAX_PASSANGERS; i++){
            if (train[i] > 0 && train[i] != getpid()) {
                kill(train[i], SIGINT);
            }
            train[i] = 0;
        }
        usleep(TRAVEL_TIME*TIME_SCALE/2);
        printf("\nKierownik: pociąg %d wrócił.", train_ID);
    }

    return 0;
}
