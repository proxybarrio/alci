/* Descriptors for the desktop's event loop to wait on (main.c): when fd has
   data, ev is posted to process id, once; call watch_fd() again after
   reading to be told of the next data. */
#ifndef WATCH_H
#define WATCH_H
#include "ek.h"
void watch_fd(int fd, ek_id_t id, ek_event_t ev);
void unwatch_fd(int fd);
#endif
