#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

struct account
{
    char accountNo[6];
    char pin[4];
    float funds;
    int numOfFailedAttempts;
};
struct alltypesmessage
{
    long message_type;
    char accountNo[6];
    char pin[4];
    float funds;
    char message[100];
};

void dbEditor(int toDBServer_message, int dbServerProcess);

int main (void){


key_t toDBServer_key = (key_t)1235;

 int toDBServer_message = msgget(toDBServer_key, IPC_CREAT|0600);

    if (toDBServer_message == -1)
    {
        perror("toDBServer_message failed");
        exit(1);
    }
    else{
        dbEditor(toDBServer_message,1);

    }
}

void dbEditor(int toDBServer_message, int dbServerProcess)
{
    char acNo[6];
    char pin[4];
    float funds;
    while (1)
    {
         struct alltypesmessage messages;
        messages.message_type = dbServerProcess;

        printf("Enter account number or X to quit: ");
        scanf("%s", acNo);
        if(!strcmp(acNo,"X")){
            strcpy(messages.message, "EXIT");
            int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
            msgsnd(toDBServer_message, &messages, msgLength, 0);
           exit(1);
        }
        printf("Enter pin or X to quit: ");
        scanf("%s", pin);
        if(!strcmp(pin,"X")){
            strcpy(messages.message, "EXIT");
            int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
            msgsnd(toDBServer_message, &messages, msgLength, 0);
           exit(1);
        }
        printf("Enter funds: ");
        scanf("%f", &funds);

        strncpy(messages.accountNo, acNo, 5);
        messages.accountNo[5] = '\0';
        strncpy(messages.pin, pin, 3);
        messages.pin[3] = '\0';
        messages.funds = funds;
        strcpy(messages.message, "Update DB");
        int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
        if (msgsnd(toDBServer_message, &messages, msgLength, 0) == -1)
        {
            perror("sending message failed.");
            exit(1);
        }
        else
        {
            printf("Message sent to server\n");
        }
    }
}

