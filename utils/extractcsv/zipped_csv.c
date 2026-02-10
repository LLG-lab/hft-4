#include "zipped_csv.h"

#include <zip.h>
#include <stdlib.h>
#include <string.h>

static char last_error[1024] = { '\0' };

#define INCREMENT_INDEX() \
    if (++index == n) \
    { \
        n = zip_fread(zf, buffer, BUFFER_SIZE); \
        if (n == 0) \
        { \
            break; \
        } \
        index = 0; \
    } \
    record_buffer[record_index++] = buffer[index];

#define GOTO_NEXT_LINE() \
    while (buffer[index] != '\n') \
    { \
        INCREMENT_INDEX() \
    } \
    record_buffer[record_index] = '\0'; \
    record_index = 0; \
    ++ln;

#define READ_DIGIT(_where_) \
    if (buffer[index] < '0' || buffer[index] > '9') \
    { \
        sprintf(last_error, "Expected digit, got ‘%c’ in line %d\n", buffer[index], ln); \
        status = 1; \
        goto End; \
    } \
    else \
    { \
        _where_ = buffer[index] - '0'; \
    }

#define EXPECT_CHARACTER(_char_) \
    if (buffer[index] != _char_) \
    { \
        sprintf(last_error, "Expected character ‘%c’, got ‘%c’ in line %d\n", _char_, buffer[index], ln); \
        status = 1; \
        goto End; \
    }

#define VALIDATE_RANGE(_what_, _min_, _max_, _s_) \
    if (_what_ < _min_ || _what_ > _max_) \
    { \
        sprintf(last_error, "%s is expected to be within %d ... %d, got %d in line %d\n", _s_, _min_, _max_, _what_, ln); \
        status = 1; \
        goto End; \
    }

static int csv_file_from_zip(char *csv_file_name, const char *zip_name)
{
    // Extract file from path
    int index = strlen(zip_name) - 1;

    if (index < 0)
    {
        return 1;
    }

    while (index > 0)
    {
        if (zip_name[index] == '/')
        {
            ++index;
            break;
        }

        --index;
    }

    strcpy(csv_file_name, zip_name + index);

    index = strlen(csv_file_name) - 3;

    csv_file_name[index++] = 'c';
    csv_file_name[index++] = 's';
    csv_file_name[index]   = 'v';

    return 0;
}

#define BUFFER_SIZE 1048576

int zipped_csv_read_all_records(const char *zip_name, on_record_function f)
{
    int status = 0;

    char csv_file_name[50] = { '\0' };

    char record_buffer[1024];
    int  record_index = 0;

    char *buffer = malloc(BUFFER_SIZE);

    int zip_err;
    zip_t *myzip = zip_open(zip_name, ZIP_RDONLY, &zip_err);

    if (myzip == NULL)
    {
        sprintf(last_error, "ZIP error (%d) – Unable to open file %s\n", zip_err, zip_name);
        status = 1;
        goto End2;
    }

    csv_file_from_zip(csv_file_name, zip_name);

    zip_file_t *zf = zip_fopen(myzip, csv_file_name, ZIP_FL_UNCHANGED);

    if (zf == NULL)
    {
        sprintf(last_error, "Unable to open CSV file ‘%s’ from ZIP archive ‘%s’\n", csv_file_name, zip_name);
        status = 1;
        goto End1;
    }

    zip_int64_t n = zip_fread(zf, buffer, BUFFER_SIZE);

    if (n == 0)
    {
        goto End;
    }

    int index = 0;
    int ln = 1;

    // Skip header.
    GOTO_NEXT_LINE()

    // 20.01.2020 00:00:00.266,1.028060,1.028050,0.00,0.00
    int day, month, year, hour, minute, second, microsecond;
    int a[4];

    day = month = year = hour = minute = second = microsecond = 0;

    timestamp_t ts;

    while (1)
    {
        INCREMENT_INDEX()

        // Day
        READ_DIGIT(a[0])
        INCREMENT_INDEX()
        READ_DIGIT(a[1])

        day = 10*a[0] + a[1];
        VALIDATE_RANGE(day, 1, 31, "Day")

        INCREMENT_INDEX()
        EXPECT_CHARACTER('.')
        INCREMENT_INDEX()

        // Month
        READ_DIGIT(a[0])
        INCREMENT_INDEX()
        READ_DIGIT(a[1])

        month = 10*a[0] + a[1];
        VALIDATE_RANGE(month, 1, 12, "Month")

        INCREMENT_INDEX()
        EXPECT_CHARACTER('.')
        INCREMENT_INDEX()

        // Year.
        READ_DIGIT(a[0])
        INCREMENT_INDEX()
        READ_DIGIT(a[1])
        INCREMENT_INDEX()
        READ_DIGIT(a[2])
        INCREMENT_INDEX()
        READ_DIGIT(a[3])

        year = 1000*a[0] + 100*a[1] + 10*a[2] + a[3];
        VALIDATE_RANGE(year, 1900, 3000, "Year")

        INCREMENT_INDEX()
        EXPECT_CHARACTER(' ')
        INCREMENT_INDEX()

        // Hour.
        READ_DIGIT(a[0])
        INCREMENT_INDEX()
        READ_DIGIT(a[1])

        hour = 10*a[0] + a[1];
        VALIDATE_RANGE(hour, 0, 23, "Hour")

        INCREMENT_INDEX()
        EXPECT_CHARACTER(':')
        INCREMENT_INDEX()

        // Minute.
        READ_DIGIT(a[0])
        INCREMENT_INDEX()
        READ_DIGIT(a[1])

        minute = 10*a[0] + a[1];
        VALIDATE_RANGE(hour, 0, 59, "Minute")

        INCREMENT_INDEX()
        EXPECT_CHARACTER(':')
        INCREMENT_INDEX()

        // Second.
        READ_DIGIT(a[0])
        INCREMENT_INDEX()
        READ_DIGIT(a[1])

        second = 10*a[0] + a[1];
        VALIDATE_RANGE(second, 0, 59, "Second")

        INCREMENT_INDEX()
        EXPECT_CHARACTER('.')
        INCREMENT_INDEX()

        // Microsecond.
        READ_DIGIT(a[0])
        INCREMENT_INDEX()
        READ_DIGIT(a[1])
        INCREMENT_INDEX()
        READ_DIGIT(a[2])

        // Microsecond not need to be validated.
        microsecond = 1000 * (100*a[0] + 10*a[1] + a[2]);

        INCREMENT_INDEX()
        EXPECT_CHARACTER(',')
        INCREMENT_INDEX()

        GOTO_NEXT_LINE()

        ts = mk_timestamp(day, month, year, hour, minute, second, microsecond);

        if (f)
        {
            f(record_buffer, ts);
        }

        if (n == 0)
        {
            break;
        }
    }

End:
    // Close compressed file.
    zip_fclose(zf);

End1:
    // Close archive.
    zip_close(myzip);

End2:
    free(buffer);

    return status;
}

const char *zipped_csv_get_last_error_str(void)
{
    return last_error;
}
