

/*void prepare_departure() {
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
    printf("\nZawiadowca: Odebrano sygnal SIGINT");
    exit(0);
}

int main() {
    setbuf(stdout, NULL);
    signal(2, handle_sigint); // Obsługa sygnału SIGINT

    printf("\nNowy zawiadowca stacji PID: %d", getpid());

    int train_msq = get_message_queue(".", 2); // Kolejka zawiadowcy
    struct message* train_message = malloc(sizeof(struct message));
    int platform_sem = sem_create(".", 1, 2);
    sem_set_value(platform_sem, 0, 1);
    sem_set_value(platform_sem, 1, 1);

    while (1) {
        // Oczekiwanie na pociąg
        printf("\nZawiadowca: oczekiwanie na pociąg.");
        receive_message(train_msq, 1, train_message);
        int train_ID = train_message->ktype;

        printf("\nZawiadowca: pociąg %d wjezdza na peron.", train_ID);

        // Powiadomienie kierownika, że może rozpocząć załadunek
        train_message->mtype = 1; // Zgoda na załadunek
        send_message(get_message_queue(".", train_ID), train_message);

        // Oczekiwanie na informację o pełnym pociągu
        receive_message(get_message_queue(".", train_ID), 2, train_message);
        printf("\nZawiadowca: pociąg %d jest pełny.", train_ID);

        //Odjazd pociągu
        train_message->mtype = 2;
        send_message(get_message_queue(".", train_ID), train_message);
        printf("\nZawiadowca: pociąg %d odjeżdża.", train_ID);
    }

    return 0;
}
