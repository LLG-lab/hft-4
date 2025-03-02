#ifndef ZIPPED_CSV_H
#define ZIPPED_CSV_H

#include "timestamp.h"

typedef void (*on_record_function)(const char *, timestamp_t);


int zipped_csv_read_all_records(const char *zip_name, on_record_function f);

const char *zipped_csv_get_last_error_str(void);


#endif /* ZIPPED_CSV_H */
