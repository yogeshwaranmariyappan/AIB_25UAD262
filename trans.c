// Bank-account program reads a random-access file sequentially,
// updates data already written to the file, creates new data to
// be placed in the file, and deletes data previously in the file.
#include <stdio.h>
#include <stdlib.h>

#define MAX_ACCOUNTS 100 // maximum number of accounts

// clientData structure definition
typedef struct clientData
{
    unsigned int acctNum; // account number
    char lastName[15];    // account last name
    char firstName[10];   // account first name
    double balance;       // account balance
} ClientData;             // end structure clientData

// prototypes
unsigned int enterChoice(void);
void textFile(FILE *readPtr);
void updateRecord(FILE *fPtr);
void newRecord(FILE *fPtr);
void deleteRecord(FILE *fPtr);
void clearInputBuffer();

int main(int argc, char *argv[])
{
    (void)argc; // suppress unused parameter warning
    FILE *cfPtr;         // credit.dat file pointer
    unsigned int choice; // user's choice

    // fopen opens the file in read/write binary mode
    if ((cfPtr = fopen("credit.dat", "rb+")) == NULL)
    {
        // If it doesn't exist, create it in write/read binary mode
        if ((cfPtr = fopen("credit.dat", "wb+")) == NULL) {
            fprintf(stderr, "%s: File could not be opened or created.\n", argv[0]);
            exit(-1);
        }
        
        // Initialize with MAX_ACCOUNTS blank records
        ClientData blankClient = {0, "", "", 0.0};
        for (unsigned int i = 1; i <= MAX_ACCOUNTS; ++i) {
            if (fwrite(&blankClient, sizeof(ClientData), 1, cfPtr) != 1) {
                fprintf(stderr, "Error: Could not initialize records.\n");
                fclose(cfPtr);
                exit(-1);
            }
        }
        rewind(cfPtr); // Go back to start
    }

    // enable user to specify action
    while ((choice = enterChoice()) != 5)
    {
        switch (choice)
        {
        // create text file from record file
        case 1:
            textFile(cfPtr);
            break;
        // update record
        case 2:
            updateRecord(cfPtr);
            break;
        // create record
        case 3:
            newRecord(cfPtr);
            break;
        // delete existing record
        case 4:
            deleteRecord(cfPtr);
            break;
        // display if user does not select valid choice
        default:
            puts("Incorrect choice");
            break;
        } // end switch
    }     // end while

    fclose(cfPtr); // fclose closes the file
    return 0;
} // end main

// create formatted text file for printing
void textFile(FILE *readPtr)
{
    FILE *writePtr; // accounts.txt file pointer
    // create clientData with default information
    ClientData client = {0, "", "", 0.0};

    // fopen opens the file; exits if file cannot be opened
    if ((writePtr = fopen("accounts.txt", "w")) == NULL)
    {
        fprintf(stderr, "File could not be opened.\n");
    } // end if
    else
    {
        rewind(readPtr); // sets pointer to beginning of file
        fprintf(writePtr, "%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");

        // copy all records from random-access file into text file
        while (fread(&client, sizeof(ClientData), 1, readPtr) == 1)
        {
            // write single record to text file
            if (client.acctNum != 0)
            {
                fprintf(writePtr, "%-6d%-16s%-11s%10.2f\n", client.acctNum, client.lastName, client.firstName,
                        client.balance);
            } // end if
        }     // end while

        fclose(writePtr); // fclose closes the file
    }                     // end else
} // end function textFile

// update balance in record
void updateRecord(FILE *fPtr)
{
    unsigned int account; // account number
    double transaction;   // transaction amount
    // create clientData with no information
    ClientData client = {0, "", "", 0.0};

    // obtain number of account to update
    printf("Enter account to update ( 1 - %d ): ", MAX_ACCOUNTS);
    while (scanf("%u", &account) != 1 || account < 1 || account > MAX_ACCOUNTS) {
        printf("Invalid input. Enter a valid account number ( 1 - %d ): ", MAX_ACCOUNTS);
        clearInputBuffer();
    }

    // move file pointer to correct record in file
    if (fseek(fPtr, (long)(account - 1) * sizeof(ClientData), SEEK_SET) != 0) {
        fprintf(stderr, "Error: Could not seek to account #%u.\n", account);
        return;
    }
    // read record from file
    if (fread(&client, sizeof(ClientData), 1, fPtr) != 1) {
        fprintf(stderr, "Error: Could not read account #%u.\n", account);
        return;
    }
    // display error if account does not exist
    if (client.acctNum == 0)
    {
        printf("Account #%d has no information.\n", account);
    }
    else
    { // update record
        printf("%-6d%-16s%-11s%10.2f\n\n", client.acctNum, client.lastName, client.firstName, client.balance);

        // request transaction amount from user
        printf("%s", "Enter charge ( + ) or payment ( - ): ");
        while (scanf("%lf", &transaction) != 1) {
            printf("Invalid input. Enter charge ( + ) or payment ( - ): ");
            clearInputBuffer();
        }

        // warn if balance would go negative
        if (client.balance + transaction < 0) {
            printf("Warning: This will result in a negative balance (%.2f).\n",
                   client.balance + transaction);
            printf("Proceed? (y/n): ");
            clearInputBuffer();
            int ch = getchar();
            if (ch != 'y' && ch != 'Y') {
                printf("Transaction cancelled.\n");
                return;
            }
        }

        client.balance += transaction; // update record balance

        printf("%-6d%-16s%-11s%10.2f\n", client.acctNum, client.lastName, client.firstName, client.balance);

        // move file pointer to correct record in file
        // move back by 1 record length
        if (fseek(fPtr, -(long)sizeof(ClientData), SEEK_CUR) != 0) {
            fprintf(stderr, "Error: Could not seek to update record.\n");
            return;
        }
        // write updated record over old record in file
        if (fwrite(&client, sizeof(ClientData), 1, fPtr) != 1) {
            fprintf(stderr, "Error: Could not write updated record.\n");
            return;
        }
    } // end else
} // end function updateRecord

// delete an existing record
void deleteRecord(FILE *fPtr)
{
    ClientData client;                       // stores record read from file
    ClientData blankClient = {0, "", "", 0}; // blank client
    unsigned int accountNum;                        // account number

    // obtain number of account to delete
    printf("Enter account number to delete ( 1 - %d ): ", MAX_ACCOUNTS);
    while (scanf("%u", &accountNum) != 1 || accountNum < 1 || accountNum > MAX_ACCOUNTS) {
        printf("Invalid input. Enter a valid account number ( 1 - %d ): ", MAX_ACCOUNTS);
        clearInputBuffer();
    }

    // move file pointer to correct record in file
    if (fseek(fPtr, (long)(accountNum - 1) * sizeof(ClientData), SEEK_SET) != 0) {
        fprintf(stderr, "Error: Could not seek to account #%u.\n", accountNum);
        return;
    }
    // read record from file
    if (fread(&client, sizeof(ClientData), 1, fPtr) != 1) {
        fprintf(stderr, "Error: Could not read account #%u.\n", accountNum);
        return;
    }
    // display error if record does not exist
    if (client.acctNum == 0)
    {
        printf("Account %d does not exist.\n", accountNum);
    } // end if
    else
    { // delete record
        // show record details before confirming
        printf("Account #%d: %s %s, Balance: %.2f\n",
               client.acctNum, client.firstName, client.lastName, client.balance);
        printf("Are you sure you want to delete this account? (y/n): ");
        clearInputBuffer();
        int ch = getchar();
        if (ch != 'y' && ch != 'Y') {
            printf("Deletion cancelled.\n");
            return;
        }

        // move file pointer to correct record in file
        if (fseek(fPtr, (long)(accountNum - 1) * sizeof(ClientData), SEEK_SET) != 0) {
            fprintf(stderr, "Error: Could not seek to delete record.\n");
            return;
        }
        // replace existing record with blank record
        if (fwrite(&blankClient, sizeof(ClientData), 1, fPtr) != 1) {
            fprintf(stderr, "Error: Could not delete record.\n");
            return;
        }
        printf("Account #%d deleted successfully.\n", accountNum);
    } // end else
} // end function deleteRecord

// create and insert record
void newRecord(FILE *fPtr)
{
    // create clientData with default information
    ClientData client = {0, "", "", 0.0};
    unsigned int accountNum; // account number

    // obtain number of account to create
    printf("Enter new account number ( 1 - %d ): ", MAX_ACCOUNTS);
    while (scanf("%u", &accountNum) != 1 || accountNum < 1 || accountNum > MAX_ACCOUNTS) {
        printf("Invalid input. Enter a valid account number ( 1 - %d ): ", MAX_ACCOUNTS);
        clearInputBuffer();
    }

    // move file pointer to correct record in file
    if (fseek(fPtr, (long)(accountNum - 1) * sizeof(ClientData), SEEK_SET) != 0) {
        fprintf(stderr, "Error: Could not seek to account #%u.\n", accountNum);
        return;
    }
    // read record from file
    if (fread(&client, sizeof(ClientData), 1, fPtr) != 1) {
        fprintf(stderr, "Error: Could not read account #%u.\n", accountNum);
        return;
    }
    // display error if account already exists
    if (client.acctNum != 0)
    {
        printf("Account #%d already contains information.\n", client.acctNum);
    } // end if
    else
    { // create record
        // user enters last name, first name and balance
        printf("%s", "Enter lastname, firstname, balance\n? ");
        while (scanf("%14s%9s%lf", client.lastName, client.firstName, &client.balance) != 3) {
            printf("Invalid input. Please enter lastname, firstname, balance\n? ");
            clearInputBuffer();
        }

        client.acctNum = accountNum;
        // move file pointer to correct record in file
        if (fseek(fPtr, (long)(client.acctNum - 1) * sizeof(ClientData), SEEK_SET) != 0) {
            fprintf(stderr, "Error: Could not seek to insert record.\n");
            return;
        }
        // insert record in file
        if (fwrite(&client, sizeof(ClientData), 1, fPtr) != 1) {
            fprintf(stderr, "Error: Could not write new record.\n");
            return;
        }
    } // end else
} // end function newRecord

// enable user to input menu choice
unsigned int enterChoice(void)
{
    unsigned int menuChoice; // variable to store user's choice
    // display available options
    printf("%s", "\nEnter your choice\n"
                 "1 - store a formatted text file of accounts called\n"
                 "    \"accounts.txt\" for printing\n"
                 "2 - update an account\n"
                 "3 - add a new account\n"
                 "4 - delete an account\n"
                 "5 - end program\n? ");

    while (scanf("%u", &menuChoice) != 1) {
        printf("Invalid input. Please enter a valid number.\n? ");
        clearInputBuffer();
    } // receive choice from user
    return menuChoice;
} // end function enterChoice

// Helper function to clear input buffer
void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}