#include "mojeFunkcje.h"

void handle_sigint(int sig) {
    printf("\nZawiadowca: Odebrano sygnal SIGINT");
    log_to_file("\nZawiadowca: Odebrano sygnal SIGINT");
    // Usuwanie wszystkich zasobów
    destroy_message_queue(get_message_queue(".", 0));
    destroy_message_queue(get_message_queue(".", 1));
    destroy_message_queue(get_message_queue(".", 2));
    destroy_message_queue(get_message_queue(".", 3));
    sem_destroy(sem_get(".", 1, 2));
    sem_destroy(sem_get(".", 2, 2));
    sem_destroy(sem_get(".", 3, 1));
    shared_mem_destroy(shared_mem_get(".", 1));
    exit(0);
}

int wait_time(int value) {
    // Maksymalny czas oczekiwania na załadunek
    usleep(value); 
    return 0;
}

int wait_loaded(struct message* train_message, int msq_ID) {
    // Oczekiwanie na informację o pełnym pociągu
    receive_message(msq_ID, 6, train_message);
    return 0;
}

void force_passanger_exit_queue(int platform_sem, int entrance_sem) {
    int returned = 0;
    //printf("\nZawiadowca: Prosze odsunac sie od krawedzi peronu.");
    struct message* entrance_message = malloc(sizeof(struct message));
    long temp_pid = 0;
    // Wysłanie sygnału do czekajacych pasażerów o opuszczeniu kolejki
    if (receive_message_no_wait(get_message_queue(".", 0), 1, entrance_message)) {
        printf("\nZawiadowca: wykryto pasażera %ld u progu proszenie o wyjście.", entrance_message->ktype);
        log_to_file("\nZawiadowca: wykryto pasażera %ld u progu proszenie o wyjście.", entrance_message->ktype);
        temp_pid = entrance_message->ktype;
        while (sem_waiters(entrance_sem, 0) == 0)
        {
            printf("\nZawiadowca: poczekaj na delikwenta %ld.", temp_pid);
            log_to_file("\nZawiadowca: opoczekaj na delikwenta %ld.", temp_pid);
            usleep(INTERVAL_TIME*TIME_SCALE);
        }
        // HAHAHAHAHA złapany
        kill(temp_pid, SIGUSR2);
        while (sem_waiters(platform_sem, 0) == 0)
        {
            printf("\nZawiadowca: oczekiwanie na powrót pasażera %ld.", temp_pid);
            log_to_file("\nZawiadowca: oczekiwanie na powrót pasażera %ld.", temp_pid);
            usleep(INTERVAL_TIME*TIME_SCALE);
        }
        
    }
    if (receive_message_no_wait(get_message_queue(".", 1), 1, entrance_message)) {
        printf("\nZawiadowca: wykryto pasażera %ld u progu proszenie o wyjście.", entrance_message->ktype);
        log_to_file("\nZawiadowca: wykryto pasażera %ld u progu proszenie o wyjście.", entrance_message->ktype);
        temp_pid = entrance_message->ktype;
        while (sem_waiters(entrance_sem, 1) == 0)
        {
            printf("\nZawiadowca: poczekaj na delikwenta %ld.", temp_pid);
            log_to_file("\nZawiadowca: opoczekaj na delikwenta %ld.", temp_pid);
            usleep(INTERVAL_TIME*TIME_SCALE);
        }
        // HAHAHAHAHA złapany
        kill(temp_pid, SIGUSR2);
        while (sem_waiters(platform_sem, 1) == 0)
        {
            printf("\nZawiadowca: oczekiwanie na powrót pasażera %ld.", temp_pid);
            log_to_file("\nZawiadowca: oczekiwanie na powrót pasażera %ld.", temp_pid);
            usleep(INTERVAL_TIME*TIME_SCALE);
        }        
    }
    free(entrance_message);
    printf("\nZawiadowca: wszyscy odeszli od krawędzi");
    log_to_file("\nZawiadowca: wszyscy odeszli od krawędzi");
    return;
}

void open_gates() {
    sem_raise(sem_get(".", 1, 2),0);
    sem_raise(sem_get(".", 1, 2),1);
    printf("\nZawiadowca: otwieram bramki.");
    log_to_file("\nZawiadowca: otwieram bramki.");
}

void close_gates() {
    sem_set_value(sem_get(".", 1, 2), 0, 0);
    sem_set_value(sem_get(".", 1, 2), 1, 0);
    printf("\nZawiadowca: zamykam bramki.");
    log_to_file("\nZawiadowca: zamykam bramki.");
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    signal(SIGINT, handle_sigint);

    printf("\nNowy zawiadowca stacji PID: %d", getpid());
    log_to_file("\nNowy zawiadowca stacji PID: %d", getpid());

    // Semafor do otwierania i zamykania bramek na peronie
    int platform_sem = sem_create_once(".", 1, 2);
    if (platform_sem == -1) {
        printf("\nInny zawiadowca już istnieje");
        log_to_file("\nInny zawiadowca już istnieje");
        exit(1);
    }
    sem_set_value(platform_sem, 0, 0);
    sem_set_value(platform_sem, 1, 0);

    // Semafor do otwierania i zamykania wejść do pociagu
    int entrance_sem = sem_create_once(".", 2, 2);
    if (entrance_sem == -1) {
        printf("\nInny zawiadowca już istnieje");
        log_to_file("\nInny zawiadowca już istnieje");
        exit(1);
    }
    sem_set_value(entrance_sem, 0, 0);
    sem_set_value(entrance_sem, 1, 0);

    // Semafor do rejestracji pociągów
    int register_sem = sem_create(".", 3, 1);

    // Przygotowanie pamięci współdzielonej dla rejestracji pociągów
    //sem_set_value(register_sem, 0, 0);
    int register_shm = shared_mem_create(".", 1, MAX_TRAINS*sizeof(int));
    int* register_shm_pointer = shared_mem_attach_int(register_shm);
    for (int i = 0; i < MAX_TRAINS; i++) {
        // Zapewniamy puste miejsca w rejestrze
        register_shm_pointer[i] = 0;
    }
    // Zezwolenie na rejestrację pociągów
    sem_raise(register_sem, 0);
    
    pid_t wait_time_pid;
    pid_t wait_loaded_pid;
    pid_t finished_pid;

    // Przygotowanie kolejek wiadomości
    int arriving_train_msq = get_message_queue(".", 2); // Kolejka zawiadowcy
    int waiting_train_msq = get_message_queue(".", 3); // Kolejka kierownika
    struct message* train_message = malloc(sizeof(struct message));

    printf("\nZawiadowca: oczekiwanie na pierwszy pociąg.");
    log_to_file("\nZawiadowca: oczekiwanie na pierwszy pociąg.");
    receive_message(arriving_train_msq, 1, train_message);
    int train_ID = train_message->ktype;

    while (1) {

        // Powiadomienie kierownika, że może rozpocząć załadunek
        train_message->mtype = train_ID;
        train_message->ktype = train_ID;
        send_message(waiting_train_msq, train_message);

        printf("\nZawiadowca: oczekiwanie na rozpoczęcie ładowania %d.",train_ID);
        log_to_file("\nZawiadowca: oczekiwanie na rozpoczęcie ładowania %d.",train_ID);
        receive_message(waiting_train_msq,7,train_message);


        open_gates();
        
        // Tworzenie procesów sprawdzających warunki odjazdu
        wait_time_pid = fork();
        if (wait_time_pid == 0) {
            wait_time(MAX_WAITTIME*TIME_SCALE);
            exit(0);
        }
        wait_loaded_pid = fork();
        if (wait_loaded_pid == 0) {
            wait_loaded(train_message, waiting_train_msq);
            exit(0);
        }

        

        //Czeka na skonczenie procesów sprawdzajacych warunki odjazdu
        int status;
        finished_pid = wait(&status);
        if (finished_pid == wait_time_pid) {
            // KONIEC CZASU
            //Usuwanie drugiego procesu sprawdzającego
            printf("\nZawiadowca: pociąg stoi za długo.");
            log_to_file("\nZawiadowca: pociąg stoi za długo.");
            kill(wait_loaded_pid, SIGKILL);

            // Odbieramy zakończenie drugiego dziecka, żeby uniknąć 'zombie'
            wait(NULL);

            // Nie pozwalamy pasażerom na wsiadanie
            //close_gates();

            // Wysyłamy sygnał 1 do kierownika, żeby pominął załadunek
            kill(train_ID, SIGUSR1);

            //close_gates();

            printf("\nZawiadowca: Czeka aż Pociąg %d potwierdzi zaprzestanie ładowania.", train_ID);
            log_to_file("\nZawiadowca: Czeka aż Pociąg %d  potwierdzi zaprzestanie ładowania.", train_ID);
            // Czeka aż potwierdzi zaprzestanie ładowania
            while (1) {
                if (receive_message_no_wait(waiting_train_msq, 4, train_message)) break;
                if (receive_message_no_wait(waiting_train_msq, 6, train_message)) break;
            }; 
            receive_message_no_wait(waiting_train_msq, 4, train_message);
            receive_message_no_wait(waiting_train_msq, 6, train_message);
            printf("\nZawiadowca: Pociąg %d przerwał załadunek",train_ID);
            log_to_file("\nZawiadowca: Pociąg %d przerwał załadunek",train_ID);

            printf("\nZawiadowca: Wydanie decyzji o tym co ma zrobić pociąg %d ", train_ID);
            log_to_file("\nZawiadowca: Wydanie decyzji o tym co ma zrobić pociąg %d ", train_ID);
            // Wydanie decyzji o tym co ma zrobić pociąg
            train_message->mtype = 5;
            train_message->ktype = 0;
            send_message(waiting_train_msq, train_message);
            // Czekamy aż wszyscy pasażerowie zejdą z wejścia
            //close_gates();
            while (sem_waiters(entrance_sem, 0) != 0 || sem_waiters(entrance_sem, 1) != 0)
            {
                printf("\nZawiadowca: oczekiwanie na zejście pasażerów.");
                log_to_file("\nZawiadowca: oczekiwanie na zejście pasażerów.");
                printf("\n%d",sem_waiters(entrance_sem, 0));
                log_to_file("\n%d",sem_waiters(entrance_sem, 0));
                printf("\n%d",sem_waiters(entrance_sem, 1));
                log_to_file("\n%d",sem_waiters(entrance_sem, 1));
                force_passanger_exit_queue(platform_sem,entrance_sem);
                usleep(INTERVAL_TIME*TIME_SCALE);
            }


            // Zezwalamy na odjazd pociągu
            train_message->mtype = 2;
            train_message->ktype = train_ID;
            send_message(waiting_train_msq, train_message);
            printf("\nZawiadowca: pociąg %d może odjechać.", train_ID);
            log_to_file("\nZawiadowca: pociąg %d może odjechać.", train_ID);
        } else {
            kill(wait_time_pid, SIGKILL);

            // Odbieramy zakończenie drugiego dziecka, żeby uniknąć 'zombie'
            wait(NULL);
            // POCIĄG JEST PEŁNY

            // Nie pozwalamy pasażerom na wsiadanie
            //close_gates();

            printf("\nZawiadowca: Czeka aż Pociąg %d potwierdzi zaprzestanie ładowania.", train_ID);
            log_to_file("\nZawiadowca: Czeka aż Pociąg %d  potwierdzi zaprzestanie ładowania.", train_ID);
            receive_message(waiting_train_msq, 4, train_message);

            // Wydanie decyzji o tym co ma zrobić pociąg
            train_message->mtype = 5;
            train_message->ktype = 1;
            send_message(waiting_train_msq, train_message);

            printf("\nZawiadowca: pociąg %d jest pełny.", train_ID);
            log_to_file("\nZawiadowca: pociąg %d jest pełny.", train_ID);

            // Czekamy aż wszyscy pasażerowie zejdą z wejścia
            //close_gates();
            while (sem_waiters(entrance_sem, 0) != 0 || sem_waiters(entrance_sem, 1) != 0)
            {
                printf("\nZawiadowca: oczekiwanie na zejście pasażerów.");
                log_to_file("\nZawiadowca: oczekiwanie na zejście pasażerów.");
                printf("\n%d",sem_waiters(entrance_sem, 0));
                log_to_file("\n%d",sem_waiters(entrance_sem, 0));
                printf("\n%d",sem_waiters(entrance_sem, 1));
                log_to_file("\n%d",sem_waiters(entrance_sem, 1));
                force_passanger_exit_queue(platform_sem,entrance_sem);
                usleep(INTERVAL_TIME*TIME_SCALE);
            }
            
            

            // Pociąg jest pełny
            // Zezwalamy na odjazd pociągu
            train_message->mtype = 2;
            train_message->ktype = train_ID;
            send_message(waiting_train_msq, train_message);
            printf("\nZawiadowca: pociąg %d może odjechać.", train_ID);
            log_to_file("\nZawiadowca: pociąg %d może odjechać.", train_ID);
        }

        printf("\nZawiadowca: czeka aż %d potwierdzi odjazd.",train_ID);
        log_to_file("\nZawiadowca: czeka aż %d potwierdzi odjazd.",train_ID);
        // Czeka aż potwierdzi odjazd
        receive_message(waiting_train_msq,3,train_message);
        printf("\nPociąg %d odjechał",train_ID);
        log_to_file("\nPociąg %d odjechał",train_ID);

        // Oczekiwanie na pociąg
        printf("\nZawiadowca: oczekiwanie na pociąg.");
        log_to_file("\nZawiadowca: oczekiwanie na pociąg.");
        while (receive_message_no_wait(arriving_train_msq, 1, train_message) == 0)
        {
        // Sprawdzenie czy są jeszcze jakieś pociągi w rejestrze
        int all_trains_empty = 0;
        sem_wait(register_sem, 0);
        all_trains_empty = 1;
        for (int i = 0; i < MAX_TRAINS; i++) {
            if (register_shm_pointer[i] != 0) {
            printf("\nZawiadowca: pociąg %d w rejestrze.", register_shm_pointer[i]);
            log_to_file("\nZawiadowca: pociąg %d w rejestrze.", register_shm_pointer[i]);
            all_trains_empty = 0;
            break;
            }
        }
        sem_raise(register_sem, 0);
        if (all_trains_empty) {
            printf("\nZawiadowca: brak pociągów w rejestrze KONIEC PRACY");
            log_to_file("\nZawiadowca: brak pociągów w rejestrze KONIEC PRACY");
            raise(SIGINT);
        }
            usleep(INTERVAL_TIME*TIME_SCALE);
            printf("\nZawiadowca: oczekiwanie na pociąg.");
            log_to_file("\nZawiadowca: oczekiwanie na pociąg.");
        }
        train_ID = train_message->ktype;

        printf("\nZawiadowca: pociag %d otrzymuje zezwolenie na wjazd.", train_ID);
        log_to_file("\nZawiadowca: pociag %d otrzymuje zezwolenie na wjazd.", train_ID);


    }

    return 0;
}
