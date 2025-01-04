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

int wait_loaded(long train_ID, struct message* train_message) {
    // Oczekiwanie na informację o pełnym pociągu
    receive_message(get_message_queue(".", train_ID), 2, train_message);
    return 0;
}

int main() {
    setbuf(stdout, NULL);
    signal(2, handle_sigint); // Obsługa sygnału SIGINT

    printf("\nNowy zawiadowca stacji PID: %d", getpid());

    // Przygotowanie kolejek wiadomości
    int train_msq = get_message_queue(".", 2); // Kolejka zawiadowcy
    struct message* train_message = malloc(sizeof(struct message));
    // Przygotowanie semaforów
    int platform_sem = sem_create(".", 1, 2);
    sem_set_value(platform_sem, 0, 1);
    sem_set_value(platform_sem, 1, 1);
    int entrance_sem = sem_create(".", 2, 2);
    sem_set_value(entrance_sem, 0, 1);
    sem_set_value(entrance_sem, 1, 1);
    pid_t wait_time_pid;
    pid_t wait_loaded_pid;
    pid_t finished_pid;
    int max_waittime = 30;

    while (1) {
        // Oczekiwanie na pociąg
        printf("\nZawiadowca: oczekiwanie na pociąg.");
        receive_message(train_msq, 1, train_message);
        int train_ID = train_message->ktype;

        printf("\nZawiadowca: pociąg %d wjezdza na peron.", train_ID);

        // Powiadomienie kierownika, że może rozpocząć załadunek
        train_message->mtype = 1; // Zgoda na załadunek
        send_message(get_message_queue(".", train_ID), train_message);

        // Tworzenie procesów sprawdzających warunki odjazdu
        wait_time_pid = fork();
        if (wait_time_pid == 0) {
            wait_time(max_waittime);
            exit(0);
        }
        wait_loaded_pid = fork();
        if (wait_loaded_pid == 0) {
            wait_loaded(train_ID, train_message);
            exit(0);
        }

        //Czeka na skonczenie procesów sprawdzajacych warunki odjazdu
        int status;
        finished_pid = wait(&status);
        if (finished_pid == wait_time_pid) {
            printf("\nZawiadowca: pociąg stoi za długo.");
            kill(wait_loaded_pid, 9);

            // Odbieramy zakończenie drugiego dziecka, żeby uniknąć 'zombie'
            wait(NULL);

            //Wysyłamy sygnał 1 do kierownika, żeby pominął załadunek
            kill(train_ID, 10);
        } else {
            printf("\nZawiadowca: pociąg %d jest pełny.", train_ID);
            kill(wait_time_pid, 9);

            // Odbieramy zakończenie drugiego dziecka, żeby uniknąć 'zombie'
            wait(NULL);

            //Odjazd pociągu regularny
            train_message->mtype = 2;
            send_message(get_message_queue(".", train_ID), train_message);
            printf("\nZawiadowca: pociąg %d odjeżdża.", train_ID);
        }
    }

    return 0;
}
