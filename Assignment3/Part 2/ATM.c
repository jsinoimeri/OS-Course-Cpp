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

void atm(int toDBServer_message, int dbServerTOatm_message, int dbServerProcess);

int main (void){
key_t toDBServer_key = (key_t)1235,
                       dbServerTOatm_key = (key_t)1236;
 int toDBServer_message = msgget(toDBServer_key, IPC_CREAT|0600);
    int dbServerTOatm_message = msgget(dbServerTOatm_key, IPC_CREAT|0600);
    if (toDBServer_message == -1)
    {
        perror("toDBServer_message failed");
        exit(1);
    }
    if (dbServerTOatm_message == -1)
    {
        perror("dbServerTOatm_message failed");
        exit(1);
    }
    else{
        atm(toDBServer_message,dbServerTOatm_message,1);

    }


}

void atm(int toDBServer_message, int dbServerTOatm_message, int dbServerProcess)
{
    char acNo[6];
    char pin[4];
    char request[1];
    float funds;
    while (1)
    {
        struct alltypesmessage messages;
        messages.message_type = dbServerProcess;

// ask user for input
        printf("Enter account number or X to exit: ");
        scanf("%s", acNo);
        if(!strcmp(acNo,"X\0")){
            strcpy(messages.message, "EXIT");
            int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
            msgsnd(toDBServer_message, &messages, msgLength, 0);
           exit(0);
        }
        printf("Enter pin or X to exit: ");
        scanf("%s", pin);
        if(!strcmp(pin,"X\0"))
        {
           strcpy(messages.message, "EXIT");
           int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
           msgsnd(toDBServer_message, &messages, msgLength, 0);
           exit(0);
        }
// create message to send to to dbServer
        strncpy(messages.accountNo, acNo, 5);
        messages.accountNo[5] = '\0';
        strncpy(messages.pin, pin, 3);
        messages.pin[3] = '\0';
        strcpy(messages.message, "PIN");
// send message to dbServer
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
// receive ok message
        if(msgrcv(dbServerTOatm_message, &messages, msgLength, 2, 0) == -1)
        {
            perror("receiving message failed.");
            exit(1);
        }
        else
        {
            printf("%s\n", messages.message);

            if (!strcmp("OK", messages.message))
            {
// ask user for input such as withdraw or display funds
                printf("Enter a request(W for widthdraw, R for request funds): ");
                scanf("%s", request);
                if (!strcmp("W", request))
                {
                    printf("Enter funds: ");
                    scanf("%f", &funds);
                    messages.funds = funds;
                    strcpy(messages.message, "WITHDRAW");
                }
                else
                {
                    strcpy(messages.message, "REQUEST FUNDS");
                }
// create message to send to dbServer
                messages.message_type = dbServerProcess;
                msgLength = sizeof(struct alltypesmessage) - sizeof(long);
                if (msgsnd(toDBServer_message, &messages, msgLength, 0) == -1)
                {
                    perror("sending message failed.");
                    exit(1);
                }
            else
                {
                    printf("Request message sent to server\n");
// receive ok message
                    if(msgrcv(dbServerTOatm_message, &messages, msgLength, 2, 0) == -1)
                    {
                        perror("receiving message failed.");
                        exit(1);
                    }
                    else
                    {
                        if (!strcmp(messages.message, "Funds available"))
                        {
                            printf("Account No: %s Funds: %f\n", messages.accountNo, messages.funds);
                        }
                        else if (!strcmp(messages.message, "Enough"))
                        {
                            printf("Enough funds.\n");
                        }
                        else if(!strcmp(messages.message, "Not Enough"))
                        {
                            printf("Not enough funds.\n");
                        }


                    }
                }
            }

            else if(!strcmp(messages.message, "Blocked"))
             {
                printf("This account has been blocked access denied.\n");
             }

        }
    }
}
