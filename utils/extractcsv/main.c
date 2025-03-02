#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "zipped_csv.h"
#include "export_csv.h"

enum
{
    INSTRUMENT_PARAM,
    BEGIN_PARAM,
    END_PARAM,
    INTERVAL_PARAM,
    OUTPUT_FILE_PARAM,
    VERBOSE_PARAM
};

#define DECLARE_PARAM(_pstr_) \
    { .str = _pstr_, .size = sizeof(_pstr_) },

static struct _param
{
    const char *str;
    size_t      size;
} parameters[] = {
    DECLARE_PARAM("--instrument")
    DECLARE_PARAM("--begin")
    DECLARE_PARAM("--end")
    DECLARE_PARAM("--interval")
    DECLARE_PARAM("--output-file")
    DECLARE_PARAM("--verbose")
};

static struct _config
{
    const char *instrument;
    timestamp_t begin;
    timestamp_t end;
    double interval;
    const char *out_csv;
    bool verbose;

} config = { .instrument = NULL, .begin = 0ul, .end = 0ul, .interval = 0.0, .out_csv = NULL, .verbose = false};


static void show_help(void)
{
    printf("Extract CSV – Extractor historical data into csv from zpiied database\n"
           "Copyright (c) 2017 - 2025 by LLG Ryszard Gradowski, All Rights Reserved.\n\n"
           "Usage:\n"
           "    extractcsv <parameter>=<value> ...\n"
           "Mandatory parameters:\n"
           "    --instrument         ticker of the instrument for which you want to\n"
           "                         extract data to a csv file\n"
           "    --begin              starting date for data, format dd.mm.yyyy\n"
           "    --end                end date for data, format dd.mm.yyyy\n"
           "    --output-file        output csv file to be created\n\n"
           "Optional parameters:\n"
           "    --interval           quotes interval in seconds. Default value 0\n"
           "                         means all market ticks\n"
           "    --verbose            Values ‘yes’ or ‘no’. Default ‘no’.\n\n");
}

static int get_week_number(timestamp_t td)
{
    const timestamp_t t0 = 1420066800000;
    const timestamp_t v  = 605798999;

    if (td + v < t0)
    {
        return -1;
    }

    unsigned long int n = ((td + v - t0) / v);

    return (int)(n);
}

timestamp_t timestamp_from_str(const char *str)
{
    if (strlen(str) != 10)
    {
        return 0ul;
    }

    if (str[2] != '.' || str[5] != '.')
    {
        return 0ul;
    }

    const int j[] = { 0,1,3,4,6,7,8,9 };

    for (int i = 0; i < 8; i++)
    {
        if (str[j[i]] < '0' || str[j[i]] > '9')
        {
            return 0ul;
        }
    }

    int x[4];

    x[0] = str[0] - '0';
    x[1] = str[1] - '0';

    int day = 10*x[0] + x[1];

    x[0] = str[3] - '0';
    x[1] = str[4] - '0';

    int month = 10*x[0] + x[1];

    x[0] = str[6] - '0';
    x[1] = str[7] - '0';
    x[2] = str[8] - '0';
    x[3] = str[9] - '0';

    int year = 1000*x[0] + 100*x[1] + 10*x[2] + x[3];

    return mk_timestamp(day, month, year, 0, 0, 0, 0);
}

static timestamp_t last_record_ts = 0ul;

static void on_record(const char *rec, timestamp_t ts)
{
    if (ts >= config.begin && ts <= config.end)
    {
        timestamp_t diff = (ts - last_record_ts);

        if (diff >= 1000.0 * config.interval)
        {
            if (exportcsv_writeln(rec) != 0)
            {
                printf("Fatal error: Unable to write to file – %s\n", exportcsv_lasterror());

                exit(1);
            }

            last_record_ts = ts;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        show_help();

        return 0;
    }

    for (int i = 1; i < argc; i++)
    {
        if (strstr(argv[i], parameters[INSTRUMENT_PARAM].str))
        {
            config.instrument = argv[i] + parameters[INSTRUMENT_PARAM].size;
        }
        else if (strstr(argv[i], parameters[BEGIN_PARAM].str))
        {
            config.begin = timestamp_from_str(argv[i] + parameters[BEGIN_PARAM].size);

            if (config.begin == 0)
            {
                printf("Bad date format in parameter ‘%s’\n", argv[i]);

                exit(1);
            }

            if (get_week_number(config.begin) < 0)
            {
                printf("Date ‘%s’ is before HFT era\n", argv[i] + parameters[BEGIN_PARAM].size);

                exit(1);
            }
        }
        else if (strstr(argv[i], parameters[END_PARAM].str))
        {
            config.end = timestamp_from_str(argv[i] + parameters[END_PARAM].size);

            if (config.end == 0)
            {
                printf("Bad date format in parameter ‘%s’\n", argv[i]);

                exit(1);
            }

            if (get_week_number(config.end) < 0)
            {
                printf("Date ‘%s’ is before HFT era\n", argv[i] + parameters[END_PARAM].size);

                exit(1);
            }
        }
        else if (strstr(argv[i], parameters[INTERVAL_PARAM].str))
        {
            config.interval = atof(argv[i] + parameters[INTERVAL_PARAM].size);
        }
        else if (strstr(argv[i], parameters[OUTPUT_FILE_PARAM].str))
        {
            config.out_csv = argv[i] + parameters[OUTPUT_FILE_PARAM].size;
        }
        else if (strstr(argv[i], parameters[VERBOSE_PARAM].str))
        {
            const char *vp = argv[i] + parameters[VERBOSE_PARAM].size;

            if (strcmp(vp, "yes") == 0)
            {
                config.verbose = true;
            }
            else if (strcmp(vp, "no") == 0)
            {
                config.verbose = false;
            }
            else
            {
                printf("Invalid verbose parameter value ‘%s’ – should be ‘yes’ or ‘no’\n", argv[i]);

                exit(1);
            }
        }
        else
        {
            printf("Illegal parameter ‘%s’\n", argv[i]);

            exit(1);
        }
    }

    // Validation.

    if (config.begin > config.end)
    {
        printf("The beginning of the data cannot be earlier than the end of the data\n");

        exit(1);
    }

    if (config.instrument == NULL)
    {
        printf("Unspecified instrument\n");

        exit(1);
    }

    if (config.out_csv == NULL)
    {
        printf("Unspecified output CSV file name\n");

        exit(1);
    }

    // Essential processing.

    if (exportcsv_create(config.out_csv) != 0)
    {
        printf("Unable to create file ‘%s’ – %s\n", config.out_csv, exportcsv_lasterror());

        exit(1);
    }

    if (exportcsv_writeln("Gmt time,Ask,Bid,AskVolume,BidVolume\r\n") != 0)
    {
        printf("Fatal error: Unable to write to file – %s\n", exportcsv_lasterror());

        exit(1);
    }

    int begin_week = get_week_number(config.begin);
    int end_week   = get_week_number(config.end);

    char archive_name[250];
    int result;

    for (int w = begin_week; w <= end_week; w++)
    {
        sprintf(archive_name, "/var/lib/hft/historical/%s/%s_WEEK%d.zip", config.instrument, config.instrument, w);

        if (config.verbose)
        {
            printf("Processing [%s] ...", archive_name);
        }

        result = zipped_csv_read_all_records(archive_name, &on_record);

        if (result == 0)
        {
            if (config.verbose)
            {
                printf("OK\n");
            }
        }
        else
        {
            if (config.verbose)
            {
                printf("ERROR\n");
            }

            printf("Error while processing file [%s] – %s\n", archive_name, zipped_csv_get_last_error_str());

            exit(1);
        }
    }

    exportcsv_close();

    return 0;
}