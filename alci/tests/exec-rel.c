/* Medium-model test program with text, far text and data relocations, for exec-corrupt.py.
 * Build: ia16-elf-gcc -mcmodel=medium -melks-libc -mtune=i8086 -Os -o rel exec-rel.c */
#include <stdio.h>
#include <string.h>
static char buf[2000];
int __far f1(int x){ return x*3+1; }
int __far f2(int x){ return f1(x)+2; }
int main(int argc,char**argv){ int i,s=0; memset(buf,'a',sizeof buf); for(i=0;i<100;i++) s+=f2(i); printf("rel ok %d %d\n",s,(int)strlen(argv[0])); return 0; }
