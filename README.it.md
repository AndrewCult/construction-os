[English](README.md) | [Italiano](README.it.md)

# Costruzione di un sistema operativo

Un sistema operativo didattico costruito da zero per l'architettura x86 a 32
bit. Il progetto parte da un piccolo sistema testuale e punta a evolvere, un
passo alla volta, verso un ambiente grafico leggero dallo stile rétro.

Lo scopo principale non è creare un sostituto per un sistema operativo
esistente, ma comprenderne il funzionamento al livello pratico più basso:
dall'avvio e dall'inizializzazione della CPU fino a memoria, interrupt,
processi, filesystem, driver, programmi e gestione dei pacchetti.

> [!IMPORTANT]
> Questo progetto si trova in una fase iniziale di apprendimento e
> sperimentazione. Al momento non è adatto a conservare dati importanti o a un
> utilizzo generico.

## Obiettivi del progetto

- Costruire un kernel freestanding principalmente in C, usando componenti
  assembly piccoli e documentati dove necessario.
- Scegliere come obiettivo l'architettura x86 a 32 bit (`i686`).
- Imparare ogni sottosistema implementandone una versione minima prima di
  estenderlo.
- Iniziare con un'interfaccia testuale ed esplorare in seguito un ambiente a
  finestre rétro.
- Progettare un semplice formato di pacchetto nativo e il relativo gestore.
- Sviluppare ed eseguire il debug in sicurezza con QEMU prima dei test su
  hardware reale.
- Esplorare, dove possibile, il supporto per computer datati e con risorse
  limitate.
- Documentare architettura e percorso di apprendimento in inglese e italiano.

## Ambito iniziale

Il primo traguardo principale è un'immagine avviabile capace di:

1. caricare un kernel `i686`;
2. scrivere informazioni diagnostiche sullo schermo e sulla porta seriale;
3. gestire eccezioni fondamentali della CPU, timer e input da tastiera;
4. offrire una shell e un filesystem minimali;
5. caricare un semplice programma;
6. installare un pacchetto locale basilare.

Il supporto hardware oltre la piattaforma emulata di riferimento verrà
aggiunto progressivamente. Funzioni specifiche dei portatili come Wi-Fi,
gestione della batteria, sospensione e ripresa, audio e grafica accelerata non
rientrano nell'ambito iniziale.

## Toolchain prevista

| Componente | Scelta prevista |
| --- | --- |
| Linguaggio principale | C17, sottoinsieme freestanding |
| Codice a basso livello | Assembly x86 con NASM |
| Architettura di destinazione | `i686-elf` |
| Compilatore e strumenti binari | Cross-compiler GCC e GNU Binutils |
| Sistema di build | GNU Make |
| Bootloader | GRUB con Multiboot |
| Formato del kernel | ELF |
| Emulatore | QEMU |
| Debugger | GDB |

Queste scelte potranno evolvere quando il progetto farà emergere nuovi
requisiti. Le decisioni architetturali importanti verranno documentate invece
di essere modificate silenziosamente.

## Roadmap

- [ ] Preparare e verificare l'ambiente di cross-development
- [ ] Avviare il primo kernel in QEMU
- [ ] Aggiungere output testuale VGA e seriale
- [ ] Implementare eccezioni della CPU e interrupt hardware
- [ ] Aggiungere timer e input da tastiera
- [ ] Introdurre la gestione della memoria fisica e virtuale
- [ ] Aggiungere processi, modalità utente e chiamate di sistema
- [ ] Implementare archiviazione e filesystem minimale
- [ ] Costruire una shell e caricare programmi utente
- [ ] Progettare il formato dei pacchetti e il relativo gestore
- [ ] Effettuare test su hardware datato selezionato
- [ ] Esplorare una TUI e, successivamente, un sistema a finestre grafico

La roadmap indica una direzione, non un calendario di pubblicazione rigido.
Ogni voce verrà suddivisa in traguardi piccoli e verificabili con l'avanzare
dello sviluppo.

## Stato del repository

Il repository contiene attualmente le fondamenta del progetto e la
documentazione. Le istruzioni di compilazione verranno aggiunte insieme al
primo kernel avviabile, in modo che ogni comando documentato possa essere
verificato e riprodotto.

## Lingue e convenzioni

La documentazione è disponibile in inglese e italiano. In caso di differenze
temporanee tra le traduzioni, l'inglese costituisce la versione canonica.

- Codice sorgente, identificatori, commenti, log e messaggi di errore sono in
  inglese.
- Messaggi di commit, titoli delle issue e terminologia tecnica sono in inglese.
- La documentazione inglese risiede in `README.md` e, in seguito, `docs/en/`.
- La documentazione italiana risiede in `README.it.md` e, in seguito,
  `docs/it/`.
- I documenti corrispondenti dovrebbero mantenere la stessa struttura nelle due
  lingue.

## Contribuire

Il progetto è attualmente sviluppato come percorso personale di apprendimento.
Suggerimenti, correzioni tecniche e riferimenti didattici sono benvenuti tramite
le issue di GitHub. Le linee guida per contribuire verranno aggiunte quando il
codice sarà pronto ad accogliere contributi esterni.

## Licenza

Il progetto è distribuito con [licenza MIT](LICENSE).
