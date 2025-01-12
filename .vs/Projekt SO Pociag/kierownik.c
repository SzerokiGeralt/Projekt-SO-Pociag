#include "mojeFunkcje.h"

int skip_loading = 0;

// Obsługa sygnału SIGINT
void handle_sigint(int sig) {
    log_to_file("\nKierownik: Odebrano sygnal SIGINT");
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

    // Sprawdzenie rejestru pociągów i usunięcie PID pociągu
    for (int i = 0; i < MAX_TRAINS; i++) {
        if (register_shm_pointer[i] == getpid()) {
            register_shm_pointer[i] = 0;
            log_to_file("\nKierownik: Usuwam pociąg %d z rejestru.", getpid());
            printf("\nKierownik: Usuwam pociąg %d z rejestru.", getpid());
            break;
        }
    }
    sem_raise(register_sem, 0);
    exit(0);
}

void handle_sigusr1(int sig) {
    log_to_file("\nKierownik: Odebrano sygnal SIGUSR1");
    printf("\nKierownik: Odebrano sygnal SIGUSR1");
    skip_loading = 1;
}

int main() {
    setbuf(stdout, NULL);
    signal(SIGINT, handle_sigint);
    signal(SIGUSR1, handle_sigusr1);

    int train_ID = getpid();
    log_to_file("\nNowy kierownik pociagu PID: %d", train_ID);
    printf("\nNowy kierownik pociagu PID: %d", train_ID);

    int register_sem;
    while ((register_sem = sem_get_return(".", 3, 1)) == -1) {
        log_to_file("\nKierownik: Brak zawiadowcy. Czekam %d ...", train_ID);
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
            log_to_file("\nKierownik: Zarejestrowano pociąg %d.", train_ID);
            printf("\nKierownik: Zarejestrowano pociąg %d.", train_ID);
            break;
        }
    }

    sem_raise(register_sem, 0);

    if (!train_registered) {
        log_to_file("\nKierownik: Maksymalna liczba pociągów osiągnięta. Kończę proces.");
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
    int platform_sem = sem_get(".", 1, 2);
    int entrance_sem = sem_get(".", 2, 2);

    while (1) {
        // Powiadomienie zawiadowcy, że pociąg czeka na wjazd
        train_message->ktype = train_ID;
        train_message->mtype = 1; // Gotowy do załadunku
        send_message(train_msq, train_message);

        log_to_file("\nKierownik: pociąg %d czeka na zgodę na wjazd.", train_ID);
        printf("\nKierownik: pociąg %d czeka na zgodę na wjazd.", train_ID);

        // Czekanie na zgodę na wjazd
        receive_message(my_msq, train_ID, train_message);


        log_to_file("\nKierownik: pociąg %d rozpoczyna załadunek.", train_ID);
        printf("\nKierownik: pociąg %d rozpoczyna załadunek.", train_ID);

        train_message->ktype = train_ID;
        train_message->mtype = 7;
        send_message(my_msq, train_message);

        // Pętla ładowania pociągu
        // Warunek ładuj dopóki nie ma maksymalnej liczby pasażerów i rowerów lub nie otrzyma sygnału
        while (passengers < MAX_PASSANGERS && skip_loading == 0) {
            if (passengers < MAX_PASSANGERS && skip_loading == 0) {
                if (receive_message_no_wait(msq0, 1, entrance_message))
                {
                    passanger_pid = entrance_message->ktype;
                    train[passengers] = passanger_pid;
                    passengers++;
                    sem_raise(entrance_sem, 0);
                    receive_message(msq0,passanger_pid,entrance_message);
                    if (passengers < MAX_PASSANGERS && skip_loading == 0) {
                        sem_raise_interruptible(platform_sem, 0);
                    }
                    usleep(INTERVAL_TIME*TIME_SCALE);
                    log_to_file("\nKierownik: pasażer %d wsiadł do pociągu %d.", passanger_pid, train_ID);
                    printf("\nKierownik: pasażer %d wsiadł do pociągu %d.", passanger_pid, train_ID);
                }
            }
            if (bikes < MAX_BIKES && passengers < MAX_PASSANGERS && skip_loading == 0) {
                if (receive_message_no_wait(msq1, 1, entrance_message))
                {
                    passanger_pid = entrance_message->ktype;
                    train[passengers] = passanger_pid;
                    passengers++;
                    bikes++;
                    sem_raise(entrance_sem, 1);
                    receive_message(msq1,passanger_pid,entrance_message);
                    if (bikes < MAX_BIKES && passengers < MAX_PASSANGERS && skip_loading == 0) {
                        sem_raise_interruptible(platform_sem, 1);
                        
                    }
                    usleep(INTERVAL_TIME*TIME_SCALE);
                    log_to_file("\nKierownik: pasażer %d wsiadł do pociągu %d z rowerem.", passanger_pid, train_ID);
                    printf("\nKierownik: pasażer %d wsiadł do pociągu %d z rowerem.", passanger_pid, train_ID);
                }
            }
        }
        sem_set_value(sem_get(".", 1, 2), 0, 0);
        sem_set_value(sem_get(".", 1, 2), 1, 0);
        log_to_file("\nKierownik: Powiadomienie zawiadowcy, że przerwano ładowanie pociągu %d powód: %d.", train_ID, skip_loading ? 4 : 6);
        printf("\nKierownik: Powiadomienie zawiadowcy, że przerwano ładowanie pociągu %d powód: %d.", train_ID, skip_loading ? 4 : 6);
        // Powiadomienie zawiadowcy, że przerwano ładowanie
        train_message->ktype = train_ID;
        if (skip_loading == 0) {
            train_message->mtype = 6;
            send_message(my_msq, train_message);
        } 
        train_message->mtype =  4;
        send_message(my_msq, train_message);

        log_to_file("\nKierownik: pociąg %d czeka na decyzję zawiadowcy.", train_ID);
        printf("\nKierownik: pociąg %d czeka na decyzję zawiadowcy.", train_ID);
        // Czeka na decyzję zawiadowcy
        receive_message(my_msq, 5, train_message);

        if (train_message->ktype == 1) {
            log_to_file("\nKierownik: pociag %d jest pełny, powiadamianie zawiadowcy.", train_ID);
            printf("\nKierownik: pociag %d jest pełny, powiadamianie zawiadowcy.", train_ID);
        } else {

            log_to_file("\nKierownik: pominięto załadunek pociągu %d.", train_ID);
            printf("\nKierownik: pominięto załadunek pociągu %d.", train_ID);
            skip_loading = 0;
        }

        log_to_file("\nKierownik: pociąg %d czekanie na zgodę na odjazd.", train_ID);
        printf("\nKierownik: pociąg %d czekanie na zgodę na odjazd.", train_ID);
        // Czekanie na zgodę na wyjazd
        receive_message(my_msq, 2, train_message);

        log_to_file("\nKierownik: pociąg %d powiadomienie zawiadowcy o odjeździe.", train_ID);
        printf("\nKierownik: pociąg %d powiadomienie zawiadowcy o odjeździe.", train_ID);        
        // Powiadomienie zawiadowcy o odjeździe
        train_message->ktype = train_ID;
        train_message->mtype = 3;
        send_message(my_msq, train_message);


        // Sprawdzenie czy jest ktokolwiek w pociągu
        if (passengers == 0 && sem_waiters(platform_sem,0) == 0 && sem_waiters(platform_sem,1) == 0) {
            log_to_file("\nKierownik: Brak pasażerów i oczekujących. Kończę proces.");
            printf("\nKierownik: Brak pasażerów i oczekujących. Kończę proces.");
            raise(SIGINT);
        }

        // odjazd pociągu
        log_to_file("\nKierownik: pociąg %d odjeżdża.", train_ID);
        printf("\nKierownik: pociąg %d odjeżdża.", train_ID);
        usleep(TRAVEL_TIME*TIME_SCALE/2);
        // Pociag dotarł do celu
        log_to_file("\nKierownik: pociąg %d dotarł do celu.", train_ID);
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
        log_to_file("\nKierownik: pociąg %d wrócił.", train_ID);
        printf("\nKierownik: pociąg %d wrócił.", train_ID);
    }

    return 0;
}
