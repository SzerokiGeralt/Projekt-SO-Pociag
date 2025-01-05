#include "mojeFunkcje.h"

int has_bike;

void handle_sigint() {
    printf("\nPasazer %d dotarl do destynacji", getpid());
    exit(0);
}

void handle_sigusr2() {
    printf("\nPasazer %d otrzymal sygnal SIGUSR2", getpid());
}

int main() {
    // Inicjalizacja
    srand(getpid());
    int has_bike = (rand() % 100) < 30 ? 1 : 0;  // 30% szans na posiadanie roweru
    signal(SIGINT, handle_sigint);
    signal(SIGUSR2, handle_sigusr2);
    setbuf(stdout, NULL);

    // Inicjalizacja zmiennych
    int id = getpid();
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

    reset_label:
    //Główny if pasażera
    if (has_bike) {
        //ubiega się o miejsce na platformie
        sem_wait(platform_sem, 1);

        //wysyła wiadomość do kierownika że jest z rowerem
        entrance = get_message_queue(".", 1);
        send_message(entrance, entrance_message);

        printf("\nPasazer: Pasazer %d z rowerem czeka u progu", id);
        if (sem_wait_interruptible(entrance_sem, 1)==0) {
            printf("\nPasazer %d wraca na peron", id);
            goto reset_label;
        }
        sem_raise(platform_sem, 1);
    } else {
        //ubiega się o miejsce na platformie
        sem_wait(platform_sem, 0);

        //wysyła wiadomość do kierownika że jest bez roweru
        entrance = get_message_queue(".", 0);
        send_message(entrance, entrance_message);

        printf("\nPasazer: Pasazer %d czeka u progu", id);
        if (sem_wait_interruptible(entrance_sem, 0)==0) {
            printf("\nPasazer %d wraca na peron", id);
            goto reset_label;
        }
        sem_raise(platform_sem, 0);
    }

    //wsiada do pociągu
    printf("\nPasazer: Pasazer %d wsiadł do pociągu", id);

    //czeka na dotarcie do celu

    sleep(60);
    while (1)
    {
        printf("\nPasazer: Pasazer %d zgubił się", id);
        sleep(0.5);
    }
    
    
    return 0;
}