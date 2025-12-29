#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>

#include "common.h"
#include  "parse.h"

int create_db_header(struct dbheader_t **headerOut) {
    struct dbheader_t *header = (struct dbheader_t *) calloc(1, sizeof(struct dbheader_t));

    if(header == NULL) {
        printf("Malloc failed to create db header\n");
        return STATUS_ERROR;
    }

    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;

    return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
    if(fd < 0) {
        printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = (struct dbheader_t *) calloc(1, sizeof(struct dbheader_t));
    if(header == NULL) {
        printf("Malloc failed create a db header\n");
        return STATUS_ERROR;
    }

    if(read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("read");
        free(header);
        return STATUS_ERROR;
    }

    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);

    if(header->version != 1) {
        printf("Improper header version\n");
        free(header);
        return STATUS_ERROR;
    }

    if(header->magic != HEADER_MAGIC) {
        printf("Improper header magic\n");
        free(header);
        return STATUS_ERROR;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);

    if(header->filesize != dbstat.st_size) {
        printf("Improper size of file\n");
        free(header);
        return STATUS_ERROR;
    }

    *headerOut = header;

    return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbheader, struct employee_t **employeesOut) {
    if(fd < 0) {
        printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

    int count = dbheader->count;

    // allocate memory for employee
    struct employee_t *employees = (struct employee_t *) calloc(count, sizeof(struct employee_t));
    if(employees == NULL) {
        printf("Malloc failed\n");
        return STATUS_ERROR;
    }

    read(fd, employees, count*sizeof(struct employee_t));

    for(int i; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;
    return STATUS_SUCCESS;
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {
    printf("%s\n", addstring);

    char *name = strtok(addstring, ",");
    char *addr = strtok(NULL, ",");
    char *hours = strtok(NULL, ",");

    if(name == NULL || addr == NULL || hours == NULL) {
        printf("Failed to parse text");
        return STATUS_ERROR;
    }

    printf("%s %s %s\n", name, addr, hours);

    strncpy(employees[dbhdr->count-1].name, name, sizeof(employees[dbhdr->count-1].name));
    strncpy(employees[dbhdr->count-1].address, addr, sizeof(employees[dbhdr->count-1].address));

    employees[dbhdr->count-1].hours = atoi(hours);

    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employee) {
    if(fd < 0) {
        printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

    int realcount = dbhdr->count;

    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->filesize = htonl(
        sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount)
    );
    dbhdr->count = htons(dbhdr->count);
    dbhdr->version = htons(dbhdr->version);

    // The file descriptor might have been used before. 
    // If fd was passed in from somewhere else and you don't know its current position, lseek guarantees you're at the start. 
    // Without it, you might accidentally write at byte 500 if someone else had seeked there.
    // You're reusing an open file descriptor. 
    // If this function gets called multiple times with the same fd, or if other code has used it between calls, 
    // lseek ensures you overwrite from the beginning each time.
    // Writing a header to an existing file. You might open a file that already has data, 
    // and you want to update just the header at the beginning without touching the rest. Then you need lseek(fd, 0, SEEK_SET) to position there.
    // In the specific context of your code: if fd is freshly opened just for this function call, then yes, lseek is unnecessary. 
    // But if it's a reused file descriptor or you're being defensive about file position, it's good practice.
    lseek(fd, 0, SEEK_SET);

    write(fd, dbhdr, sizeof(struct dbheader_t));

    for(int i = 0; i < realcount; i++) {
        employee[i].hours = htonl(employee[i].hours); // hours is int which is different upon system.
        write(fd, &employee[i], sizeof(struct employee_t));
    }

    return STATUS_SUCCESS;
}