/*#include "mojeFunkcje.h"

void clock(int time) {
    sleep(time);
    prepare_departure();
}

void prepare_departure() {
    printf("\nPrzygotowanie do odjazdu");
}

int main() {
    setbuf(stdout, NULL);
    printf("\nNowy zawiadowca stacji PID: %d",getpid());

    int train_msq;
    int current_train = 0;
    int no_trains = 2;
    
    int platform_sem = sem_create(".", 1, 2);
    sem_set_value(platform_sem, 0, 1);
    sem_set_value(platform_sem, 1, 1);

    struct message* train_message = malloc(sizeof(struct message));
    int train_msq = get_message_queue(".", 2);

    //Inicjalizacja kolejki pociągów
    for (int i = 0; i < no_trains; i++) {
        train_message->ktype = i;
        train_message->mtype = 1;
        send_message(train_msq, train_message);
    }

    while (1)
    {
        //Oczekiwanie na pociąg
        receive_message(train_msq, 1, train_message);
        current_train = train_message->ktype;
        printf("\nKierownik: pociag %d przyjechal na stacje", current_train);
    }
    
    return 0;
}*/
#include "mojeFunkcje.h"

void handle_sigint(int sig) {
    destroy_message_queue(get_message_queue(".", 2));
    sem_destroy(sem_get(".", 1, 2));
    exit(0);
}

int main() {
    setbuf(stdout, NULL);
    signal(9, handle_sigint); // Obsługa sygnału SIGINT

    printf("\nNowy zawiadowca stacji PID: %d", getpid());

    int train_msq = get_message_queue(".", 2); // Kolejka zawiadowcy
    struct message* train_message = malloc(sizeof(struct message));
    int platform_sem = sem_create(".", 1, 2);
    sem_set_value(platform_sem, 0, 1);
    sem_set_value(platform_sem, 1, 1);

    while (1) {
        // Oczekiwanie na pociąg gotowy do załadunku
        receive_message(train_msq, 1, train_message);
        int train_ID = train_message->ktype;

        printf("\nZawiadowca: pociąg %d gotowy do załadunku.", train_ID);

        // Powiadomienie kierownika, że może rozpocząć załadunek
        train_message->mtype = 1; // Zgoda na załadunek
        send_message(get_message_queue(".", train_ID), train_message);

        // Oczekiwanie na informację o pełnym pociągu
        receive_message(train_msq, 2, train_message);
        printf("\nZawiadowca: pociąg %d jest pełny i odjeżdża.", train_ID);

        // Oczekiwanie na powrót pociągu
        receive_message(train_msq, 0, train_message);
        printf("\nZawiadowca: pociąg %d wrócił na stację.", train_ID);
    }

    return 0;
}
