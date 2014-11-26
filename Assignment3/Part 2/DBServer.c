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

void encrypt(char x[], char encryption[]);
void decrypt(char x[], char decryption[]);
void dbServer(struct account accounts[], int toDBServer_message, int dbServerTOatm_message, int atmProcess);
int checkPin(struct account accounts[], char accountNo[], char pin[]);
int search(struct account accounts[], char accountNo[]);
void writeFile(FILE *file, struct account accounts[]);


int main (void){

    struct account accounts[3];
    strcpy(accounts[0].accountNo, "00001");
    strcpy(accounts[0].pin, "218");
    accounts[0].funds = 3443.22;
    accounts[0].numOfFailedAttempts = 0;

    strcpy(accounts[1].accountNo, "00011");
    strcpy(accounts[1].pin, "434");
    accounts[1].funds = 10089.97;
    accounts[1].numOfFailedAttempts = 0;

    strcpy(accounts[2].accountNo, "00117");
    strcpy(accounts[2].pin, "360");
    accounts[2].funds = 112.00;
    accounts[2].numOfFailedAttempts = 0;

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
        dbServer(accounts, toDBServer_message,dbServerTOatm_message,2);

    }


}
int search(struct account accounts[], char accountNo[])
{
    int size = sizeof(struct account) / sizeof(accounts) + 1;
    int i;
    for(i = 0; i < size; i++)
    {
        if (!strcmp(accounts[i].accountNo, accountNo))
            return i;
    }
    return -1;
}

void writeFile(FILE *file, struct account accounts[])
{
// open file
    file = fopen("db.txt","w");
    int i;
    int size = sizeof(struct account) / sizeof(accounts) + 1;
    for(i = 0; i < size; i++)
        fprintf(file, "%s,%s,%f\n", accounts[i].accountNo, accounts[i].pin, accounts[i].funds);
    fclose(file);
}

int checkPin(struct account accounts[], char accountNo[], char pin[])
{
    int index = search(accounts, accountNo);
    if (index != -1)
    {
        char encTest[3];
        decrypt(accounts[index].pin, encTest);

        if (!strcmp(encTest, pin))
            return 1;
        return 0;
    }
    else
        printf("Account Num does not exist.\n");
    return index;
}

void dbServer(struct account accounts[], int toDBServer_message, int dbServerTOatm_message, int atmProcess)
{
    FILE *file = NULL;
    struct alltypesmessage messages;
    while (1)
    {
        int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
        if(msgrcv(toDBServer_message, &messages, msgLength, 1, 0) == -1)
        {
            perror("receiving message failed.");
            exit(1);
        }
// check what the message contains
        else
        {
            printf("Recieved message.\n");
// if it is pin message

            if (!strcmp("PIN", messages.message))
            {
                //get the index of the account list
                int index = search(accounts, messages.accountNo);
                //temp char to see if it is a blocked account
                char temp[6];
                //store what was passed in to temp
                strcpy(temp,messages.accountNo);
                //replace the first char with X
                temp[0] = 'X';
                //If it is not a blocked
                if(search(accounts,temp)==-1)
                {


                    int correctPin = checkPin(accounts, messages.accountNo, messages.pin);
// check correctPin
                    if (correctPin == -1)
                    {
                        printf("Wrong account gotten from atm in server\n");
                        strcpy(messages.message, "NOT OK");
                    }
                    else if (correctPin == 0)
                    {
                        //int index = search(accounts, messages.accountNo);

                        if(index != -1)
                        {
                            if(accounts[index].numOfFailedAttempts==2)
                            {
                                accounts[index].accountNo[0] = 'X';
                                writeFile(file, accounts);

                                strcpy(messages.message, "Blocked");
                                messages.message_type = atmProcess;
                                msgLength = sizeof(struct alltypesmessage) - sizeof(long);
                                if (msgsnd(dbServerTOatm_message, &messages, msgLength, 0) == -1)
                                {
                                    perror("sending message failed.");
                                    exit(1);
                                }

                                else
                                {
                                    printf("Account Blocked!");
                                }

                            }
                            else
                            {
                                accounts[index].numOfFailedAttempts++;
                            }

                        }

                        printf("Incorrect Pin gotten from atm in server\n");
                        strcpy(messages.message, "NOT OK");
                    }
// if correct send ok message to atm
                    else if (correctPin == 1)
                    {
                        printf("Correct Pin gotten from atm in server\n");
                        strcpy(messages.message, "OK");
                    }
                    messages.message_type = atmProcess;
                    msgLength = sizeof(struct alltypesmessage) - sizeof(long);
                    if (msgsnd(dbServerTOatm_message, &messages, msgLength, 0) == -1)
                    {
                        perror("sending message failed.");
                        exit(1);
                    }
                    else
                    {
                        printf("%s\n", messages.message);
                    }
// otherwise
                }

                else
                {
                    printf("AccountBlocked\n");


                }
            }

// if it is a withdraw message
            if(!strcmp("WITHDRAW", messages.message))
            {
                int index = search(accounts, messages.accountNo);
                if (index != -1)
                {
                    if (accounts[index].funds - messages.funds >= 0)
                    {
                        accounts[index].funds -= messages.funds;
                        writeFile(file, accounts);
                        strcpy(messages.message, "Enough");
//messages.funds = accounts[index].funds;
                    }
                    else
                    {
                        strcpy(messages.message, "Not Enough");
                    }
                }
                else
                {
                    printf("Account does not exist.\n");
                    strcpy(messages.message, "Funds not available");
                }
                messages.message_type = atmProcess;
                msgLength = sizeof(struct alltypesmessage) - sizeof(long);
                if (msgsnd(dbServerTOatm_message, &messages, msgLength, 0) == -1)
                {
                    perror("sending message failed.");
                    exit(1);
                }
                else
                {
                    printf("%s\n", messages.message);
                }
            }
// if it is a request funds message
            if(!strcmp("REQUEST FUNDS", messages.message))
            {
                int index = search(accounts, messages.accountNo);
                if (index != -1)
                {
                    messages.funds = accounts[index].funds;
                    strcpy(messages.message, "Funds available");
                }
                else
                {
                    printf("Account does not exist.\n");
                    strcpy(messages.message, "Funds not available");
                }
                messages.message_type = atmProcess;
                msgLength = sizeof(struct alltypesmessage) - sizeof(long);
                if (msgsnd(dbServerTOatm_message, &messages, msgLength, 0) == -1)
                {
                    perror("sending message failed.");
                    exit(1);
                }
                else
                {
                    printf("Server %s\n", messages.message);
                }
            }
// if it is a update DB message
            if (!strcmp("Update DB", messages.message))
            {
// find the appropriate index of the account to update
                int index = search(accounts, messages.accountNo);
                if (index != -1)
                {
// update the account
                    strcpy(accounts[index].accountNo, messages.accountNo);
                    char encTest[3];
                    encrypt(messages.pin,encTest);
                    strcpy(accounts[index].pin, encTest);
                    accounts[index].funds = messages.funds;
//save to file
                    writeFile(file, accounts);
                }
                else
                {
                    printf("\nAccount Number does not exist.\n");
                }
            }

            if (!strcmp("EXIT", messages.message))
            {
                printf("Exiting.\n");
                exit(1);
            }
        }
    }
}

void encrypt(char x[], char encryption[]){
    int i;
    for(i =0; i< 3; i++){
        if(x[i]=='9'){
          encryption[i]='0';
        }
        else{
            encryption[i] = x[i]+1;
        }
    }

}

void decrypt(char x[], char decryption[]){
    int i;
    for(i =0; i< 3; i++){
         if(x[i]=='0'){
          decryption[i]='9';
        }
        else{
            decryption[i] = x[i]-1;
        }
    }

}
