#include "mojeFunkcje.h"

void handle_sigkill () {
    printf("\nPasazer %d dotarl do destynacji", getpid());
    exit(0);
}

int main() {
    // Inicjalizacja
    srand(time(NULL));
    signal(9, handle_sigkill);
    setbuf(stdout, NULL);

    // Inicjalizacja zmiennych
    int id = getpid();
    int has_bike = rand() % 2;
    int platform_sem = sem_get(".", 1, 2);
    int entrance_sem = sem_get(".", 2, 2);


    printf("\nNowy pasazer PID: %d",id);
    if (has_bike) {
        printf(" z rowerem");
    }
    
    int entrance;
    // Przygotowanie wiadomości
    struct message* entrance_message = malloc(sizeof(struct message));
    entrance_message->ktype = id;
    entrance_message->mtype = 1;

    //Główny if pasażera
    if (has_bike) {
        //ubiega się o miejsce na platformie
        sem_wait(platform_sem, 1);
         //wysyła wiadomość do kierownika że jest z rowerem
        entrance = get_message_queue(".", 1);
        send_message(entrance, entrance_message);
        printf("\nPasazer %d z rowerem czeka u progu", id);
        sem_wait(entrance_sem, 1);
        sem_raise(platform_sem, 1);
    } else {
        //ubiega się o miejsce na platformie
        sem_wait(platform_sem, 0);
        entrance = get_message_queue(".", 0);
        //wysyła wiadomość do kierownika że jest bez roweru
        send_message(entrance, entrance_message);
        printf("\nPasazer %d czeka u progu", id);
        sem_wait(entrance_sem, 0);
        sem_raise(platform_sem, 0);
    }

    //wsiada do pociągu
    printf("\nPasazer %d wsiadł do pociągu", id);

    while (1) {
        sleep(1);
    }
    
    return 0;
}