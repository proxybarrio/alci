# Alci: risultati misurati

MartyPC, IBM 5160 con GLaBIOS, CGA, 640 KB. 8088 a 4,77 MHz salvo indicazione.
Script: `alci/scripts/bench.sh` e `bench2.sh`.

| Operazione | Originale | Ora | Patch |
| --- | --- | --- | --- |
| `cat` di 200 righe a schermo | 17,7 s | 1,54 s | console veloce, scroll hardware, tty a blocchi |
| `ls -l /bin`, dati in memoria | 10,4 s | 4,1 s | namei, _wake_up, iget, date, fputs |
| `ls -l /bin`, dati da floppy | 14,7 s | 7,9 s | come sopra |
| Pipe a tre stadi, 20 KB | 3,86 s | 1,13 s | buffer pipe 512 byte |
| `grep` su 20 KB, file in memoria | 3,59 s | 2,89 s | fputs a blocchi |
| 10 avvii di `pwd` | 2,26 s | 2,03 s | vari |
| Avvio fino al prompt, con rotazione floppy (`ELKS_FDROT=9`) | 21,0 s | 18,4 s | caricatore a blocchi, lettura su due facce |

Su 8086 a 4,77 MHz i guadagni in proporzione sono gli stessi; dove conta il
floppy calano un poco.

## Costi

- Kernel: circa +550 byte di codice in tutto, +34 byte di dati.
- Programmi che usano `fputs`: circa +150 byte; programmi che usano le date: +80 byte.
- Pipe: 432 byte di RAM in più per ogni pipe aperta.
- Settore di avvio: usati i 69 byte che restavano liberi.

## Sperimentale

- Lettura del floppy su due facce (`bioshd.c`): +15% sui file grandi,
  +2-3% sui file piccoli; ripiego automatico se il BIOS rifiuta.

## Conferma su 86Box (15/11/2026)

86Box 6.0, IBM PC/XT, 8088 a 4,77 MHz, IBM CGA, floppy 3,5" 720 KB, immagini
`elks-originale-720.img` e `alci-720.img`. Tempi misurati con `time` dentro ELKS.

| Operazione | Originale | Alci | Guadagno |
| --- | --- | --- | --- |
| Avvio del kernel fino al login | 13,53 s | 12,37 s | -9% |
| `ls -l /bin`, prima volta (floppy) | 17,77 s | 10,00 s | 1,8x |
| `ls -l /bin`, seconda volta (memoria) | 12,17 s | 4,32 s | 2,8x |
| `cat` di `ls -l /bin /dev` (~180 righe), prima volta | 12,32 s | 3,02 s | 4,1x |
| stesso `cat`, seconda volta (memoria) | 12,29 s | 1,23 s | 10x |

I guadagni confermano MartyPC (1,9x e 2,5x); in 86Box i tempi assoluti sono
più alti, probabilmente per una simulazione più fedele del floppy e dello schermo.
