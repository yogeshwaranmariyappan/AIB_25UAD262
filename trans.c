// Bank-account program reads a random-access file sequentially,
// updates data already written to the file, creates new data to
// be placed in the file, and deletes data previously in the file.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_ACCOUNTS 100  // maximum number of accounts
#define LAST_NAME_LEN 15  // max length of last name
#define FIRST_NAME_LEN 10 // max length of first name
#define LOG_FILE "transactions.log" // transaction log filename
#define BACKUP_FILE "credit.dat.bak" // backup filename
#define MIN_BALANCE 0.0   // minimum allowed balance
#define INTEREST_RATE 0.05 // annual interest rate (5%)
#define PAGE_SIZE 10       // records per page for pagination

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
void textFile(FILE *readPtr);    // export all records to text file (sorted)
void updateRecord(FILE *fPtr);   // update balance of existing record
void newRecord(FILE *fPtr);      // create a new account record
void deleteRecord(FILE *fPtr);   // delete an existing account record
void viewRecord(FILE *fPtr);     // view a single account record
void searchByName(FILE *fPtr);   // search records by last name
void transferFunds(FILE *fPtr);  // transfer between two accounts
void listAllAccounts(FILE *fPtr); // list all active accounts with pagination
void showStatistics(FILE *fPtr); // display summary statistics
void applyInterest(FILE *fPtr);  // apply interest to all accounts
void viewTransactionLog(void);   // view recent transaction log entries
void backupData(FILE *fPtr);     // backup credit.dat
void restoreData(FILE **fPtrRef); // restore from backup
void clearInputBuffer(void);     // clear stdin input buffer
void clearScreen(void);          // clear console screen
void logTransaction(unsigned int acctNum, const char *type,
                    double amount, double oldBal, double newBal); // log to file
int compareByName(const void *a, const void *b); // comparator for qsort
int compareByBalance(const void *a, const void *b); // sort by balance

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
    while ((choice = enterChoice()) != 14)
    {
        clearScreen();
        switch (choice)
        {
        case 1:
            textFile(cfPtr);
            break;
        case 2:
            updateRecord(cfPtr);
            break;
        case 3:
            newRecord(cfPtr);
            break;
        case 4:
            deleteRecord(cfPtr);
            break;
        case 5:
            viewRecord(cfPtr);
            break;
        case 6:
            searchByName(cfPtr);
            break;
        case 7:
            transferFunds(cfPtr);
            break;
        case 8:
            listAllAccounts(cfPtr);
            break;
        case 9:
            showStatistics(cfPtr);
            break;
        case 10:
            applyInterest(cfPtr);
            break;
        case 11:
            viewTransactionLog();
            break;
        case 12:
            backupData(cfPtr);
            break;
        case 13:
            restoreData(&cfPtr);
            break;
        default:
            puts("Incorrect choice");
            break;
        } // end switch
    }     // end while

    fclose(cfPtr); // fclose closes the file
    printf("Program ended. Goodbye!\n");
    return 0;
} // end main

// Export all records to a sorted text file for printing
// Records are sorted alphabetically by last name
// @param readPtr - pointer to the binary data file
void textFile(FILE *readPtr)
{
    FILE *writePtr; // accounts.txt file pointer
    ClientData records[MAX_ACCOUNTS]; // array to hold all active records
    ClientData client = {0, "", "", 0.0};
    int count = 0; // number of active records

    // fopen opens the file; exits if file cannot be opened
    if ((writePtr = fopen("accounts.txt", "w")) == NULL)
    {
        fprintf(stderr, "File could not be opened.\n");
    } // end if
    else
    {
        rewind(readPtr); // sets pointer to beginning of file

        // read all active records into array
        while (fread(&client, sizeof(ClientData), 1, readPtr) == 1)
        {
            if (client.acctNum != 0 && count < MAX_ACCOUNTS)
            {
                records[count++] = client;
            }
        }

        // sort records by last name
        qsort(records, count, sizeof(ClientData), compareByName);

        // write sorted records to text file
        fprintf(writePtr, "%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");
        fprintf(writePtr, "----------------------------------------------\n");
        for (int i = 0; i < count; ++i) {
            fprintf(writePtr, "%-6u%-16s%-11s%10.2f\n", records[i].acctNum,
                    records[i].lastName, records[i].firstName, records[i].balance);
        }

        printf("Exported %d accounts to \"accounts.txt\" (sorted by last name).\n", count);
        fclose(writePtr); // fclose closes the file
    }                     // end else
} // end function textFile

// Update balance in an existing record
// @param fPtr - pointer to the binary data file
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

        double oldBalance = client.balance;
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

        // log the transaction
        const char *type = (transaction >= 0) ? "CHARGE" : "PAYMENT";
        logTransaction(client.acctNum, type, transaction, oldBalance, client.balance);
    } // end else
} // end function updateRecord

// delete an existing record
void deleteRecord(FILE *fPtr)
{
    ClientData client;                          // stores record read from file
    ClientData blankClient = {0, "", "", 0};     // blank client
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
        printf("Account #%u created successfully.\n", accountNum);
        logTransaction(accountNum, "NEW_ACCOUNT", client.balance, 0, client.balance);
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
    printf("\n========== BANK ACCOUNT MANAGEMENT ==========\n"
           " 1 - Export accounts to text file\n"
           " 2 - Update an account\n"
           " 3 - Add a new account\n"
           " 4 - Delete an account\n"
           " 5 - View an account\n"
           " 6 - Search by last name\n"
           " 7 - Transfer funds\n"
           " 8 - List all accounts\n"
           " 9 - Account statistics\n"
           "10 - Apply interest\n"
           "11 - View transaction log\n"
           "12 - Backup data\n"
           "13 - Restore data\n"
           "14 - Exit program\n"
           "=============================================\n? ");

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

// Compare two ClientData records by last name (for qsort)
// @param a - pointer to first ClientData
// @param b - pointer to second ClientData
// @return negative if a < b, 0 if equal, positive if a > b
int compareByName(const void *a, const void *b)
{
    const ClientData *clientA = (const ClientData *)a;
    const ClientData *clientB = (const ClientData *)b;
    int result = strcmp(clientA->lastName, clientB->lastName);
    if (result == 0) {
        // if last names match, sort by first name
        result = strcmp(clientA->firstName, clientB->firstName);
    }
    return result;
} // end function compareByName

// Log a transaction to the transactions.log file with timestamp
// @param acctNum  - account number involved
// @param type     - transaction type ("CHARGE", "PAYMENT", "NEW_ACCOUNT", etc.)
// @param amount   - transaction amount
// @param oldBal   - balance before transaction
// @param newBal   - balance after transaction
void logTransaction(unsigned int acctNum, const char *type,
                    double amount, double oldBal, double newBal)
{
    FILE *logFile = fopen(LOG_FILE, "a");
    if (logFile == NULL) {
        fprintf(stderr, "Warning: Could not open %s for logging.\n", LOG_FILE);
        return;
    }

    // get current timestamp
    time_t now = time(NULL);
    char timeStr[26];
    struct tm *tm_info = localtime(&now);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(logFile, "[%s] Acct #%-4u | %-12s | Amount: %+10.2f | Old Bal: %10.2f | New Bal: %10.2f\n",
            timeStr, acctNum, type, amount, oldBal, newBal);

    fclose(logFile);
} // end function logTransaction

// Transfer funds between two accounts
// @param fPtr - pointer to the binary data file
void transferFunds(FILE *fPtr)
{
    unsigned int fromAcct, toAcct;
    double amount;
    ClientData fromClient = {0, "", "", 0.0};
    ClientData toClient = {0, "", "", 0.0};

    printf("Enter source account ( 1 - %d ): ", MAX_ACCOUNTS);
    while (scanf("%u", &fromAcct) != 1 || fromAcct < 1 || fromAcct > MAX_ACCOUNTS) {
        printf("Invalid. Enter source account ( 1 - %d ): ", MAX_ACCOUNTS);
        clearInputBuffer();
    }
    printf("Enter destination account ( 1 - %d ): ", MAX_ACCOUNTS);
    while (scanf("%u", &toAcct) != 1 || toAcct < 1 || toAcct > MAX_ACCOUNTS) {
        printf("Invalid. Enter destination account ( 1 - %d ): ", MAX_ACCOUNTS);
        clearInputBuffer();
    }
    if (fromAcct == toAcct) {
        printf("Cannot transfer to the same account.\n");
        return;
    }

    // read source account
    fseek(fPtr, (long)(fromAcct - 1) * sizeof(ClientData), SEEK_SET);
    fread(&fromClient, sizeof(ClientData), 1, fPtr);
    if (fromClient.acctNum == 0) { printf("Source account #%u does not exist.\n", fromAcct); return; }

    // read destination account
    fseek(fPtr, (long)(toAcct - 1) * sizeof(ClientData), SEEK_SET);
    fread(&toClient, sizeof(ClientData), 1, fPtr);
    if (toClient.acctNum == 0) { printf("Destination account #%u does not exist.\n", toAcct); return; }

    printf("Transfer amount: ");
    while (scanf("%lf", &amount) != 1 || amount <= 0) {
        printf("Invalid. Enter a positive amount: ");
        clearInputBuffer();
    }

    if (fromClient.balance - amount < MIN_BALANCE) {
        printf("Insufficient funds. Source balance: %.2f\n", fromClient.balance);
        return;
    }

    double oldFrom = fromClient.balance, oldTo = toClient.balance;
    fromClient.balance -= amount;
    toClient.balance += amount;

    // write updated source
    fseek(fPtr, (long)(fromAcct - 1) * sizeof(ClientData), SEEK_SET);
    fwrite(&fromClient, sizeof(ClientData), 1, fPtr);
    // write updated destination
    fseek(fPtr, (long)(toAcct - 1) * sizeof(ClientData), SEEK_SET);
    fwrite(&toClient, sizeof(ClientData), 1, fPtr);

    printf("Transferred %.2f from #%u to #%u.\n", amount, fromAcct, toAcct);
    printf("  #%u balance: %.2f -> %.2f\n", fromAcct, oldFrom, fromClient.balance);
    printf("  #%u balance: %.2f -> %.2f\n", toAcct, oldTo, toClient.balance);

    logTransaction(fromAcct, "TRANSFER_OUT", -amount, oldFrom, fromClient.balance);
    logTransaction(toAcct, "TRANSFER_IN", amount, oldTo, toClient.balance);
} // end function transferFunds

// List all active accounts with pagination
// @param fPtr - pointer to the binary data file
void listAllAccounts(FILE *fPtr)
{
    ClientData client = {0, "", "", 0.0};
    int count = 0, page = 0;

    rewind(fPtr);
    printf("\n%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");
    printf("----------------------------------------------\n");

    while (fread(&client, sizeof(ClientData), 1, fPtr) == 1) {
        if (client.acctNum != 0) {
            printf("%-6u%-16s%-11s%10.2f\n", client.acctNum, client.lastName,
                   client.firstName, client.balance);
            count++;
            if (count % PAGE_SIZE == 0) {
                page++;
                printf("--- Page %d (%d accounts shown) - Press Enter for more ---", page, count);
                clearInputBuffer();
                getchar();
            }
        }
    }
    printf("----------------------------------------------\n");
    printf("Total: %d active account(s).\n", count);
} // end function listAllAccounts

// Display summary statistics for all accounts
// @param fPtr - pointer to the binary data file
void showStatistics(FILE *fPtr)
{
    ClientData client = {0, "", "", 0.0};
    int count = 0;
    double total = 0, highest = 0, lowest = 0;
    unsigned int highAcct = 0, lowAcct = 0;

    rewind(fPtr);
    while (fread(&client, sizeof(ClientData), 1, fPtr) == 1) {
        if (client.acctNum != 0) {
            if (count == 0) {
                highest = lowest = client.balance;
                highAcct = lowAcct = client.acctNum;
            }
            total += client.balance;
            if (client.balance > highest) { highest = client.balance; highAcct = client.acctNum; }
            if (client.balance < lowest) { lowest = client.balance; lowAcct = client.acctNum; }
            count++;
        }
    }

    if (count == 0) {
        printf("No active accounts found.\n");
        return;
    }

    printf("\n============ ACCOUNT STATISTICS ============\n");
    printf("  Total accounts   : %d\n", count);
    printf("  Total balance    : %.2f\n", total);
    printf("  Average balance  : %.2f\n", total / count);
    printf("  Highest balance  : %.2f (Account #%u)\n", highest, highAcct);
    printf("  Lowest balance   : %.2f (Account #%u)\n", lowest, lowAcct);
    printf("============================================\n");
} // end function showStatistics

// Apply annual interest rate to all active accounts
// @param fPtr - pointer to the binary data file
void applyInterest(FILE *fPtr)
{
    ClientData client = {0, "", "", 0.0};
    int count = 0;

    printf("Apply %.1f%% interest to all accounts? (y/n): ", INTEREST_RATE * 100);
    clearInputBuffer();
    int ch = getchar();
    if (ch != 'y' && ch != 'Y') { printf("Interest application cancelled.\n"); return; }

    rewind(fPtr);
    while (fread(&client, sizeof(ClientData), 1, fPtr) == 1) {
        if (client.acctNum != 0 && client.balance > 0) {
            double oldBal = client.balance;
            double interest = client.balance * INTEREST_RATE;
            client.balance += interest;

            // seek back and overwrite
            fseek(fPtr, -(long)sizeof(ClientData), SEEK_CUR);
            fwrite(&client, sizeof(ClientData), 1, fPtr);
            fflush(fPtr); // flush before next read

            logTransaction(client.acctNum, "INTEREST", interest, oldBal, client.balance);
            count++;
        }
    }
    printf("Interest applied to %d account(s).\n", count);
} // end function applyInterest

// View recent entries from the transaction log file
void viewTransactionLog(void)
{
    FILE *logFile = fopen(LOG_FILE, "r");
    if (logFile == NULL) {
        printf("No transaction log found.\n");
        return;
    }

    // read all lines and display last 20
    char lines[100][150]; // store up to 100 recent lines
    int lineCount = 0;

    while (fgets(lines[lineCount % 100], 150, logFile) != NULL) {
        lineCount++;
    }
    fclose(logFile);

    int total = lineCount;
    int start = (lineCount > 20) ? lineCount - 20 : 0;
    int displayStart = (total > 100) ? (total - 100) : 0;

    printf("\n========== TRANSACTION LOG (last 20 of %d entries) ==========\n", total);
    for (int i = start; i < lineCount && i < start + 20; i++) {
        printf("%s", lines[(displayStart + i) % 100]);
    }
    printf("=============================================================\n");
} // end function viewTransactionLog

// Backup credit.dat to credit.dat.bak
// @param fPtr - pointer to the binary data file
void backupData(FILE *fPtr)
{
    FILE *backup = fopen(BACKUP_FILE, "wb");
    if (backup == NULL) {
        fprintf(stderr, "Error: Could not create backup file.\n");
        return;
    }

    ClientData client;
    rewind(fPtr);
    while (fread(&client, sizeof(ClientData), 1, fPtr) == 1) {
        fwrite(&client, sizeof(ClientData), 1, backup);
    }

    fclose(backup);
    printf("Data backed up to \"%s\" successfully.\n", BACKUP_FILE);
} // end function backupData

// Restore credit.dat from credit.dat.bak
// @param fPtrRef - pointer to the file pointer (so we can reopen)
void restoreData(FILE **fPtrRef)
{
    FILE *backup = fopen(BACKUP_FILE, "rb");
    if (backup == NULL) {
        fprintf(stderr, "Error: Backup file \"%s\" not found.\n", BACKUP_FILE);
        return;
    }

    printf("Restore from backup? This will overwrite current data. (y/n): ");
    clearInputBuffer();
    int ch = getchar();
    if (ch != 'y' && ch != 'Y') { fclose(backup); printf("Restore cancelled.\n"); return; }

    // close current file, reopen for writing
    fclose(*fPtrRef);
    *fPtrRef = fopen("credit.dat", "wb+");
    if (*fPtrRef == NULL) {
        fprintf(stderr, "Error: Could not open credit.dat for restore.\n");
        fclose(backup);
        exit(-1);
    }

    ClientData client;
    while (fread(&client, sizeof(ClientData), 1, backup) == 1) {
        fwrite(&client, sizeof(ClientData), 1, *fPtrRef);
    }

    fclose(backup);
    rewind(*fPtrRef);
    printf("Data restored from \"%s\" successfully.\n", BACKUP_FILE);
} // end function restoreData

// Clear the console screen (cross-platform)
void clearScreen(void)
{
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
} // end function clearScreen

// Compare two ClientData records by balance (for qsort, descending)
int compareByBalance(const void *a, const void *b)
{
    const ClientData *clientA = (const ClientData *)a;
    const ClientData *clientB = (const ClientData *)b;
    if (clientB->balance > clientA->balance) return 1;
    if (clientB->balance < clientA->balance) return -1;
    return 0;
} // end function compareByBalance