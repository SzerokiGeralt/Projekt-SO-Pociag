#include "mojeFunkcje.h"
#define MAX_KIEROWNIKOW 5

void handle_sigint();
pid_t zawiadowca_pid;
pid_t kierownik_pid[MAX_KIEROWNIKOW];

int main() {
    setbuf(stdout, NULL);
    signal(2, handle_sigint);

    printf("Uruchomiono zaawansowana symulacje kolejowa\nstworzona przez Karol Kapusta.\nProject not sponsored by \"Koleje Malopolskie\"\n\n");

    // Uruchomienie procesu zawiadowcy stacji
    if ((zawiadowca_pid = fork()) == 0) {
        execl("./zawiadowca", "zawiadowca", NULL);
        perror("Nie udalo sie uruchomic procesu zawiadowca");
        exit(1);
    }

    sleep(1);

    // Uruchomienie procesów kierownika pociągu
    for (int i = 0; i < MAX_KIEROWNIKOW; i++) {
        if ((kierownik_pid[i] = fork()) == 0) {
            execl("./kierownik", "kierownik", NULL);
            perror("Nie udalo sie uruchomic procesu kierownik");
            exit(1);
        }
    }

    sleep(1);

    // Uruchomienie procesów pasażerów
    while (1) {
        if (fork() == 0) {
            execl("./pasazer", "pasazer", NULL);
            perror("Nie udalo sie uruchomic procesu pasazer");
            exit(1);
        }
        sleep(2);
    }

    // Oczekiwanie na zakończenie procesów podrzędnych
    while (wait(NULL) > 0);

    printf("\nMaster: Wszystkie procesy zakonczone.");
    return 0;
}

// Obsługa sygnału SIGINT
void handle_sigint(int sig) {
    printf("\nMaster: Odebrano sygnal SIGINT");

    // Usuwanie zasobów IPC
    destroy_message_queue(get_message_queue(".", 0));
    destroy_message_queue(get_message_queue(".", 1));
    destroy_message_queue(get_message_queue(".", 2));

    sem_destroy(sem_get(".", 1, 2));
    sem_destroy(sem_get(".", 2, 2));

    exit(0);
}