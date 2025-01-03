#include "mojeFunkcje.h"


int main(int argc, char *argv[]) {
    int id = getpid();
    int hasBike = rand() % 2;

    printf("Nowy pasazer PID: %d\n",getpid());

    return 0;
}