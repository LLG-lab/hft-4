#ifndef TIMESTAMP_H
#define TIMESTAMP_H

typedef unsigned long int timestamp_t;

timestamp_t mk_timestamp(int day, int mon, int year, int hour, int minute, int sec, int microsecond);

#endif /* TIMESTAMP_H */