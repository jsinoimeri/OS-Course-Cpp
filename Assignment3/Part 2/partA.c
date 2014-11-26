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
key_t toDBServer_key = (key_t)1235,
                       dbServerTOatm_key = (key_t)1236;
void dbServer(struct account accounts[], int toDBServer_message, int dbServerTOatm_message, int atmProcess);
int search(struct account accounts[3], char accountNo[]);
void readFILE(void);
void writeFile(FILE *file, struct account accounts[]);
void dbEditor(int toDBServer_message, int dbServerProcess);
void atm(int toDBServer_message, int dbServerTOatm_message, int dbServerProcess);
int checkPin(struct account accounts[], char accountNo[], char pin[]);
void atm(int toDBServer_message, int dbServerTOatm_message, int dbServerProcess);
int main(void)
{
    struct account accounts[3];
    strcpy(accounts[0].accountNo, "00001");
    strcpy(accounts[0].pin, "107");
    accounts[0].funds = 3443.22;
    accounts[0].numOfFailedAttempts = 0;

    strcpy(accounts[1].accountNo, "00011");
    strcpy(accounts[1].pin, "323");
    accounts[1].funds = 10089.97;
    accounts[1].numOfFailedAttempts = 0;

    strcpy(accounts[2].accountNo, "00117");
    strcpy(accounts[2].pin, "259");
    accounts[2].funds = 112.00;
    accounts[2].numOfFailedAttempts = 0;

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
    
    else
    {
        int dbEditorProcess = fork();
        
        if (dbEditorProcess == 0)
        {
						//dbEditor(toDBServer_message, getppid());
            atm(toDBServer_message, dbServerTOatm_message, getppid());
        }
				// parent
        
        else
        {
            dbServer(accounts, toDBServer_message, dbServerTOatm_message, dbEditorProcess);

        }
    }
    
    
    return 0;
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


void readFILE(void)
{
}


int checkPin(struct account accounts[], char accountNo[], char pin[])
{
    int index = search(accounts, accountNo);
    if (index != -1)
    {
        if (!strcmp(accounts[index].pin, pin))
            return 1;
        return 0;
    }
    else
        printf("Account Num does not exist.\n");
    return index;
}


void writeFile(FILE *file, struct account accounts[])
{
		// open file
    file = fopen("db.txt","w");
    
    int size = sizeof(struct account) / sizeof(accounts) + 1;
    int i;
    
    for(i = 0; i < size; i++)
        fprintf(file, "%s,%s,%f\n", accounts[i].accountNo, accounts[i].pin, accounts[i].funds);
    
    //close
    fclose(file);
}


void dbServer(struct account accounts[], int toDBServer_message, int dbServerTOatm_message, int atmProcess)
{
    FILE *file = NULL;
    struct alltypesmessage messages;
    
    while (1)
    {
        int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
        
        if(msgrcv(toDBServer_message, &messages, msgLength, getpid(), 0) == -1)
        {
            perror("receiving message failed.");
            exit(1);
        }
        
				// check what the message contains
        else
        {
        	
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
                                printf("Account Blocked!");
                                accounts[index].accountNo[0] = 'X';
                                writeFile(file, accounts);

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
                }

                else
                {
                    printf("AccountBlocked\n");
                    strcpy(messages.message, "NOT OK");
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
                    printf("Message sent to atm from server with ok\n");
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
                    printf("Message sent to atm from server with funds\n");
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
                    strcpy(accounts[index].pin, messages.pin);
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
        
        if(!strcmp(acNo,"X\0"))
        {       		
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
        if(msgrcv(dbServerTOatm_message, &messages, msgLength, getpid(), 0) == -1)
        {
            perror("receiving message failed.");
            exit(1);
        }
        else
        {
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
                    if(msgrcv(dbServerTOatm_message, &messages, msgLength, getpid(), 0) == -1)
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

        }
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
