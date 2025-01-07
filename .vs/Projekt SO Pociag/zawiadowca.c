#include "mojeFunkcje.h"

void handle_sigint(int sig) {
    printf("\nZawiadowca: Odebrano sygnal SIGINT");
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

int wait_loaded(long train_ID, struct message* train_message, int msq_ID) {
    // Oczekiwanie na informację o pełnym pociągu
    receive_message(msq_ID, 2, train_message);
    return 0;
}

int force_passanger_exit_queue() {
    printf("\nZawiadowca: Prosze odsunac sie od krawedzi peronu.");
    struct message* entrance_message = malloc(sizeof(struct message));
    long temp_pid = 0;
    // Wysłanie sygnału do czekajacych pasażerów o opuszczeniu kolejki
    if (receive_message_no_wait(get_message_queue(".", 0), 1, entrance_message)) {
        printf("\nZawiadowca: wyktyto pasażer %ld u progu proszenie o wyjście.", entrance_message->ktype);
        temp_pid = entrance_message->ktype;
        kill(temp_pid, SIGUSR2);
    }
    if (receive_message_no_wait(get_message_queue(".", 1), 1, entrance_message)) {
        printf("\nZawiadowca: wyktyto pasażer %ld u progu proszenie o wyjście.", entrance_message->ktype);
        temp_pid = entrance_message->ktype;
        kill(temp_pid, SIGUSR2);
    }

    free(entrance_message);
}

void open_gates() {
    sem_set_value(sem_get(".", 1, 2), 0, 1);
    sem_set_value(sem_get(".", 1, 2), 1, 1);
    printf("\nZawiadowca: otwieram bramki.");
}

void close_gates() {
    sem_set_value(sem_get(".", 1, 2), 0, 0);
    sem_set_value(sem_get(".", 1, 2), 1, 0);
    printf("\nZawiadowca: zamykam bramki.");
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    signal(SIGINT, handle_sigint);

    printf("\nNowy zawiadowca stacji PID: %d", getpid());

    // Semafor do otwierania i zamykania bramek na peronie
    int platform_sem = sem_create_once(".", 1, 2);
    if (platform_sem == -1) {
        printf("\nInny zawiadowca już istnieje");
        exit(1);
    }
    sem_set_value(platform_sem, 0, 0);
    sem_set_value(platform_sem, 1, 0);

    // Semafor do otwierania i zamykania wejść do pociagu
    int entrance_sem = sem_create_once(".", 2, 2);
    if (entrance_sem == -1) {
        printf("\nInny zawiadowca już istnieje");
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

    receive_message(arriving_train_msq, 1, train_message);
    int train_ID = train_message->ktype;

    while (1) {

        // Powiadomienie kierownika, że może rozpocząć załadunek
        train_message->mtype = 1; // Zgoda na załadunek
        send_message(waiting_train_msq, train_message);

        open_gates();

        // Tworzenie procesów sprawdzających warunki odjazdu
        wait_time_pid = fork();
        if (wait_time_pid == 0) {
            wait_time(MAX_WAITTIME*TIME_SCALE);
            exit(0);
        }
        wait_loaded_pid = fork();
        if (wait_loaded_pid == 0) {
            wait_loaded(train_ID, train_message, waiting_train_msq);
            exit(0);
        }

        //Czeka na skonczenie procesów sprawdzajacych warunki odjazdu
        int status;
        finished_pid = wait(&status);
        if (finished_pid == wait_time_pid) {
            printf("\nZawiadowca: pociąg stoi za długo.");
            kill(wait_loaded_pid, SIGKILL);

            // Odbieramy zakończenie drugiego dziecka, żeby uniknąć 'zombie'
            wait(NULL);

            // Nie pozwalamy pasażerom na wsiadanie
            close_gates();
            force_passanger_exit_queue();

            // Wysyłamy sygnał 1 do kierownika, żeby pominął załadunek
            // Zezwalamy na odjazd pociągu
            kill(train_ID, SIGUSR1);
        } else {
            printf("\nZawiadowca: pociąg %d jest pełny.", train_ID);
            kill(wait_time_pid, SIGKILL);

            // Odbieramy zakończenie drugiego dziecka, żeby uniknąć 'zombie'
            wait(NULL);

            // Nie pozwalamy pasażerom na wsiadanie
            close_gates();
            force_passanger_exit_queue();

            // Pociąg jest pełny
            // Zezwalamy na odjazd pociągu
            train_message->mtype = 2;
            send_message(waiting_train_msq, train_message);
            printf("\nZawiadowca: pociąg %d odjeżdża.", train_ID);
        }
        
        // Oczekiwanie na pociąg
        printf("\nZawiadowca: oczekiwanie na pociąg.");
        while (receive_message_no_wait(arriving_train_msq, 1, train_message) == 0)
        {
        // Sprawdzenie czy są jeszcze jakieś pociągi w rejestrze
        int all_trains_empty = 0;
        sem_wait(register_sem, 0);
        all_trains_empty = 1;
        for (int i = 0; i < MAX_TRAINS; i++) {
            if (register_shm_pointer[i] != 0) {
            printf("\nZawiadowca: pociąg %d w rejestrze.", register_shm_pointer[i]);
            all_trains_empty = 0;
            break;
            }
        }
        sem_raise(register_sem, 0);
        if (all_trains_empty) {
            printf("\nZawiadowca: brak pociągów w rejestrze KONIEC PRACY");
            raise(SIGINT);
        }
            usleep(INTERVAL_TIME*TIME_SCALE);
            printf("\nZawiadowca: oczekiwanie na pociąg.");
        }
        train_ID = train_message->ktype;

        printf("\nZawiadowca: pociag %d otrzymuje zezwolenie na wjazd.", train_ID);


    }

    return 0;
}
