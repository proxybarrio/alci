#ifndef SNTP_H
#define SNTP_H

#define SNTP_MAX_SERVERS	4

extern char sntp_timer_active;
extern int sntp_nservers;

void sntp_start(void);
void sntp_timer(void);
void sntp_add_server(ipaddr_t ip, int first);

#endif
