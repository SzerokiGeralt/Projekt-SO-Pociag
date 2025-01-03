#include "mojeFunkcje.h"

int main() {
    setbuf(stdout, NULL);
    printf("\nNowy zawiadowca stacji PID: %d",getpid());

    int platform_sem = sem_create(".", 1, 2);
    sem_set_value(platform_sem, 0, 1);
    sem_set_value(platform_sem, 1, 1);

    

    while (1)
    {
        sleep(1);
    }
    
    return 0;
}