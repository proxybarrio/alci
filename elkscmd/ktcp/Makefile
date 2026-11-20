# Makefile for ktcp
BASEDIR=..
include $(BASEDIR)/Makefile-rules

SHELL		= /bin/sh

CFILES		= ktcp.c slip.c ip.c icmp.c tcp.c tcp_cb.c tcp_output.c \
		  timer.c tcpdev.c netconf.c vjhc.c deveth.c arp.c udp.c dhcp.c sntp.c

OBJS		= $(CFILES:.c=.o)

# SNTP refuses times before the build
sntp.o: LOCALFLAGS += -DBUILD_TIME=$(shell date -u +%s)L

##############################################################################

all:	ktcp

ktcp:	$(OBJS)
	$(LD) $(LDFLAGS) -maout-heap=33792 -maout-stack=3072  -o ktcp $(OBJS) $(LDLIBS)

install: ktcp
	$(INSTALL) ktcp $(DESTDIR)/bin

clean: 
	$(RM) *.o ktcp

dep:
	makedepend $(CFILES)
