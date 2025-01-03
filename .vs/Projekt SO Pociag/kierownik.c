#include "mojeFunkcje.h"

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
    int no_trains = 1;
    int max_passengers = 5;
    int max_bikes = 2;
    int passengers = 0;
    int bikes = 0;
    int passanger_pid;

    //Przygotowanie wiaodmości i kolejek
    struct message* entrance_message = malloc(sizeof(struct message));
    int msq0 = get_message_queue(".", 0);
    int msq1 = get_message_queue(".", 1);

    //Przygotowanie semaforów
    int entrance_sem = sem_create(".", 2, 2);
    sem_set_value(entrance_sem, 0, 1);
    sem_set_value(entrance_sem, 1, 1);

    //Przygotowanie pamięci współdzielonej reprezentującej pociągi
    char* trains = shared_mem_attach(shared_mem_create(".",3,1));

    //Główna pętla kierownika
    while (1) {
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
        if (passengers == max_passengers) break;
        sleep(0.5);
    }

    //Informacja o zapełnieniu pociągu
    printf("\nKierownik: pociag jest pelny");
    return 0;
}