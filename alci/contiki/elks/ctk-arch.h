/* Contiki CTK on ELKS: keyboard codes (prototype) */
#ifndef __CTK_ARCH_H__
#define __CTK_ARCH_H__
#include <ctype.h>
typedef unsigned char ctk_arch_key_t;
unsigned char ctk_arch_keyavail(void);
ctk_arch_key_t ctk_arch_getkey(void);
#define CH_CURS_UP    0x80
#define CH_CURS_DOWN  0x81
#define CH_CURS_LEFT  0x82
#define CH_CURS_RIGHT 0x83
#define CH_F1         0x84
#define CH_F2         0x85
#define CH_F3         0x86
#define CH_F4         0x87
#define CH_F5         0x88
#define CH_ENTER      '\r'
#define CH_TAB        '\t'
#define CH_DEL        0x08
#define CH_ESC        0x1b
#define ctk_arch_isprint(x) isprint(x)
#endif
