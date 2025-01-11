#include "mojeFunkcje.h"

int reset = 0;

void handle_sigint() {
    printf("\nPasazer %d dotarl do destynacji", getpid());
    log_to_file("\nPasazer %d dotarl do destynacji", getpid());
    exit(0);
}

void handle_sigusr2() {
    printf("\nPasazer %d otrzymal sygnal SIGUSR2", getpid());
    log_to_file("\nPasazer %d otrzymal sygnal SIGUSR2", getpid());
    reset = 1;
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
    int platform_sem;
    while ((platform_sem = sem_get_return(".", 1, 2)) == -1) {
        //printf("\nPasazer : Brak zawiadowcy. Czekam %d ...", id);
        usleep(INTERVAL_TIME*TIME_SCALE);
    }
    int entrance_sem;
    while ((entrance_sem = sem_get_return(".", 2, 2)) == -1) {
        //printf("\nPasazer : Brak zawiadowcy. Czekam %d ...", id);
        usleep(INTERVAL_TIME*TIME_SCALE);
    }

    printf("\nPasazer: Nowy pasazer PID: %d", id);
    log_to_file("\nPasazer: Nowy pasazer PID: %d", id);
    if (has_bike) {
        printf(" z rowerem");
        log_to_file(" z rowerem");
    }

    int entrance;
    // Przygotowanie wiadomości
    struct message* entrance_message = malloc(sizeof(struct message));
    entrance_message->ktype = id;
    entrance_message->mtype = 1;

    //Główny if pasażera
    if (has_bike) {
        while (1) {
            printf("\nPasazer: Pasazer %d z rowerem czeka na peronie", id);
            log_to_file("\nPasazer: Pasazer %d z rowerem czeka na peronie", id);
            entrance = get_message_queue(".", 1);
            entrance_message->ktype = getpid();
            entrance_message->mtype = 1;
            sem_wait(platform_sem, 1);
            send_message(entrance, entrance_message);
            printf("\nPasazer: Pasazer %d z rowerem czeka u progu", id);
            log_to_file("\nPasazer: Pasazer %d z rowerem czeka u progu", id);
            if (reset) {
                reset = 0;
                printf("\nPasazer %d wraca na peron. 1", id);
                log_to_file("\nPasazer %d wraca na peron. 1", id);
                continue;
            }
            if (sem_wait_interruptible(entrance_sem, 1) == 0) {
                printf("\nPasazer %d wraca na peron. 2", id);
                log_to_file("\nPasazer %d wraca na peron. 2", id);
                reset = 0;
                continue;
            }
            entrance_message->mtype = getpid();
            send_message(entrance, entrance_message);
            break;
        }
    } else {
        while (1) {
            printf("\nPasazer: Pasazer %d czeka na peronie", id);
            log_to_file("\nPasazer: Pasazer %d czeka na peronie", id);
            entrance = get_message_queue(".", 0);
            entrance_message->ktype = getpid();
            entrance_message->mtype = 1;
            sem_wait(platform_sem, 0);
            send_message(entrance, entrance_message);
            printf("\nPasazer: Pasazer %d czeka u progu", id);
            log_to_file("\nPasazer: Pasazer %d czeka u progu", id);
            if (reset) {
                reset = 0;
                printf("\nPasazer %d wraca na peron. 4", id);
                log_to_file("\nPasazer %d wraca na peron. 4", id);
                continue;
            }
            if (sem_wait_interruptible(entrance_sem, 0) == 0) {
                printf("\nPasazer %d wraca na peron. 5", id);
                log_to_file("\nPasazer %d wraca na peron. 5", id);
                reset = 0;
                continue;
            }
            entrance_message->mtype = getpid();
            send_message(entrance, entrance_message);
            break;
        }
    }

    //wsiada do pociągu
    printf("\nPasazer: Pasazer %d wsiadł do pociągu", id);
    log_to_file("\nPasazer: Pasazer %d wsiadł do pociągu", id);

    //czeka na dotarcie do celu
    while (1) {
        usleep(INTERVAL_TIME*TIME_SCALE);
    }

    return 0;
}