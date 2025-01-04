/*#include "mojeFunkcje.h"

//Obsługa sygnału SIGINT
void handle_sigint(int sig) {
    sem_destroy(sem_get(".", 2, 2));
    destroy_message_queue(get_message_queue(".", 0));
    destroy_message_queue(get_message_queue(".", 1));
    exit(0);
}



int main() {
    setbuf(stdout, NULL);
    printf("\nNowy kierownik pociagu PID: %d",getpid());

    //Inicjalizacja zmiennych
    int max_passengers = 5;
    int max_bikes = 2;
    int passengers = 0;
    int bikes = 0;
    int train_ID;
    int passanger_pid;
    int timeout = 5;

    //Przygotowanie wiaodmości i kolejek
    struct message* entrance_message = malloc(sizeof(struct message));
    int msq0 = get_message_queue(".", 0);
    int msq1 = get_message_queue(".", 1);
    struct message* train_message = malloc(sizeof(struct message));

    //Przygotowanie semaforów
    int entrance_sem = sem_create(".", 2, 2);
    sem_set_value(entrance_sem, 0, 1);
    sem_set_value(entrance_sem, 1, 1);

    //Przygotowanie pamięci współdzielonej reprezentującej pociągi
    //char* trains = shared_mem_attach(shared_mem_create(".",3,no_trains*(max_bikes+max_passengers)));

    //Główna pętla kierownika
    while (1) {
        //Czeka na wjazd
        receive_message(get_message_queue(".", 3), 1, train_message);

        //Pętla ładowania pociągu
        while (passengers != max_passengers)
        {
            if (passengers < max_passengers) {
                if (receive_message_no_wait(msq0, 1, entrance_message)) {
                    //Podnoszenie semafora dla pasażera bez roweru
                    sem_raise(entrance_sem, 0);
                    passengers++;
                    passanger_pid = entrance_message->ktype;
                    sleep(1);
                    //printf("\nKierownik: pasazer %d wsiadl do pociagu", passanger_pid);
                }
            }
            if (bikes < max_bikes && passengers < max_passengers) {
                if (receive_message_no_wait(msq1, 1, entrance_message)) {
                    //Podnoszenie semafora dla pasażera z rowerem
                    sem_raise(entrance_sem, 1);
                    passengers++;
                    bikes++;
                    passanger_pid = entrance_message->ktype;
                    sleep(1);
                    //printf("\nKierownik: pasazer %d wsiadl do pociagu z rowerem",  passanger_pid);
                }
            }
            sleep(0.5);
        }
        //Informuje o pełnym pociągu
        train_message->ktype = train_ID;
        train_message->mtype = 1;
        send_message(get_message_queue(".", 2), train_message);

        //Czeka na wyjazd
        receive_message(get_message_queue(".", 3), 1, train_message);

        //Wyjazd
        printf("\nPociag: Pociag %d odjechal", train_ID);
        sleep(timeout);
        printf("\nPociag: Pociag %d wrócił", train_ID);
        train_message->ktype = train_ID;
        train_message->mtype = 0;
        send_message(get_message_queue(".", 2), train_message);

        //Zerowanie liczników
        passengers = 0;
        bikes = 0;
    }
    return 0;
}*/
#include "mojeFunkcje.h"

// Obsługa sygnału SIGINT
void handle_sigint(int sig) {
    sem_destroy(sem_get(".", 2, 2));
    destroy_message_queue(get_message_queue(".", 0));
    destroy_message_queue(get_message_queue(".", 1));
    destroy_message_queue(get_message_queue(".", getpid())); // Usunięcie kolejki kierownika
    exit(0);
}

int main() {
    setbuf(stdout, NULL);
    signal(9, handle_sigint); // Obsługa sygnału SIGINT

    printf("\nNowy kierownik pociagu PID: %d", getpid());

    // Inicjalizacja zmiennych
    int max_passengers = 5;
    int max_bikes = 2;
    int passengers = 0;
    int bikes = 0;
    int passanger_pid;
    int train_ID = getpid();

    // Przygotowanie wiadomości i kolejek
    struct message* entrance_message = malloc(sizeof(struct message));
    struct message* train_message = malloc(sizeof(struct message));
    int msq0 = get_message_queue(".", 0);
    int msq1 = get_message_queue(".", 1);
    int train_msq = get_message_queue(".", 2); //Kolejka zawiadowcy
    int my_msq = get_message_queue(".", train_ID); // Kolejka prywatna kierownika

    // Przygotowanie semaforów
    int entrance_sem = sem_create(".", 2, 2);
    sem_set_value(entrance_sem, 0, 1);
    sem_set_value(entrance_sem, 1, 1);

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
        while (passengers < max_passengers && receive_message_no_wait(my_msq, 2, train_message)!=1) {
            if (passengers < max_passengers) {
                if (receive_message_no_wait(msq0, 1, entrance_message)) {
                    sem_raise(entrance_sem, 0);
                    passengers++;
                    passanger_pid = entrance_message->ktype;
                    printf("\nKierownik: pasażer %d wsiadł do pociągu %d.", passanger_pid, train_ID);
                }
            }
            if (bikes < max_bikes && passengers < max_passengers) {
                if (receive_message_no_wait(msq1, 1, entrance_message)) {
                    sem_raise(entrance_sem, 1);
                    passengers++;
                    bikes++;
                    passanger_pid = entrance_message->ktype;
                    printf("\nKierownik: pasażer %d wsiadł do pociągu %d z rowerem.", passanger_pid, train_ID);
                }
            }
        }

        // Powiadomienie zawiadowcy, że pociąg jest pełny
        train_message->ktype = train_ID;
        train_message->mtype = 2; // Gotowość do odjazdu
        send_message(my_msq, train_message);

        // Czekanie na zgodę na wyjazd
        receive_message(my_msq, 2, train_message);

        // Wyjazd pociągu
        printf("\nKierownik: pociąg %d odjeżdża.", train_ID);
        sleep(30);
        // Zerowanie liczników
        passengers = 0;
        bikes = 0;
        printf("\nKierownik: pociąg %d wrócił.", train_ID);
    }

    return 0;
}
