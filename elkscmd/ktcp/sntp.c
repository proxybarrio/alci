/*
 * SNTP client (RFC 4330): sets the system clock in UTC once the network
 * is configured. Servers are tried in order: the NTP server given by DHCP
 * (option 42), then those given with -t. Each server gets SNTP_TRIES
 * requests SNTP_TIMEOUT apart; the first valid answer sets the clock and
 * the client stops. Uses the internal UDP of ktcp (see udp.c).
 */
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "config.h"
#include "ip.h"
#include "udp.h"
#include "timer.h"
#include "netconf.h"
#include "sntp.h"

#ifndef BUILD_TIME
#define BUILD_TIME	0L
#endif

#define NTP_PORT	123
#define SNTP_PORT	1123		/* local port */
#define SNTP_LEN	48
#define SNTP_TIMEOUT	(2 << 4)	/* 2 s in 1/16 s units */
#define SNTP_TRIES	2
#define SNTP_MAXJUMP	1000L		/* report corrections above this (s) */
#define NTP_UNIX	2208988800UL	/* 1900 to 1970 in seconds */

char sntp_timer_active;
ipaddr_t sntp_servers[SNTP_MAX_SERVERS];
int sntp_nservers;

static int sntp_cur;			/* server index */
static int sntp_tries;
static timeq_t sntp_sent;		/* when the request left */
static __u32 sntp_nonce;		/* our transmit timestamp, echoed back */

static void sntp_send(void)
{
    __u32 pkt[SNTP_LEN / 4];

    memset(pkt, 0, SNTP_LEN);
    pkt[0] = htonl(0x23000000UL);	/* LI 0, version 4, mode 3 (client) */
    sntp_nonce = (Now << 16) ^ (__u32)local_ip ^ sntp_tries;
    pkt[10] = htonl(sntp_nonce);	/* transmit timestamp, seconds */
    sntp_sent = Now;
    udp_send(sntp_servers[sntp_cur], NTP_PORT, SNTP_PORT,
	(unsigned char *)pkt, SNTP_LEN, local_ip);
    sntp_timer_active = 1;
}

static void sntp_stop(void)
{
    sntp_timer_active = 0;
    udp_unregister(SNTP_PORT);
}

static void sntp_input(struct iphdr_s *iph, __u16 src_port, unsigned char *data, int len)
{
    __u32 *pkt = (__u32 *)data;
    unsigned long secs, frac;
    unsigned char b0, stratum;
    struct timeval tv, old;
    long diff;

    if (!sntp_timer_active || src_port != NTP_PORT || len < SNTP_LEN ||
	iph->saddr != sntp_servers[sntp_cur])
	return;
    b0 = data[0];
    stratum = data[1];
    if ((b0 & 7) != 4 || (b0 >> 6) == 3 ||	/* server mode, clock in sync */
	stratum == 0 || stratum > 15 ||
	ntohl(pkt[6]) != sntp_nonce || pkt[7] != 0)	/* our request */
	return;
    secs = ntohl(pkt[10]);			/* server transmit timestamp */
    frac = ntohl(pkt[11]);
    if (secs < NTP_UNIX + BUILD_TIME) {
	printf("sntp: %s: time too old\n", in_ntoa(iph->saddr));
	return;
    }
    tv.tv_sec = secs - NTP_UNIX;
    /* fraction to microseconds, plus half the round trip (60 ms units) */
    tv.tv_usec = (long)(((frac >> 16) * 15625UL) >> 10) +
	(long)(get_time() - sntp_sent) * 30000L;
    while (tv.tv_usec >= 1000000L) {
	tv.tv_usec -= 1000000L;
	tv.tv_sec++;
    }
    gettimeofday(&old, NULL);
    diff = tv.tv_sec - old.tv_sec;
    if (settimeofday(&tv, NULL) < 0) {
	printf("sntp: can't set time\n");
    } else {
	printf("sntp: time from %s", in_ntoa(iph->saddr));
	if (old.tv_sec >= BUILD_TIME && (diff > SNTP_MAXJUMP || diff < -SNTP_MAXJUMP))
	    printf(", corrected by %ld s", diff);
	printf("\n");
    }
    sntp_stop();
}

/* start once the network is configured (static address, or DHCP bound) */
void sntp_start(void)
{
    if (!sntp_nservers || !udp_register(SNTP_PORT, sntp_input))
	return;
    sntp_cur = 0;
    sntp_tries = 0;
    sntp_send();
}

void sntp_timer(void)
{
    if (!TIME_GEQ(Now, sntp_sent + SNTP_TIMEOUT))
	return;
    if (++sntp_tries >= SNTP_TRIES) {
	sntp_tries = 0;
	if (++sntp_cur >= sntp_nservers) {
	    printf("sntp: no answer\n");
	    sntp_stop();
	    return;
	}
    }
    sntp_send();
}

/* add a server; DHCP's goes first */
void sntp_add_server(ipaddr_t ip, int first)
{
    int i;

    if (!ip || sntp_nservers >= SNTP_MAX_SERVERS)
	return;
    if (first) {
	for (i = sntp_nservers; i > 0; i--)
	    sntp_servers[i] = sntp_servers[i - 1];
	sntp_servers[0] = ip;
    } else
	sntp_servers[sntp_nservers] = ip;
    sntp_nservers++;
}
