#include "mojeFunkcje.h"

void handle_sigint();
pid_t zawiadowca_pid;

void handle_sigchld(int sig) {
    int status;
    pid_t pid;

    // Pętla odbierająca wszystkie zakończone procesy potomne
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    }
}
int main() {
    // Usuwanie wszystkich plików log*.txt
    system("rm -f log*.txt");
    
    setbuf(stdout, NULL);
    signal(SIGINT, handle_sigint);
    signal(SIGCHLD, handle_sigchld);

    printf("Uruchomiono zaawansowana symulacje kolejowa\nstworzona przez Karol Kapusta.\nProject not sponsored by \"Koleje Malopolskie\"\n\n");

    // Uruchomienie procesów pasażerów
    for (int i = 0; i < SPAWN_PASSANGERS ; i++) {
        if (fork() == 0) {
            execl("./pasazer", "pasazer", NULL);
            perror("Nie udalo sie uruchomic procesu pasazer");
            exit(1);
        }
        usleep(rand() % PASSANGER_SPAWNRATE * TIME_SCALE);
    }

    // Uruchomienie procesów kierownika pociągu
    for (int i = 0; i < MAX_TRAINS ; i++) {
        if (fork() == 0) {
            execl("./kierownik", "kierownik", NULL);
            perror("Nie udalo sie uruchomic procesu kierownik");
            exit(1);
        }
    }

    // Uruchomienie procesu zawiadowcy stacji
    if ((zawiadowca_pid = fork()) == 0) {
        execl("./zawiadowca", "zawiadowca", NULL);
        perror("Nie udalo sie uruchomic procesu zawiadowca");
        exit(1);
    }




    // Oczekiwanie na zakończenie procesów podrzędnych
    while (wait(NULL) > 0);

    printf("\nMaster: Wszystkie procesy zakonczone.");
    return 0;
}

// Obsługa sygnału SIGINT
void handle_sigint(int sig) {
    printf("\nMaster: Odebrano sygnal SIGINT");
    exit(0);
}