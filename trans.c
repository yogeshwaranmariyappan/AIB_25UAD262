// Bank-account program reads a random-access file sequentially,
// updates data already written to the file, creates new data to
// be placed in the file, and deletes data previously in the file.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ACCOUNTS 100  // maximum number of accounts
#define LAST_NAME_LEN 15  // max length of last name
#define FIRST_NAME_LEN 10 // max length of first name

// clientData structure definition
typedef struct clientData
{
    unsigned int acctNum;          // account number
    char lastName[LAST_NAME_LEN];  // account last name
    char firstName[FIRST_NAME_LEN]; // account first name
    double balance;                // account balance
} ClientData;                      // end structure clientData

// prototypes
unsigned int enterChoice(void);  // display menu and get user choice
void textFile(FILE *readPtr);    // export all records to text file
void updateRecord(FILE *fPtr);   // update balance of existing record
void newRecord(FILE *fPtr);      // create a new account record
void deleteRecord(FILE *fPtr);   // delete an existing account record
void viewRecord(FILE *fPtr);     // view a single account record
void searchByName(FILE *fPtr);   // search records by last name
void clearInputBuffer(void);     // clear stdin input buffer

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
    while ((choice = enterChoice()) != 7)
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
        // view single record
        case 5:
            viewRecord(cfPtr);
            break;
        // search by last name
        case 6:
            searchByName(cfPtr);
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

// View a single account record by account number
// @param fPtr - pointer to the binary data file
void viewRecord(FILE *fPtr)
{
    unsigned int account; // account number
    ClientData client = {0, "", "", 0.0};

    printf("Enter account number to view ( 1 - %d ): ", MAX_ACCOUNTS);
    while (scanf("%u", &account) != 1 || account < 1 || account > MAX_ACCOUNTS) {
        printf("Invalid input. Enter a valid account number ( 1 - %d ): ", MAX_ACCOUNTS);
        clearInputBuffer();
    }

    // move file pointer to correct record
    if (fseek(fPtr, (long)(account - 1) * sizeof(ClientData), SEEK_SET) != 0) {
        fprintf(stderr, "Error: Could not seek to account #%u.\n", account);
        return;
    }
    // read record from file
    if (fread(&client, sizeof(ClientData), 1, fPtr) != 1) {
        fprintf(stderr, "Error: Could not read account #%u.\n", account);
        return;
    }

    if (client.acctNum == 0) {
        printf("Account #%u has no information.\n", account);
    } else {
        printf("\n%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");
        printf("%-6u%-16s%-11s%10.2f\n", client.acctNum, client.lastName,
               client.firstName, client.balance);
    }
} // end function viewRecord

// Search and display all records matching a last name (case-insensitive)
// @param fPtr - pointer to the binary data file
void searchByName(FILE *fPtr)
{
    char searchName[LAST_NAME_LEN]; // name to search for
    ClientData client = {0, "", "", 0.0};
    int found = 0; // flag to track if any matches found

    printf("Enter last name to search: ");
    scanf("%14s", searchName);

    rewind(fPtr); // start from beginning of file

    printf("\n%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");
    printf("----------------------------------------------\n");

    // scan through all records
    while (fread(&client, sizeof(ClientData), 1, fPtr) == 1) {
        if (client.acctNum != 0) {
            // case-insensitive comparison
            int match = 1;
            size_t len = strlen(searchName);
            if (len != strlen(client.lastName)) {
                match = 0;
            } else {
                for (size_t i = 0; i < len; ++i) {
                    char a = client.lastName[i];
                    char b = searchName[i];
                    // convert to lowercase for comparison
                    if (a >= 'A' && a <= 'Z') a += 32;
                    if (b >= 'A' && b <= 'Z') b += 32;
                    if (a != b) { match = 0; break; }
                }
            }
            if (match) {
                printf("%-6u%-16s%-11s%10.2f\n", client.acctNum, client.lastName,
                       client.firstName, client.balance);
                found = 1;
            }
        }
    }

    if (!found) {
        printf("No accounts found with last name \"%s\".\n", searchName);
    }
} // end function searchByName

// Display menu and get user choice
// @return the menu option selected by the user
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
                 "5 - view an account\n"
                 "6 - search by last name\n"
                 "7 - end program\n? ");

    while (scanf("%u", &menuChoice) != 1) {
        printf("Invalid input. Please enter a valid number.\n? ");
        clearInputBuffer();
    } // receive choice from user
    return menuChoice;
} // end function enterChoice

// Clear stdin input buffer to prevent leftover characters
// from affecting subsequent scanf calls
void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}