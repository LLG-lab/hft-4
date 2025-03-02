#ifndef EXPORT_CSV_H
#define EXPORT_CSV_H

int exportcsv_create(const char *file_name);
int exportcsv_writeln(const char *line);
void exportcsv_close(void);
const char *exportcsv_lasterror(void);

#endif /* EXPORT_CSV_H */
