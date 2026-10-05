# Gestionale MGS

Applicazione desktop C++ per la gestione dei dati di un maglificio. Il programma utilizza un database MySQL online e offre sia un'interfaccia grafica Win32 sia una modalita da terminale.

## Funzionalita

- gestione di prodotti, clienti, filati, tecnici, fornitori e trattamenti di lavanderia;
- inserimento, ricerca, modifica, eliminazione e stampa dei record;
- ordinamento crescente per chiave primaria;
- relazioni tra prodotti, clienti, filati, tecnici e stagioni;
- assegnazione opzionale di un trattamento di lavanderia a ciascun prodotto;
- immagini dei prodotti archiviate come BLOB nel database;
- anteprima dell'immagine con lente di ingrandimento interattiva;
- schermata animata di avvio;
- verifica automatica dello schema e della connessione.

## Tecnologie

- C++20;
- Visual Studio 2026;
- Win32 API e GDI+;
- MySQL 8 e MySQL Connector/C++;
- database cloud Aiven.

## Struttura principale

- `GestionaleMGS/GestionaleMGS.cpp`: avvio e selezione della modalita;
- `GestionaleMGS/DatabaseManager.*`: accesso e operazioni MySQL;
- `GestionaleMGS/InterfacciaGrafica.*`: interfaccia desktop;
- `GestionaleMGS/Menu.*`: interfaccia da terminale;
- `GestionaleMGS/FinestraRecord.*`: finestre per inserimento e modifica;
- classi di dominio: `Prodotto`, `Cliente`, `Filato`, `Tecnico`, `Fornitore` e `Immagine`.

## Configurazione del database

Le credenziali non sono incluse nel repository. Copiare `database.conf.example` in uno dei seguenti percorsi e sostituire i segnaposto:

1. `%APPDATA%\GestionaleMGS\database.conf`;
2. `database.conf` nella stessa cartella dell'eseguibile.

In alternativa sono supportate le variabili d'ambiente:

```text
MGS_DB_HOST
MGS_DB_USER
MGS_DB_PASSWORD
MGS_DB_NAME
```

Per una dimostrazione usare un database separato e un utente Aiven temporaneo con i soli privilegi necessari. Non utilizzare l'account amministratore nel pacchetto distribuito.

## Compilazione

1. Aprire `GestionaleMGS.slnx` con Visual Studio 2026.
2. Selezionare `Release` e `x64`.
3. Compilare la soluzione.
4. L'eseguibile viene generato in `x64/Release/GestionaleMGS.exe` insieme alle DLL richieste.

Il progetto contiene gia gli header, le librerie e le DLL di MySQL Connector/C++ usate dalla soluzione.

## Avvio

- `GestionaleMGS.exe`: interfaccia grafica;
- `GestionaleMGS.exe --console`: menu da terminale;
- `GestionaleMGS.exe --verify`: verifica schema, connessione e letture principali.

Il computer di destinazione deve essere Windows x64 e potrebbe richiedere il Microsoft Visual C++ Redistributable corrispondente alla versione utilizzata per la compilazione.

## Sicurezza

- `database.conf` e gli ZIP di distribuzione sono esclusi da Git;
- nessuna password deve essere inserita nel codice sorgente;
- al termine della dimostrazione revocare o cambiare la password dell'utente Aiven temporaneo;
- usare esclusivamente dati dimostrativi privi di informazioni sensibili.
