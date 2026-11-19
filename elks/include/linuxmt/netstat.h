#ifndef __LINUXMT_NETSTAT_H
#define __LINUXMT_NETSTAT_H

#define MAX_ETHS	5	/* max # of ethernet device minor numbers (=max NICs) */

/* Indices of the eths[] array, must match minor device numbers */
#define ETH_NE0 	0	/* Novell NE & compatibles, including 8bit versions */
#define ETH_WD0 	1	/* Western Digital 8003 and 8013, compatibles */
#define ETH_EL3		2	/* 3Com 3C503 Etherlink III */
#define ETH_UL0		3	/* SMC Ultra/UltraChip 83C790 ISA */
#define ETH_LE0		4	/* AMD PCnet-PCI 79C970 and LANCE/79C960/NE1500/NE2100 */

#ifndef __ASSEMBLER__

#include <arch/ports.h>	/* struct isa_conf */
#include <linuxmt/init.h>	/* DRVFAR */

/* /bootopts-parsed port, irq, ram and flags */
/* Should put this into the eths struct */
extern struct isa_conf ne0_conf;
extern struct isa_conf wd0_conf;
extern struct isa_conf el3_conf;
extern struct isa_conf ul0_conf;
extern struct isa_conf le0_conf;

struct inode;
struct file;

/* ALCI: driver operations are far, so that the code of a driver can sit in
 * its own far section, released at boot when its card is not found */
typedef size_t DRVFAR eth_rw_fn(struct inode *, struct file *, char *, size_t);
typedef int DRVFAR eth_select_fn(struct inode *, struct file *, int);
typedef int DRVFAR eth_ioctl_fn(struct inode *, struct file *, unsigned int, unsigned int);
typedef int DRVFAR eth_open_fn(struct inode *, struct file *);
typedef void DRVFAR eth_release_fn(struct inode *, struct file *);

struct eth_farops {
    eth_rw_fn *read;
    eth_rw_fn *write;
    eth_select_fn *select;
    eth_ioctl_fn *ioctl;
    eth_open_fn *open;
    eth_release_fn *release;
};

struct eth {
    struct eth_farops *farops;          /* NULL when no card was found */
};
extern struct eth eths[];

/* status for each NIC, returned through ioctl */
struct netif_stat {
	__u16 rx_errors;	/* Receive errors, flagged by NIC */
	__u16 rq_errors;	/* Receive queue errors (in 8bit interfaces) */
	__u16 tx_errors;	/* Transmit errors, flagged by NIC */
	__u16 oflow_errors;	/* Receive buffer overflow interrupts */
	__u16 if_status;	/* Interface status flags */
	__s16 oflow_keep;	/* # of packets to keep if overflow */
	__u8  mac_addr[6];	/* Current MAC address */
};

#endif	/* __ASSEMBLER__ */

/* Status flags for if_status.
 * The lower nibble has the autodetected buffer memory size, 1=4k, 2=8k, 3=16k etc.
 */
#define	NETIF_AUTO_8BIT	0x10	
#define NETIF_IS_QEMU	0x20	/* unused */

/* Network card configuration flags specified in /bootopts.
 * The low 3 bits specify the buffer size (0-4), the rest are regular flag bits.
 */
#define ETHF_DEF_BUF	0x00	/* Use default (i.e. max) NIC buffer */
#define ETHF_8K_BUF	0x01	/* Force  8K NIC (default on SMC/WD memory mapped NICs) */
#define ETHF_16K_BUF	0x02	/* Force 16k NIC buffer */
#define ETHF_32K_BUF	0x03	/* Force 32k NIC buffer */
#define ETHF_4K_BUF	0x04	/* Force  4k NIC buffer */
#define ETHF_BUF_MASK	0x07	/* mask for all buffer sizes */
#define ETHF_8BIT_BUS	0x10	/* Force  8 bit bus */
#define ETHF_16BIT_BUS 	0x20	/* Force 16 bit bus */
#define ETHF_USE_BNC	0x08	/* Use BNC connection */
#define ETHF_USE_AUI	0x40	/* Use AUI connection */
#define ETHF_VERBOSE	0x80	/* turn on verbose console error messages */

#endif
