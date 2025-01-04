#include "mojeFunkcje.h"

void handle_sigkill () {
    printf("\nPasazer %d dotarl do destynacji", getpid());
    exit(0);
}

int main() {
    // Inicjalizacja
    srand(getpid());
    signal(2, handle_sigkill);
    setbuf(stdout, NULL);

    // Inicjalizacja zmiennych
    int id = getpid();
    // 30% szans na posiadanie roweru
    int has_bike = (rand() % 100) < 30 ? 1 : 0; 
    int platform_sem = sem_get(".", 1, 2);
    int entrance_sem = sem_get(".", 2, 2);


    printf("\nPasazer: Nowy pasazer PID: %d",id);
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
        //printf("\nPasazer: Pasazer %d z rowerem czeka u progu", id);
        sem_wait(entrance_sem, 1);
        sem_raise(platform_sem, 1);
    } else {
        //ubiega się o miejsce na platformie
        sem_wait(platform_sem, 0);
        entrance = get_message_queue(".", 0);
        //wysyła wiadomość do kierownika że jest bez roweru
        send_message(entrance, entrance_message);
        //printf("\nPasazer: Pasazer %d czeka u progu", id);
        sem_wait(entrance_sem, 0);
        sem_raise(platform_sem, 0);
    }

    //wsiada do pociągu
    //printf("\nPasazer: Pasazer %d wsiadł do pociągu", id);

    while (1) {
        sleep(1);
    }
    
    return 0;
}