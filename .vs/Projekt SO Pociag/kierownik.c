#include "mojeFunkcje.h"

int skip_loading = 0;

// Obsługa sygnału SIGINT
void handle_sigint(int sig) {
    printf("\nKierownik: Odebrano sygnal SIGINT");
    destroy_message_queue(get_message_queue(".", getpid())); // Usunięcie kolejki kierownika
    exit(0);
}

void handle_sigusr1(int sig) {
    printf("\nKierownik: Odebrano sygnal SIGUSR1");
    skip_loading = 1;
}

int main() {
    setbuf(stdout, NULL);
    signal(2, handle_sigint); // Przypisanie obsługi sygnału SIGINT
    signal(10, handle_sigusr1); // Przypisanie obsługi sygnału SIGUSR1

    printf("\nNowy kierownik pociagu PID: %d", getpid());

    // Inicjalizacja zmiennych
    int max_passengers = 20;
    int max_bikes = 5;
    int passengers = 0;
    int bikes = 0;
    int passanger_pid;
    
    int train_ID = getpid();
    pid_t* train = malloc(sizeof(int)*(max_passengers));
    for (int i = 0; i < max_passengers; i++){
        train[i] = 0;
    }
    

    // Przygotowanie wiadomości i kolejek
    struct message* entrance_message = malloc(sizeof(struct message));
    struct message* train_message = malloc(sizeof(struct message));
    int msq0 = get_message_queue(".", 0);
    int msq1 = get_message_queue(".", 1);
    int train_msq = get_message_queue(".", 2); //Kolejka zawiadowcy
    int my_msq = get_message_queue(".", train_ID); // Kolejka prywatna kierownika

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
        while (passengers < max_passengers && skip_loading == 0) {
            if (passengers < max_passengers) {
                if (receive_message_no_wait(msq0, 1, entrance_message)) {
                    sem_raise(entrance_sem, 0);
                    train[passengers] = entrance_message->ktype;
                    passengers++;
                    passanger_pid = entrance_message->ktype;
                    sleep(1);
                    printf("\nKierownik: pasażer %d wsiadł do pociągu %d.", passanger_pid, train_ID);
                }
            }
            if (bikes < max_bikes && passengers < max_passengers) {
                if (receive_message_no_wait(msq1, 1, entrance_message)) {
                    sem_raise(entrance_sem, 1);
                    train[passengers] = entrance_message->ktype;
                    passengers++;
                    bikes++;
                    passanger_pid = entrance_message->ktype;
                    sleep(1);
                    printf("\nKierownik: pasażer %d wsiadł do pociągu %d z rowerem.", passanger_pid, train_ID);
                }
            }
            sleep(0.5);
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
        }
        
        // Wyjazd pociągu
        printf("\nKierownik: pociąg %d odjeżdża.", train_ID);
        sleep(15);
        // Pociag dotarł do celu
        printf("\nKierownik: pociąg %d dotarł do celu.", train_ID);
        // Zerowanie liczników
        passengers = 0;
        bikes = 0;
        for (int i = 0; i < max_passengers; i++){
            if (train[i] > 0 && train[i] != getpid()) {
                kill(train[i], 2);
            }
            train[i] = 0;
        }
        sleep(15);
        printf("\nKierownik: pociąg %d wrócił.", train_ID);
    }

    return 0;
}
