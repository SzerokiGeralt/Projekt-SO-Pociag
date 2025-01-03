#include "mojeFunkcje.h"

void handle_sigint(int sig) {
    printf("\nOdebrano sygnal SIGINT (CTRL+C). Zamykam program...\n{{{DO IMPLEMENTACJI}}}\n");
    exit(0);
}

int main() {
    pid_t zawiadowca_pid, kierownik_pid;
    signal(2, handle_sigint);

    printf("Uruchomiono zaawansowana symulacje kolejowa\nstworzona przez Karol Kapusta.\nProject not sponsored by \"Koleje Malopolskie\"\n\n");

    // Uruchomienie procesu zawiadowcy stacji
    if ((zawiadowca_pid = fork()) == 0) {
        execl("./zawiadowca", "zawiadowca", NULL);
        perror("Nie udalo sie uruchomic procesu zawiadowca");
        exit(1);
    }


    // Uruchomienie procesu kierownika pociągu
    if ((kierownik_pid = fork()) == 0) {
        execl("./kierownik", "kierownik", NULL);
        perror("Nie udalo sie uruchomic procesu kierownik");
        exit(1);
    }

    // Uruchomienie procesów pasażerów
    for (int i = 0; i < 15; i++) {
        if (fork() == 0) {
            execl("./pasazer", "pasazer", NULL);
            perror("Nie udalo sie uruchomic procesu pasazer");
            exit(1);
        }
    }

    // Oczekiwanie na zakończenie procesów podrzędnych
    while (wait(NULL) > 0);

    printf("Master: Wszystkie procesy zakonczone.\n");
    return 0;
}