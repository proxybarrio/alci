# Alci: compiti in ordine

Stati: `TODO`, `DOING`, `DONE`, `DROPPED` (provato, non conviene),
`HUMAN` (serve il maintainer), `BLOCKED` (aspetta qualcosa).
L'agente prende il primo `TODO` dall'alto.

## Fase 1: velocità di base

- [ ] A1 `TODO` Avvio dei programmi. Oggi 10 avvii di `pwd` costano ~2,0 s su
  8088 (bench2). Profilare `exec`/`fork`/caricamento a.out, ridurre il costo
  senza aumentare la RAM occupata.
- [ ] A2 `TODO` Timer: il kernel spende ~7,6% della CPU anche a riposo
  (tick a 100 Hz). Ridurre il lavoro per tick o saltare i tick a vuoto.
  Misurare anche l'effetto sul tempo dei comandi.
- [ ] A3 `TODO` Ordine dei file sull'immagine floppy: mettere kernel, shell,
  `bootopts` e file letti all'avvio vicini e nell'ordine di lettura.
  Misurare l'avvio con `ELKS_FDROT=9`.

## Fase 2: memoria

- [ ] B1 `TODO` Misurare e documentare dove va la memoria del kernel
  (codice, dati, heap, buffer, cache) in `alci/docs/memoria.md`.
- [ ] B2 `TODO` Cache del disco elastica: solo letture, scritture subito su
  disco, svuotata a mount/umount e cambio disco, in memoria convenzionale al
  massimo 64 KB ceduti ai programmi su richiesta; in EMS o memoria estesa
  quando presenti. Prima un documento di progetto in `alci/docs/`, poi
  l'implementazione a passi.

## Fase 3: prove su hardware vario

- [ ] C1 `TODO` Kit di prova: immagini 360 KB, 720 KB e 1,44 MB con tutte le
  patch e uno script di misura che giri su ELKS e stampi i tempi.
- [ ] C2 `HUMAN` Provare il kit su macchine vere (T1200, un XT, un AT) e
  riportare i numeri. Verificare la lettura su due facce del floppy.

## Fase 4: grafica

- [ ] D1 `TODO` Studiare PC/GEOS (fork `proxybarrio/pcgeos`): gestione della
  memoria, segmenti scartabili, caratteri, finestre. Verificarne la licenza.
  Risultato: `alci/docs/geos-studio.md`.
- [ ] D2 `TODO` Misurare Nano-X su 8088 con CGA in MartyPC come motore di
  disegno (memoria, tempo per finestra e testo).
- [ ] D3 `TODO` Progetto del caricatore di segmenti di codice scartabili nel
  kernel: documento, poi prototipo.

## Fase 5: terminale Claude

- [ ] E1 `TODO` Verificare se Zimodem/WiFi232 gestisce HTTPS verso
  `api.anthropic.com` (documentazione e sorgenti del firmware). Risultato in
  `alci/docs/claude-terminale.md`.
- [ ] E2 `TODO` Client `claude` per ELKS a riga di comando sulla seriale
  (comandi AT, richiesta HTTP, JSON minimo), con un finto modem per i test.
- [ ] E3 `HUMAN` Comprare o montare il modem Wi-Fi e provarlo.

## Fase 6: immagine universale

- [ ] F1 `TODO` Rilevamento del processore all'avvio e scelta delle routine
  (8088, 186, 286); memoria estesa del 286 per la cache.

## Fatti finora (sessione del 15/11/2026)

Vedi `alci/RESULTS.md`. Patch sul ramo `alci`, un commit ciascuna.
