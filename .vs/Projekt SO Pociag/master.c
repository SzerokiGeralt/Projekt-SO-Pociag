#include "mojeFunkcje.h"

//Obsługa sygnału SIGINT
void handle_sigint(int sig) {
    printf("\nOdebrano sygnal SIGINT (CTRL+C). Zamykam program...\n{{{DO IMPLEMENTACJI}}}\n");
    sem_destroy(sem_get(".", 1, 2));
    sem_destroy(sem_get(".", 2, 2));
    exit(0);
}

int main() {
    setbuf(stdout, NULL);
    pid_t zawiadowca_pid, kierownik_pid;
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
    for (int i = 0; i < 5; i++) {
        if ((kierownik_pid = fork()) == 0) {
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

    printf("Master: Wszystkie procesy zakonczone.\n");
    return 0;
}