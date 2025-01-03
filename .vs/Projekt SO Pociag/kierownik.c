#include "mojeFunkcje.h"

//Obsługa sygnału SIGINT
void handle_sigint(int sig) {
    sem_destroy(sem_get(".", 2, 2));
    destroy_message_queue(get_message_queue(".", 0));
    destroy_message_queue(get_message_queue(".", 1));
    exit(0);
}

void train_process(int train_no) {
    printf("\nPociag: Pociag numer: %d PID: %d odjezdza",train_no ,getpid());
    struct message* train_message_process = malloc(sizeof(struct message));
    sleep(10);
    printf("\nPociag: Pociag %d wrócił", train_no);
    train_message_process->ktype = train_no;
    train_message_process->mtype = 1;
    send_message(get_message_queue(".", 3), train_message_process);
    return;
}

int main() {
    setbuf(stdout, NULL);
    printf("\nNowy kierownik pociagu PID: %d",getpid());

    //Inicjalizacja zmiennych
    int no_trains = 2;
    int max_passengers = 5;
    int max_bikes = 2;
    int passengers = 0;
    int bikes = 0;
    int current_train = 1;
    int passanger_pid;

    //Przygotowanie wiaodmości i kolejek
    struct message* entrance_message = malloc(sizeof(struct message));
    struct message* train_message = malloc(sizeof(struct message));
    int msq0 = get_message_queue(".", 0);
    int msq1 = get_message_queue(".", 1);
    int train_msq = get_message_queue(".", 3);

    //Przygotowanie semaforów
    int entrance_sem = sem_create(".", 2, 2);
    sem_set_value(entrance_sem, 0, 1);
    sem_set_value(entrance_sem, 1, 1);

    //Przygotowanie pamięci współdzielonej reprezentującej pociągi
    char* trains = shared_mem_attach(shared_mem_create(".",3,1));

    //Inicjalizacja kolejki pociągów
    for (int i = 0; i < no_trains; i++) {
        train_message->ktype = i;
        train_message->mtype = 1;
        send_message(train_msq, train_message);
    }

    //Główna pętla kierownika
    while (1) {
        //Oczekiwanie na pociąg
        receive_message(train_msq, 1, train_message);
        current_train = train_message->ktype;
        printf("\nKierownik: pociag %d przyjechal na stacje", current_train);

        //Pętla ładowania pociągu
        while (passengers != max_passengers)
        {
            if (passengers < max_passengers) {
                if (receive_message_no_wait(msq0, 1, entrance_message)) {
                    //Podnoszenie semafora dla pasażera bez roweru
                    sem_raise(entrance_sem, 0);
                    passengers++;
                    passanger_pid = entrance_message->ktype;
                    printf("\nKierownik: pasazer %d wsiadl do pociagu", passanger_pid);
                }
            }
            if (bikes < max_bikes && passengers < max_passengers) {
                if (receive_message_no_wait(msq1, 1, entrance_message)) {
                    //Podnoszenie semafora dla pasażera z rowerem
                    sem_raise(entrance_sem, 1);
                    passengers++;
                    bikes++;
                    passanger_pid = entrance_message->ktype;
                    printf("\nKierownik: pasazer %d wsiadl do pociagu z rowerem",  passanger_pid);
                }
            }
            sleep(0.5);
        }
        //Odjazd pociągu
        printf("\nKierownik: pociag odjezdza");
        if (fork() == 0) {
            //Uruchomienie procesu pociągu
            train_process(current_train);
        }
        //Zerowanie liczników
        passengers = 0;
        bikes = 0;
    }
    return 0;
}