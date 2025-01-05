#include "mojeFunkcje.h"

void handle_sigint(int sig) {
    printf("\nZawiadowca: Odebrano sygnal SIGINT");
    exit(0);
}

int wait_time(int value) {
    // Maksymalny czas oczekiwania na załadunek
    sleep(value); 
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

int main() {
    setbuf(stdout, NULL);
    signal(SIGINT, handle_sigint); // Obsługa sygnału SIGINT

    printf("\nNowy zawiadowca stacji PID: %d", getpid());

    // Przygotowanie kolejek wiadomości
    int arriving_train_msq = get_message_queue(".", 2); // Kolejka zawiadowcy
    int waiting_train_msq = get_message_queue(".", 3); // Kolejka kierownika
    struct message* train_message = malloc(sizeof(struct message));
    // Przygotowanie semaforów
    int platform_sem = sem_create(".", 1, 2);
    sem_set_value(platform_sem, 0, 0);
    sem_set_value(platform_sem, 1, 0);
    int entrance_sem = sem_create(".", 2, 2);
    sem_set_value(entrance_sem, 0, 0);
    sem_set_value(entrance_sem, 1, 0);
    pid_t wait_time_pid;
    pid_t wait_loaded_pid;
    pid_t finished_pid;
    int max_waittime = 30;

    while (1) {
        // Oczekiwanie na pociąg
        printf("\nZawiadowca: oczekiwanie na pociąg.");
        receive_message(arriving_train_msq, 1, train_message);
        int train_ID = train_message->ktype;

        printf("\nZawiadowca: pociąg %d wjezdza na peron.", train_ID);

        // Powiadomienie kierownika, że może rozpocząć załadunek
        train_message->mtype = 1; // Zgoda na załadunek
        send_message(waiting_train_msq, train_message);

        open_gates();

        // Tworzenie procesów sprawdzających warunki odjazdu
        wait_time_pid = fork();
        if (wait_time_pid == 0) {
            wait_time(max_waittime);
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
    }

    return 0;
}
