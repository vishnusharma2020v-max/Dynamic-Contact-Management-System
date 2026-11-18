#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_SIZE 100
#define ADDRESS_SIZE 200

typedef struct {
    char name[TEXT_SIZE];
    char **phones;
    int phoneCount;
    char **emails;
    int emailCount;
    char address[ADDRESS_SIZE];
    int mergedInto;  /* Index of the surviving contact, or -1 */
} Contact;

typedef struct {
    Contact *items;
    int count;
    int capacity;
} ContactList;

void readLine(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    }
}

void initializeList(ContactList *list) {
    list->count = 0;
    list->capacity = 4;
    list->items = malloc((size_t)list->capacity * sizeof(Contact));

    if (list->items == NULL) {
        perror("Unable to allocate contact list");
        exit(EXIT_FAILURE);
    }
}

void ensureListCapacity(ContactList *list) {
    if (list->count < list->capacity) {
        return;
    }

    int newCapacity = list->capacity * 2;
    Contact *newItems = realloc(
        list->items,
        (size_t)newCapacity * sizeof(Contact)
    );

    if (newItems == NULL) {
        perror("Unable to grow contact list");
        exit(EXIT_FAILURE);
    }

    list->items = newItems;
    list->capacity = newCapacity;
}

void addString(char ***strings, int *count, const char *value) {
    char **newStrings = realloc(
        *strings,
        (size_t)(*count + 1) * sizeof(char *)
    );

    if (newStrings == NULL) {
        perror("Unable to grow string list");
        exit(EXIT_FAILURE);
    }

    *strings = newStrings;
    (*strings)[*count] = malloc(strlen(value) + 1);

    if ((*strings)[*count] == NULL) {
        perror("Unable to allocate string");
        exit(EXIT_FAILURE);
    }

    strcpy((*strings)[*count], value);
    (*count)++;
}

void freeStrings(char **strings, int count) {
    for (int i = 0; i < count; i++) {
        free(strings[i]);
    }
    free(strings);
}

void freeContact(Contact *contact) {
    freeStrings(contact->phones, contact->phoneCount);
    freeStrings(contact->emails, contact->emailCount);
    contact->phones = NULL;
    contact->emails = NULL;
    contact->phoneCount = 0;
    contact->emailCount = 0;
}

int isActive(const Contact *contact) {
    return contact->mergedInto == -1;
}

void addContact(ContactList *list) {
    Contact contact = {0};
    contact.mergedInto = -1;

    readLine("Name: ", contact.name, sizeof(contact.name));
    readLine("Address: ", contact.address, sizeof(contact.address));

    char value[TEXT_SIZE];

    printf("Enter phone numbers one at a time. Enter a blank line when done.\n");
    while (1) {
        readLine("Phone: ", value, sizeof(value));
        if (value[0] == '\0') {
            break;
        }
        addString(&contact.phones, &contact.phoneCount, value);
    }

    printf("Enter email addresses one at a time. Enter a blank line when done.\n");
    while (1) {
        readLine("Email: ", value, sizeof(value));
        if (value[0] == '\0') {
            break;
        }
        addString(&contact.emails, &contact.emailCount, value);
    }

    ensureListCapacity(list);
    list->items[list->count++] = contact;

    printf("Contact added with ID %d.\n", list->count);
}

void printContact(const Contact *contact, int index) {
    printf("\nID: %d\nName: %s\nAddress: %s\n",
           index + 1, contact->name, contact->address);

    printf("Phones:");
    for (int i = 0; i < contact->phoneCount; i++) {
        printf(" %s%s", i ? "," : "", contact->phones[i]);
    }

    printf("\nEmails:");
    for (int i = 0; i < contact->emailCount; i++) {
        printf(" %s%s", i ? "," : "", contact->emails[i]);
    }
    printf("\n");
}

void listContacts(const ContactList *list) {
    int found = 0;
    for (int i = 0; i < list->count; i++) {
        if (isActive(&list->items[i])) {
            printContact(&list->items[i], i);
            found = 1;
        }
    }
    if (!found) {
        printf("No contacts found.\n");
    }
}

void editContact(ContactList *list) {
    int id;
    printf("Contact ID to edit: ");
    if (scanf("%d", &id) != 1) {
        while (getchar() != '\n') {}
        printf("Invalid ID.\n");
        return;
    }
    getchar();

    if (id < 1 || id > list->count || !isActive(&list->items[id - 1])) {
        printf("Contact not found.\n");
        return;
    }

    Contact *contact = &list->items[id - 1];
    readLine("New name: ", contact->name, sizeof(contact->name));
    readLine("New address: ", contact->address, sizeof(contact->address));
    printf("Name and address updated. Existing phones and emails were kept.\n");
}

void deleteContact(ContactList *list) {
    int id;
    printf("Contact ID to delete: ");
    if (scanf("%d", &id) != 1) {
        while (getchar() != '\n') {}
        printf("Invalid ID.\n");
        return;
    }
    getchar();

    if (id < 1 || id > list->count || !isActive(&list->items[id - 1])) {
        printf("Contact not found.\n");
        return;
    }

    freeContact(&list->items[id - 1]);
    list->items[id - 1].mergedInto = -2; /* Deleted */
    printf("Contact deleted.\n");
}

void normalize(const char *source, char *result, int keepDigitsOnly) {
    int j = 0;

    for (int i = 0; source[i] != '\0' && j < TEXT_SIZE - 1; i++) {
        unsigned char ch = (unsigned char)source[i];

        if (keepDigitsOnly) {
            if (isdigit(ch)) {
                result[j++] = (char)ch;
            }
        } else if (!isspace(ch)) {
            result[j++] = (char)tolower(ch);
        }
    }
    result[j] = '\0';
}

int sameValue(const char *a, const char *b, int phone) {
    char normalizedA[TEXT_SIZE];
    char normalizedB[TEXT_SIZE];

    normalize(a, normalizedA, phone);
    normalize(b, normalizedB, phone);

    return normalizedA[0] != '\0' &&
           strcmp(normalizedA, normalizedB) == 0;
}

int contactsAreDuplicates(const Contact *a, const Contact *b) {
    if (sameValue(a->name, b->name, 0)) {
        return 1;
    }

    for (int i = 0; i < a->phoneCount; i++) {
        for (int j = 0; j < b->phoneCount; j++) {
            if (sameValue(a->phones[i], b->phones[j], 1)) {
                return 1;
            }
        }
    }

    for (int i = 0; i < a->emailCount; i++) {
        for (int j = 0; j < b->emailCount; j++) {
            if (sameValue(a->emails[i], b->emails[j], 0)) {
                return 1;
            }
        }
    }

    return 0;
}

void findDuplicates(const ContactList *list) {
    int found = 0;

    for (int i = 0; i < list->count; i++) {
        if (!isActive(&list->items[i])) {
            continue;
        }

        for (int j = i + 1; j < list->count; j++) {
            if (isActive(&list->items[j]) &&
                contactsAreDuplicates(&list->items[i], &list->items[j])) {
                printf("Possible duplicate: ID %d and ID %d\n", i + 1, j + 1);
                found = 1;
            }
        }
    }

    if (!found) {
        printf("No possible duplicates found.\n");
    }
}

int containsString(char **strings, int count, const char *value, int phone) {
    for (int i = 0; i < count; i++) {
        if (sameValue(strings[i], value, phone)) {
            return 1;
        }
    }
    return 0;
}

void mergeContacts(ContactList *list) {
    int firstId, secondId;

    printf("ID of contact to keep: ");
    if (scanf("%d", &firstId) != 1) {
        while (getchar() != '\n') {}
        printf("Invalid ID.\n");
        return;
    }

    printf("ID of contact to merge into it: ");
    if (scanf("%d", &secondId) != 1) {
        while (getchar() != '\n') {}
        printf("Invalid ID.\n");
        return;
    }
    getchar();

    if (firstId < 1 || firstId > list->count ||
        secondId < 1 || secondId > list->count ||
        firstId == secondId ||
        !isActive(&list->items[firstId - 1]) ||
        !isActive(&list->items[secondId - 1])) {
        printf("Invalid contact IDs.\n");
        return;
    }

    Contact *keep = &list->items[firstId - 1];
    Contact *remove = &list->items[secondId - 1];

    for (int i = 0; i < remove->phoneCount; i++) {
        if (!containsString(keep->phones, keep->phoneCount,
                            remove->phones[i], 1)) {
            addString(&keep->phones, &keep->phoneCount, remove->phones[i]);
        }
    }

    for (int i = 0; i < remove->emailCount; i++) {
        if (!containsString(keep->emails, keep->emailCount,
                            remove->emails[i], 0)) {
            addString(&keep->emails, &keep->emailCount, remove->emails[i]);
        }
    }

    if (keep->address[0] == '\0') {
        strcpy(keep->address, remove->address);
    }

    freeContact(remove);
    remove->mergedInto = firstId - 1;

    printf("Contact %d was merged into contact %d.\n", secondId, firstId);
}

int containsIgnoreCase(const char *text, const char *query) {
    if (query[0] == '\0') {
        return 1;
    }

    size_t queryLength = strlen(query);

    for (size_t i = 0; text[i] != '\0'; i++) {
        size_t j = 0;
        while (j < queryLength && text[i + j] != '\0' &&
               tolower((unsigned char)text[i + j]) ==
               tolower((unsigned char)query[j])) {
            j++;
        }
        if (j == queryLength) {
            return 1;
        }
    }
    return 0;
}

int contactMatches(const Contact *contact, const char *query) {
    if (containsIgnoreCase(contact->name, query) ||
        containsIgnoreCase(contact->address, query)) {
        return 1;
    }

    for (int i = 0; i < contact->phoneCount; i++) {
        if (containsIgnoreCase(contact->phones[i], query)) {
            return 1;
        }
    }

    for (int i = 0; i < contact->emailCount; i++) {
        if (containsIgnoreCase(contact->emails[i], query)) {
            return 1;
        }
    }

    return 0;
}

void searchContacts(const ContactList *list) {
    char query[TEXT_SIZE];
    readLine("Search text (prefix or substring): ", query, sizeof(query));

    int found = 0;
    for (int i = 0; i < list->count; i++) {
        if (isActive(&list->items[i]) &&
            contactMatches(&list->items[i], query)) {
            printContact(&list->items[i], i);
            found = 1;
        }
    }

    if (!found) {
        printf("No matching contacts found.\n");
    }
}

void exportContacts(const ContactList *list) {
    char filename[TEXT_SIZE];
    readLine("Export filename: ", filename, sizeof(filename));

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Could not open export file");
        return;
    }

    for (int i = 0; i < list->count; i++) {
        const Contact *contact = &list->items[i];
        if (!isActive(contact)) {
            continue;
        }

        fprintf(file, "Name: %s\nAddress: %s\nPhones:",
                contact->name, contact->address);

        for (int j = 0; j < contact->phoneCount; j++) {
            fprintf(file, "%s%s", j ? ", " : " ", contact->phones[j]);
        }

        fprintf(file, "\nEmails:");
        for (int j = 0; j < contact->emailCount; j++) {
            fprintf(file, "%s%s", j ? ", " : " ", contact->emails[j]);
        }
        fprintf(file, "\n\n");
    }

    fclose(file);
    printf("Contacts exported to %s.\n", filename);
}

void importContacts(ContactList *list) {
    char filename[TEXT_SIZE];
    readLine("Import filename: ", filename, sizeof(filename));

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Could not open import file");
        return;
    }

    /*
     * Import format: one contact per line:
     * name|phone1,phone2|email1,email2|address
     */
    char line[600];
    int imported = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        char *name = strtok(line, "|");
        char *phones = strtok(NULL, "|");
        char *emails = strtok(NULL, "|");
        char *address = strtok(NULL, "|");

        if (name == NULL) {
            continue;
        }

        Contact contact = {0};
        contact.mergedInto = -1;
        strncpy(contact.name, name, sizeof(contact.name) - 1);

        if (address != NULL) {
            strncpy(contact.address, address, sizeof(contact.address) - 1);
        }

        char *token;
        if (phones != NULL) {
            token = strtok(phones, ",");
            while (token != NULL) {
                addString(&contact.phones, &contact.phoneCount, token);
                token = strtok(NULL, ",");
            }
        }

        if (emails != NULL) {
            token = strtok(emails, ",");
            while (token != NULL) {
                addString(&contact.emails, &contact.emailCount, token);
                token = strtok(NULL, ",");
            }
        }

        ensureListCapacity(list);
        list->items[list->count++] = contact;
        imported++;
    }

    fclose(file);
    printf("Imported %d contact(s).\n", imported);
}

void cleanup(ContactList *list) {
    for (int i = 0; i < list->count; i++) {
        freeContact(&list->items[i]);
    }
    free(list->items);
}

int main(void) {
    ContactList contacts;
    initializeList(&contacts);

    int choice;
    do {
        printf("\n=== Dynamic Contact Manager ===\n");
        printf("1. Add contact\n");
        printf("2. Edit contact name/address\n");
        printf("3. Delete contact\n");
        printf("4. List contacts\n");
        printf("5. Detect possible duplicates\n");
        printf("6. Merge contacts\n");
        printf("7. Search contacts\n");
        printf("8. Import contacts\n");
        printf("9. Export contacts\n");
        printf("0. Exit\n");
        printf("Choice: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n') {}
            printf("Please enter a number.\n");
            continue;
        }
        getchar();

        switch (choice) {
            case 1: addContact(&contacts); break;
            case 2: editContact(&contacts); break;
            case 3: deleteContact(&contacts); break;
            case 4: listContacts(&contacts); break;
            case 5: findDuplicates(&contacts); break;
            case 6: mergeContacts(&contacts); break;
            case 7: searchContacts(&contacts); break;
            case 8: importContacts(&contacts); break;
            case 9: exportContacts(&contacts); break;
            case 0: printf("Goodbye.\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 0);

    cleanup(&contacts);
    return 0;
}
