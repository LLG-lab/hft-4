#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "export_csv.h"

#define BUFFER_SIZE 1048576

static char *buffer = NULL;

static int cursor = 0;

static char lasterrorstr[150] = {'\0'};

static FILE *csv_file = NULL;


int exportcsv_create(const char *file_name)
{
    csv_file = fopen(file_name, "w");

    if (csv_file == NULL)
    {
        sprintf(lasterrorstr, "%s", strerror(errno));

        return 1;
    }

    buffer = malloc(BUFFER_SIZE);
    cursor = 0;

    return 0;
}

int exportcsv_writeln(const char *line)
{
    size_t bytes_left = BUFFER_SIZE - cursor;
    size_t strsz = strlen(line);

    if (strsz >= bytes_left)
    {
        size_t n = fwrite(buffer , sizeof(char), cursor, csv_file);

        if (n != cursor)
        {
            sprintf(lasterrorstr, "Write error");

            return 1;
        }

        cursor = 0;
    }

    strcpy(buffer + cursor, line);
    cursor += strsz;

    return 0;
}

void exportcsv_close(void)
{
    if (buffer == NULL)
    {
        return;
    }

    if (cursor > 0)
    {
        fwrite(buffer , sizeof(char), cursor, csv_file);
        cursor = 0;
    }

    fclose(csv_file);

    free(buffer);

    buffer = NULL;
}

const char *exportcsv_lasterror(void)
{
    return lasterrorstr;
}