#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#define QUEUE_KEY   0x0020c8e9
#define MAX_SIZE    1024
#define MSG_STOP    "exit"
struct message {
    long mtype;
    long ktype;
};
int receive_message_no_wait(int msq_ID, struct message *msg) {
    while (1) {
        if (msgrcv(msq_ID, msg, sizeof(*msg) - sizeof(long), 0, IPC_NOWAIT) == -1) {
            if (errno == EINTR) {
                // Przerwane przez sygnał – ponawiamy
                continue;
            } else if (errno == ENOMSG) {
                // Brak komunikatu
                return 0;
            } else {
                // Błąd
                perror("Blad recieve_message_no_wait");
                return -1;
            }
        }
        // Sukces
        return 1;
    }
}

int get_message_queue(const char *path, int proj_ID) {
    key_t key = ftok(path, proj_ID);
    if (key == -1) {
        perror("Blad key_message_queue");
        return -1;
    }

    int msq_ID = msgget(key, IPC_CREAT | 0600);
    if (msq_ID == -1) {
        perror("Blad get_message_queue");
        return -1;
    }

    return msq_ID;
}


int main() {
    
    int msgid;
    struct message msg;
    int must_stop = 0;


    for(int i = 0 ; i<4 ; i++) {
        msgid = get_message_queue(".",i);
        printf("Listening for messages on queue with key: %d\n", QUEUE_KEY);
        while (receive_message_no_wait(msgid,&msg) == 1) {
            switch (i) {
                case 0:
                    printf("pasazerowie bez rowera\n");
                    break;
                case 1:
                    printf("pasazer z rowerem\n");
                    break;
                case 2:
                    printf("kolejka global pociagi\n");
                    break;
                case 3:
                    printf("kolejka prywatna pociagi\n");
                    break;
                default:
                    printf("Received message on unknown queue\n");
                    break;
            }
             printf("message: mtype %ld ktype %ld\n", msg.mtype, msg.ktype);   
        }
    }

    return 0;
}
