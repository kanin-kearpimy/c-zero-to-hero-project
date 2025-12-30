#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "file.h"
#include "parse.h"
#include "common.h"

void print_usage(char *argv[]) {
    printf("Usage: %s -n -f <database file>\n", argv[0]);
    printf("\t -n - create new database file\n");
    printf("\t -f - (required) path to database file\n");
    printf("\t -l - list employee\n");
    printf("\t -r [string] - remove employee by name");
    printf("\t -u [string] - update employee by name");
    printf("\t -a [string] - add new employee to database\n");
    return;
}

int main(int argc, char *argv[]) {
    char *filepath = NULL;
    char *addstring = NULL;
    bool newfile = false;
    bool list = false;
    bool isRemove = false;
    char *nameToRemove = NULL;
    bool isUpdate = false;
    char *nameToUpdate = NULL;
    char *dataToUpdate = NULL;
    int c;

    int dbfd = -1;
    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;

    while ((c = getopt(argc, argv, "nf:a:lru:")) != -1) {
        switch(c) {
            case 'n':
                newfile = true;
                break;
            case 'f':
                filepath = optarg;
                break;
            case 'a':
                addstring = optarg;
                break;
            case 'l':
                list = true;
                break;
            case 'r':
                isRemove = true;
                nameToRemove = optarg;
                break;
            case 'u':
                isUpdate = true;
                nameToUpdate = optarg;
                dataToUpdate = argv[optind++];
                break;
            case '?':
                printf("Unknown option -%c\n", c);
            default:
                return -1;
        }
    }

    if (filepath == NULL) {
        printf("Filepath is a required argument\n");
        print_usage(argv);

        return 0;
    }

    if (newfile) {
        dbfd = create_db_file(filepath);
        if(dbfd == STATUS_ERROR) {
            printf("Unable to create database file\n");
            return -1;
        }
        if(create_db_header(&dbhdr) == STATUS_ERROR) {
            printf("Failed to create database header\n");
            return -1;
        }
    }else{
        dbfd = open_db_file(filepath);
        if(dbfd == STATUS_ERROR) {
            printf("Unable to open database file\n");
            return -1;
        }

        if(validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
            printf("Failed to validate database header\n");
            return -1;
        }

        // CONTINUE: Adding Employees to Our Database at 20:16
    }

    // printf("Newfile: %d\n", newfile);
    // printf("Filepath: %s\n", filepath);

    if(read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
        printf("Failed to read employee");
        return 0;
    };

    if(addstring) {
        add_employee(dbhdr, &employees, addstring);
    }

    if(list) {
        list_employees(dbhdr, employees);
    }

    if(isRemove) {
        remove_employee(dbhdr, &employees, nameToRemove);
    }

    if(isUpdate) {
        update_employee(dbhdr, &employees, nameToUpdate, dataToUpdate);
    }

    output_file(dbfd, dbhdr, employees);

    return 0;
}