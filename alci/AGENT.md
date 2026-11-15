# Alci: istruzioni per l'agente programmato

Alci è un ramo di ELKS (account GitHub `proxybarrio`): un ELKS più veloce
e più parco di memoria, verso una GUI sul modello di GEOS e un terminale per Claude,
su qualunque PC dall'8088 al 286. Ogni esecuzione programmata fa UN compito
di `alci/TASKS.md`, lo misura, salva e scrive un resoconto.

## Regole fisse

1. Scrivi SOLO su `proxybarrio/elks` (ramo `alci`) e `proxybarrio/martypc`
   (ramo `elks-bench`). Mai push su `ghaerr/elks` o `dbalsom/martypc`, mai issue,
   pull request o commenti upstream: quelli li manda il maintainer.
2. Mai `push --force`, mai riscrivere commit già pubblicati.
3. Un compito per esecuzione. Se è troppo grande, dividilo in `TASKS.md` e fai
   solo il primo pezzo.
4. La memoria viene prima della velocità: ogni patch dichiara byte di codice
   kernel, byte per programma, RAM occupata. Una patch che peggiora un caso
   misurato o occupa RAM in modo permanente senza motivo va respinta.
5. Riferimento: 8088 a 4,77 MHz con CGA (MartyPC). Le patch che dipendono dal
   BIOS devono avere un ripiego automatico e restano "sperimentali" finché
   non sono provate su hardware vero.
6. Niente decisioni irreversibili da solo: preparale e segnale nel resoconto.

## Procedura

1. Prepara l'ambiente:
   - `git clone --depth 50 -b alci https://github.com/proxybarrio/elks /home/claude/elks`
   - `bash /home/claude/elks/alci/scripts/setup.sh` (toolchain precompilata,
     MartyPC con il runner ELKS, primo build). Deve stampare `ALCI SETUP OK`.
2. Leggi `alci/TASKS.md`, `alci/RESULTS.md` e le ultime voci di `alci/LOG.md`.
   Scegli il primo compito con stato `TODO` (salta `HUMAN` e `BLOCKED`).
3. Misura prima: `alci/scripts/bench.sh base` e, se serve,
   `alci/scripts/bench2.sh base`. Profilo: variabili `ELKS_PROF`,
   `ELKS_PROF_FROM`, `ELKS_SAMPLE`, `ELKS_PROF_BOOT`, `ELKS_FDROT=9`
   (rotazione del floppy) nel runner `martypc_headless` (vedi
   `crates/bin/martypc_headless/src/run_elks.rs` nel fork MartyPC);
   `alci/scripts/prof.py profilo simboli_utente` per leggere il profilo.
4. Lavora, poi misura dopo con un'etichetta nuova. Verifica che l'output a
   schermo dei comandi resti identico a prima (confronta le schermate).
   Dopo modifiche alla libc serve `make -C elkscmd clean`; dopo modifiche
   a un header, `make -C elks clean`.
5. Se il compito non porta un guadagno misurabile o peggiora qualcosa,
   annulla le modifiche e registra il risultato negativo: è un risultato.
6. Aggiorna `alci/TASKS.md` (stato, note), `alci/RESULTS.md` (tabella) e
   aggiungi una voce datata in `alci/LOG.md`.
7. Commit sul ramo `alci` (un commit per modifica logica, messaggio in
   inglese stile ELKS `[area] descrizione`), poi
   `git -c push.negotiate=true push origin alci`.
8. Messaggio finale (arriva al maintainer via mail), in italiano, dando del tu,
   senza trattini lunghi, breve: cosa hai fatto, i numeri prima e dopo,
   il costo in byte, cosa resta da decidere a lui, il prossimo compito.
