//
//  Menu.cpp
//  GestionaleMGS
//

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include <cctype>

#include "Menu.hpp"
#include "Prodotto.hpp"
#include "Cliente.hpp"
#include "Tecnico.hpp"
#include "Filato.hpp"
#include "Fornitore.hpp"
#include "DatabaseManager.hpp"

using namespace std;

namespace
{
    string normalizzaTesto(string testo)
    {
        auto inizio = find_if_not(testo.begin(), testo.end(),
            [](unsigned char c) { return isspace(c); });
        auto fine = find_if_not(testo.rbegin(), testo.rend(),
            [](unsigned char c) { return isspace(c); }).base();

        if (inizio >= fine)
            return "";

        string pulito(inizio, fine);
        transform(pulito.begin(), pulito.end(), pulito.begin(),
            [](unsigned char c) { return static_cast<char>(tolower(c)); });
        return pulito;
    }

    bool testoContiene(const string& testo, const string& ricerca)
    {
        string valore = normalizzaTesto(testo);
        string filtro = normalizzaTesto(ricerca);
        return !filtro.empty() && valore.find(filtro) != string::npos;
    }

    bool nomeClienteCorrisponde(const Cliente& cliente, const string& ricerca)
    {
        return testoContiene(cliente.getNome(), ricerca);
    }

    bool nomeFilatoCorrisponde(const Filato& filato, const string& ricerca)
    {
        return testoContiene(filato.getNome(), ricerca);
    }

    bool nomeFornitoreCorrisponde(const Fornitore& fornitore, const string& ricerca)
    {
        return testoContiene(fornitore.getNome(), ricerca);
    }

    Cliente* scegliClientePerNome(vector<Cliente>& clienti, const string& ricerca)
    {
        vector<Cliente*> risultati;
        for (Cliente& cliente : clienti)
        {
            if (nomeClienteCorrisponde(cliente, ricerca))
                risultati.push_back(&cliente);
        }

        if (risultati.empty())
            return nullptr;

        if (risultati.size() == 1)
            return risultati.front();

        cout << "Sono stati trovati piu' clienti:" << endl;
        for (size_t i = 0; i < risultati.size(); ++i)
        {
            cout << i + 1 << " - ";
            risultati[i]->stampaRecordCliente();
        }

        size_t scelta = 0;
        do
        {
            cout << "Scegli il numero del cliente: ";
            cin >> scelta;
        } while (scelta < 1 || scelta > risultati.size());
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        return risultati[scelta - 1];
    }

    Filato* scegliFilatoPerNome(vector<Filato>& filati, const string& ricerca)
    {
        vector<Filato*> risultati;
        for (Filato& filato : filati)
        {
            if (nomeFilatoCorrisponde(filato, ricerca))
                risultati.push_back(&filato);
        }

        if (risultati.empty())
            return nullptr;

        if (risultati.size() == 1)
            return risultati.front();

        cout << "Sono stati trovati piu' filati:" << endl;
        for (size_t i = 0; i < risultati.size(); ++i)
        {
            cout << i + 1 << " - ";
            risultati[i]->stampaRecordFilato();
        }

        size_t scelta = 0;
        do
        {
            cout << "Scegli il numero del filato: ";
            cin >> scelta;
        } while (scelta < 1 || scelta > risultati.size());
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        return risultati[scelta - 1];
    }

    Fornitore* scegliFornitorePerNome(vector<Fornitore>& fornitori, const string& ricerca)
    {
        vector<Fornitore*> risultati;
        for (Fornitore& fornitore : fornitori)
        {
            if (nomeFornitoreCorrisponde(fornitore, ricerca))
                risultati.push_back(&fornitore);
        }

        if (risultati.empty())
            return nullptr;

        if (risultati.size() == 1)
            return risultati.front();

        cout << "Sono stati trovati piu' fornitori:" << endl;
        for (size_t i = 0; i < risultati.size(); ++i)
        {
            cout << i + 1 << " - ";
            risultati[i]->stampaRecordFornitore();
        }

        size_t scelta = 0;
        do
        {
            cout << "Scegli il numero del fornitore: ";
            cin >> scelta;
        } while (scelta < 1 || scelta > risultati.size());
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        return risultati[scelta - 1];
    }

    string scegliNomeFornitoreEsistente(DatabaseManager* databaseManager)
    {
        vector<Fornitore> fornitori = databaseManager->caricaFornitori();
        if (!databaseManager->ultimoErrore().empty())
        {
            cout << "Errore lettura FORNITORI da MySQL: "
                << databaseManager->ultimoErrore() << endl;
            return "";
        }

        if (fornitori.empty())
        {
            cout << "Nessun fornitore attivo presente nel database. Inserire prima un fornitore." << endl;
            return "";
        }

        string nomeFornitore;
        Fornitore* fornitore = nullptr;
        do
        {
            cout << " ===== SCEGLI IL FORNITORE ===== " << endl;
            for (Fornitore& item : fornitori)
            {
                item.stampaRecordFornitore();
                cout << endl;
            }

            cout << "Nome fornitore esistente: ";
            getline(cin, nomeFornitore);
            fornitore = scegliFornitorePerNome(fornitori, nomeFornitore);

            if (fornitore == nullptr)
                cout << "Fornitore non trovato. Scegliere un fornitore gia' presente nel database." << endl;
        } while (fornitore == nullptr);

        return fornitore->getNome();
    }
    Cliente* trovaClientePerSerie(vector<Cliente>& clienti, int serie)
    {
        for (Cliente& cliente : clienti)
            if (cliente.getSerie() == serie)
                return &cliente;
        return nullptr;
    }

    Filato* trovaFilatoPerCodice(vector<Filato>& filati, int codice)
    {
        for (Filato& filato : filati)
            if (filato.getCodice() == codice)
                return &filato;
        return nullptr;
    }

    Tecnico* trovaTecnico(vector<Tecnico>& tecnici, const string& cognome, const string& nome)
    {
        string cognomeRicerca = normalizzaTesto(cognome);
        string nomeRicerca = normalizzaTesto(nome);
        for (Tecnico& tecnico : tecnici)
        {
            if (normalizzaTesto(tecnico.getCognome()) == cognomeRicerca
                && normalizzaTesto(tecnico.getNome()) == nomeRicerca)
                return &tecnico;
        }
        return nullptr;
    }

    Prodotto* trovaProdotto(vector<Prodotto>& prodotti, int articolo, int modello)
    {
        for (Prodotto& prodotto : prodotti)
            if (prodotto.getArticolo() == articolo && prodotto.getModello() == modello)
                return &prodotto;
        return nullptr;
    }
}

Menu::Menu(DatabaseManager& dbManager)
    : databaseManager(&dbManager)
{
}

void Menu::stampaProdottiDaDatabase()
{
    if (databaseManager == nullptr || !databaseManager->isConnesso())
    {
        cout << "Stampa da MySQL non disponibile: connessione al database assente." << endl;
        return;
    }

    vector<Prodotto> prodotti = databaseManager->caricaProdotti();
    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore lettura PRODOTTI da MySQL: "
            << databaseManager->ultimoErrore() << endl;
        return;
    }

    if (prodotti.empty())
    {
        cout << "Il DataBase dei PRODOTTI in MySQL e' vuoto!" << endl;
        return;
    }

    cout << "+---------- ELENCO PRODOTTI DA MYSQL ----------+" << endl;
    for (Prodotto& prodotto : prodotti)
    {
        prodotto.stampaRecordProdotto();
        cout << endl;
    }
}

void Menu::stampaClientiDaDatabase()
{
    if (databaseManager == nullptr || !databaseManager->isConnesso())
    {
        cout << "Stampa da MySQL non disponibile: connessione al database assente." << endl;
        return;
    }

    vector<Cliente> clienti = databaseManager->caricaClienti();
    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore lettura CLIENTI da MySQL: "
            << databaseManager->ultimoErrore() << endl;
        return;
    }

    if (clienti.empty())
    {
        cout << "Il DataBase dei CLIENTI in MySQL e' vuoto!" << endl;
        return;
    }

    cout << "+---------- ELENCO CLIENTI DA MYSQL ----------+" << endl;
    for (Cliente& cliente : clienti)
    {
        cliente.stampaRecordCliente();
        cout << endl;
    }
}

void Menu::stampaFilatiDaDatabase()
{
    if (databaseManager == nullptr || !databaseManager->isConnesso())
    {
        cout << "Stampa da MySQL non disponibile: connessione al database assente." << endl;
        return;
    }

    vector<Filato> filati = databaseManager->caricaFilati();
    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore lettura FILATI da MySQL: "
            << databaseManager->ultimoErrore() << endl;
        return;
    }

    if (filati.empty())
    {
        cout << "Il DataBase dei FILATI in MySQL e' vuoto!" << endl;
        return;
    }

    cout << "+---------- ELENCO FILATI DA MYSQL ----------+" << endl;
    for (Filato& filato : filati)
    {
        filato.stampaRecordFilato();
        cout << endl;
    }
}

void Menu::stampaFornitoriDaDatabase()
{
    if (databaseManager == nullptr || !databaseManager->isConnesso())
    {
        cout << "Stampa da MySQL non disponibile: connessione al database assente." << endl;
        return;
    }

    vector<Fornitore> fornitori = databaseManager->caricaFornitori();
    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore lettura FORNITORI da MySQL: "
            << databaseManager->ultimoErrore() << endl;
        return;
    }

    if (fornitori.empty())
    {
        cout << "Il DataBase dei FORNITORI in MySQL e' vuoto!" << endl;
        return;
    }

    cout << "+---------- ELENCO FORNITORI DA MYSQL ----------+" << endl;
    for (Fornitore& fornitore : fornitori)
    {
        fornitore.stampaRecordFornitore();
        cout << endl;
    }
}

void Menu::stampaTecniciDaDatabase()
{
    if (databaseManager == nullptr || !databaseManager->isConnesso())
    {
        cout << "Stampa da MySQL non disponibile: connessione al database assente." << endl;
        return;
    }

    vector<Tecnico> tecnici = databaseManager->caricaTecnici();
    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore lettura TECNICI da MySQL: "
            << databaseManager->ultimoErrore() << endl;
        return;
    }

    if (tecnici.empty())
    {
        cout << "Il DataBase dei TECNICI in MySQL e' vuoto!" << endl;
        return;
    }

    cout << "+---------- ELENCO TECNICI DA MYSQL ----------+" << endl;
    for (Tecnico& tecnico : tecnici)
    {
        tecnico.stampaRecordTecnico();
        cout << endl;
    }
}

int Menu::leggiScelta()
{
    int scelta;

    cout << "Scelta: ";
    cin >> scelta;

    return scelta;
}

void Menu::menuIniziale()
{
    cout << endl;
    cout << "+------------------------------------+" << endl;
    cout << "|      GESTIONALE MAGLIFICIO         |" << endl;
    cout << "+------------------------------------+" << endl;
    cout << "| 1 - Lavora con DataBase Prodotti   |" << endl;
    cout << "| 2 - Lavora con DataBase Clienti    |" << endl;
    cout << "| 3 - Lavora con DataBase Filati     |" << endl;
    cout << "| 4 - Lavora con DataBase Tecnici    |" << endl;
    cout << "| 5 - Lavora con DataBase Fornitori  |" << endl;
    cout << "| 0 - Esci                           |" << endl;
    cout << "+------------------------------------+" << endl;
    cout << endl;
}


void Menu::avvia()
{
    int scelta=0;

    do
    {
        menuIniziale();

        scelta = leggiScelta();

        switch (scelta)
        {
        case 1:
            sceltoProdotti();
            break;

        case 2:
            sceltoClienti();
            break;

        case 3:
            sceltoFilati();
            break;

        case 4:
            sceltoTecnici();
            break;

        case 5:
            sceltoFornitori();
            break;

        case 0:
            cout << "Arrivederci." << endl << endl;
            break;

        default:
            cout << "Scelta non valida." << endl;
        }

    } while (scelta != 0);
}

void Menu::menuProdotti()
{
    cout << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "|          GESTIONALE MAGLIFICIO           |" << endl;
    cout << "|          - DATABASE PRODOTTI -           |" << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "| 1 - Cerca Prodotto                       |" << endl;
    cout << "| 2 - Crea Nuovo Prodotto                  |" << endl;
    cout << "| 3 - Modifica Prodotto                    |" << endl;
    cout << "| 4 - Elimina Prodotto                     |" << endl;
    cout << "| 5 - Stampa DataBase Prodotti completo    |" << endl;
    cout << "| 0 - Indietro                             |" << endl;
    cout << "+------------------------------------------+" << endl;
    cout << endl;
}

void Menu::sceltoProdotti()
{
    int scelta=0;

    do
    {
        menuProdotti();

        scelta = leggiScelta();

        switch (scelta)
        {
        case 1:
            cercaProdotto();
            break;

        case 2:
            inserisciNewProdotto();
            break;

        case 3:
            modificaProdotto();
            break;

        case 4:
            eliminaProdotto();
            break;

        case 5:
            stampaProdottiDaDatabase();
            break;

        case 0:
            break;

        default:
            cout << "Scelta non valida." << endl;
        }

    } while (scelta != 0);
}


void Menu::inserisciNewProdotto()
{
    short stagione = 0;
    int anno = 0;
    int articolo = 0;
    int modello = 0;
    string descrizione;
    int codiceFilato = 0;
    int codiceCliente = 0;
    string cognome;
    string nome;

    vector<Filato> filati = databaseManager->caricaFilati();
    vector<Cliente> clienti = databaseManager->caricaClienti();
    vector<Tecnico> tecnici = databaseManager->caricaTecnici();

    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore caricamento dati da MySQL: " << databaseManager->ultimoErrore() << endl;
        return;
    }

    Filato* f = nullptr;
    Cliente* c = nullptr;
    Tecnico* t = nullptr;

    cout << "\n===== NUOVO PRODOTTO =====\n";

    do {
        cout << "Stagione (PE = 1 / AI = 2): ";
        cin >> stagione;
        if (stagione != 1 && stagione != 2)
            cout << "Stagione non valida, inserire 1 per PE o 2 per AI!" << endl;
    } while (stagione != 1 && stagione != 2);

    do {
        cout << "Anno (aaaa): ";
        cin >> anno;
        if (anno < 1900 || anno > 2100)
            cout << "Anno non valido, inserire un anno compreso tra 1900 e 2100!" << endl;
    } while (anno < 1900 || anno > 2100);

    do {
        cout << "Articolo: ";
        cin >> articolo;
        if (articolo > 10000 && articolo < 99999) {
            codiceFilato = articolo / 100;
            f = trovaFilatoPerCodice(filati, codiceFilato);
            if (f != nullptr) {
                cout << "Codice Filato scelto => ";
                f->stampaFilato();
                cout << endl;
            }
            else {
                cout << "Codice filato non esistente o non piu' attivo, inserire ARTICOLO e/o FILATO CORRETTO!" << endl;
            }
        }
        else
            cout << "Articolo non valido, inserire un numero compreso tra 10000 e 99999!" << endl;
    } while (articolo < 10000 || articolo > 99999 || f == nullptr);

    do {
        cout << "Modello: ";
        cin >> modello;
        if (modello > 10000 && modello < 99999) {
            codiceCliente = (modello / 100) - ((modello / 1000) * 10);
            c = trovaClientePerSerie(clienti, codiceCliente);
            if (c != nullptr) {
                cout << "Codice Cliente scelto => ";
                c->stampaCliente();
                cout << endl;
            }
            else {
                cout << "Codice Cliente non esistente o non piu' attivo, inserire MODELLO e/o CLIENTE CORRETTO!" << endl;
            }
        }
        else
            cout << "Modello non valido, inserire un numero compreso tra 10000 e 99999!" << endl;
    } while (modello < 10000 || modello > 99999 || c == nullptr);

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Descrizione: ";
    getline(cin, descrizione);

    do {
        cout << " ===== SCEGLI IL TECNICO ===== " << endl;
        stampaTecniciDaDatabase();
        cout << "Scegli il Tecnico tra quelli esistenti" << endl;
        cout << "Cognome tecnico: ";
        getline(cin, cognome);
        cout << endl;
        cout << "Nome tecnico: ";
        getline(cin, nome);
        cout << endl;
        t = trovaTecnico(tecnici, cognome, nome);
        if (t == nullptr)
            cout << "Cognome e/o nome tecnico non esistente, inserire COGNOME e/o NOME del tecnico corretto!" << endl;
    } while (t == nullptr);

    Prodotto p = { stagione, anno, articolo, modello, descrizione, *f, *c, *t };
    if (!databaseManager->inserisciProdotto(p))
    {
        cout << "Errore inserimento prodotto su MySQL: "
            << databaseManager->ultimoErrore() << endl << endl;
        return;
    }

    p.stampaProdotto();
    cout << endl;
    cout << "Nuovo Prodotto aggiunto correttamente!" << endl << endl;
}

void Menu::cercaProdotto() {
    int articolo = 0, modello = 0;

    cout << "Inserisci ARTICOLO " << endl;
    articolo = leggiScelta();

    cout << "Inserisci MODELLO " << endl;
    modello = leggiScelta();

    vector<Prodotto> prodotti = databaseManager->caricaProdotti();
    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore lettura PRODOTTI da MySQL: " << databaseManager->ultimoErrore() << endl;
        return;
    }

    Prodotto* p = trovaProdotto(prodotti, articolo, modello);

    if (p == nullptr)
    {
        cout << "Prodotto non trovato!" << endl;
        return;
    }

    cout << "\nProdotto trovato:\n";
    p->stampaProdotto();
}

void Menu::modificaProdotto() {
    int articolo = 0, modello = 0;

    cout << "Inserisci ARTICOLO: " << endl;
    articolo = leggiScelta();

    cout << "Inserisci MODELLO: " << endl;
    modello = leggiScelta();

    vector<Prodotto> prodotti = databaseManager->caricaProdotti();
    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore lettura PRODOTTI da MySQL: " << databaseManager->ultimoErrore() << endl;
        return;
    }

    Prodotto* prodotto = trovaProdotto(prodotti, articolo, modello);

    if (prodotto == nullptr)
    {
        cout << "Prodotto non trovato!" << endl;
        return;
    }

    int vecchioArticolo = articolo;
    int vecchioModello = modello;

    cout << "\nProdotto trovato:\n";
    prodotto->stampaProdotto();

    int scelta = 0;

    do
    {
        cout << "\n\n===== CAMPI MODIFICABILI =====" << endl;
        cout << "1 - Stagione" << endl;
        cout << "2 - Anno" << endl;
        cout << "3 - Articolo" << endl;
        cout << "4 - Modello" << endl;
        cout << "5 - Descrizione" << endl;
        cout << "6 - Filato" << endl;
        cout << "7 - Cliente" << endl;
        cout << "8 - Tecnico" << endl;
        cout << "0 - Fine modifica" << endl;

        scelta = leggiScelta();

        switch (scelta)
        {
        case 1:
        {
            short stagione;
            cout << "Nuova stagione (PE = 1 / AI = 2): ";
            cin >> stagione;
            prodotto->setStagione(stagione);
            break;
        }

        case 2:
        {
            int anno;
            cout << "Nuovo anno: ";
            anno = leggiScelta();
            prodotto->setAnno(anno);
            break;
        }

        case 3:
        {
            int nuovoArticolo;
            cout << "Nuovo articolo: ";
            nuovoArticolo = leggiScelta();
            prodotto->setArticolo(nuovoArticolo);
            break;
        }

        case 4:
        {
            int nuovoModello;
            cout << "Nuovo modello: ";
            nuovoModello = leggiScelta();
            prodotto->setModello(nuovoModello);
            break;
        }

        case 5:
        {
            string descrizione;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Nuova descrizione: ";
            getline(cin, descrizione);
            prodotto->setDescrizione(descrizione);
            break;
        }

        case 6:
        {
            int codiceFilato;
            Filato* filato = nullptr;
            vector<Filato> filati = databaseManager->caricaFilati();

            if (!databaseManager->ultimoErrore().empty())
            {
                cout << "Errore lettura FILATI da MySQL: " << databaseManager->ultimoErrore() << endl;
                break;
            }

            do
            {
                stampaFilatiDaDatabase();
                cout << endl << endl;
                cout << "Nuovo codice filato: ";
                codiceFilato = leggiScelta();

                filato = trovaFilatoPerCodice(filati, codiceFilato);

                if (filato == nullptr)
                    cout << "Filato non trovato!" << endl;

            } while (filato == nullptr);

            prodotto->setFilato(*filato);
            break;
        }

        case 7:
        {
            int serie = 0;
            Cliente* cliente = nullptr;
            vector<Cliente> clienti = databaseManager->caricaClienti();

            if (!databaseManager->ultimoErrore().empty())
            {
                cout << "Errore lettura CLIENTI da MySQL: " << databaseManager->ultimoErrore() << endl;
                break;
            }

            do
            {
                stampaClientiDaDatabase();
                cout << endl << endl;
                cout << "Nuova serie cliente: ";
                serie = leggiScelta();

                cliente = trovaClientePerSerie(clienti, serie);

                if (cliente == nullptr)
                    cout << "Cliente non trovato!" << endl;

            } while (cliente == nullptr);

            prodotto->setCliente(*cliente);
            break;
        }

        case 8:
        {
            string cognome;
            string nome;
            Tecnico* tecnico = nullptr;
            vector<Tecnico> tecnici = databaseManager->caricaTecnici();

            if (!databaseManager->ultimoErrore().empty())
            {
                cout << "Errore lettura TECNICI da MySQL: " << databaseManager->ultimoErrore() << endl;
                break;
            }

            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            do
            {
                stampaTecniciDaDatabase();
                cout << endl << endl;
                cout << "Nuovo COGNOME tecnico: ";
                getline(cin, cognome);
                cout << endl;
                cout << "Nuovo NOME tecnico: ";
                getline(cin, nome);
                cout << endl;

                tecnico = trovaTecnico(tecnici, cognome, nome);

                if (tecnico == nullptr)
                    cout << "Tecnico non trovato!" << endl;

            } while (tecnico == nullptr);

            prodotto->setTecnico(*tecnico);
            break;
        }

        case 0:
            if (!databaseManager->aggiornaProdotto(vecchioArticolo, vecchioModello, *prodotto))
            {
                cout << "Errore aggiornamento prodotto su MySQL: "
                    << databaseManager->ultimoErrore() << endl;
                return;
            }
            cout << "\nProdotto aggiornato correttamente.\n";
            break;

        default:
            cout << "Scelta non valida!" << endl;
        }

        if (scelta != 0)
        {
            cout << "\nRecord aggiornato:\n";
            prodotto->stampaProdotto();
        }

    } while (scelta != 0);
}

void Menu::eliminaProdotto()
{
    int articolo = 0, modello = 0;

    cout << "\n===== ELIMINA PRODOTTO =====\n";

    cout << "Inserisci ARTICOLO del prodotto da eliminare: ";
    articolo = leggiScelta();

    cout << "Inserisci MODELLO del prodotto da eliminare: ";
    modello = leggiScelta();

    vector<Prodotto> prodotti = databaseManager->caricaProdotti();
    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore lettura PRODOTTI da MySQL: " << databaseManager->ultimoErrore() << endl;
        return;
    }

    Prodotto* prodotto = trovaProdotto(prodotti, articolo, modello);

    if (prodotto == nullptr)
    {
        cout << "\nProdotto non trovato!" << endl;
        return;
    }

    cout << "\nProdotto trovato:\n";
    prodotto->stampaProdotto();

    string conferma;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    do
    {
        cout << "\nConfermi l'eliminazione? (SI/NO): ";
        getline(cin, conferma);

        for (char& c : conferma)
        {
            c = toupper(c);
        }

    } while (conferma != "SI" && conferma != "NO");

    if (conferma == "NO")
    {
        cout << "\nOperazione annullata." << endl;
        return;
    }

    if (!databaseManager->eliminaProdotto(articolo, modello))
    {
        cout << "Errore eliminazione prodotto su MySQL: "
            << databaseManager->ultimoErrore() << endl << endl;
        return;
    }

    cout << "\nProdotto eliminato correttamente!" << endl;
}

// --------------------------------------------------------------------------------> CLIENTI

void Menu::menuClienti()
{
    cout << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "|          GESTIONALE MAGLIFICIO           |" << endl;
    cout << "|          - DATABASE  CLIENTI -           |" << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "| 1 - Cerca Cliente                        |" << endl;
    cout << "| 2 - Crea Nuovo Cliente                   |" << endl;
    cout << "| 3 - Modifica Cliente                     |" << endl;
    cout << "| 4 - Elimina Cliente                      |" << endl;
    cout << "| 5 - Stampa DataBase Clienti completo     |" << endl;
    cout << "| 0 - Indietro                             |" << endl;
    cout << "+------------------------------------------+" << endl;
    cout << endl;
}

void Menu::sceltoClienti()
{
    int scelta = 0;

    do
    {
        menuClienti();

        scelta = leggiScelta();

        switch (scelta)
        {
        case 1:
            cercaCliente();
            break;

        case 2:
            inserisciNewCliente();
            break;

        case 3:
            modificaCliente();
            break;

        case 4:
                eliminaCliente();
            break;
                
        case 5:
                stampaClientiDaDatabase();
            break;

        case 0:
            break;

        default:
            cout << "Scelta non valida." << endl;
        }

    } while (scelta != 0);
}

void Menu::inserisciNewCliente()
{
    string nome;
    int serie;

    cout << "\n===== NUOVO CLIENTE =====\n";

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "Nome Cliente: ";

    getline(cin, nome);

    cout << "Codice Serie: ";
    cin >> serie;


    Cliente c = { nome, serie };
    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        if (!databaseManager->inserisciCliente(c))
        {
            cout << "Errore inserimento cliente su MySQL: "
                << databaseManager->ultimoErrore() << endl << endl;
            return;
        }
    }

    c.stampaCliente();
    cout << endl;
    cout << "Nuovo Cliente aggiunto correttamente!" << endl << endl;
}


void Menu::cercaCliente() {
    int tipoRicerca = 0;

    cout << "\n===== CERCA CLIENTE =====" << endl;
    cout << "1 - Cerca per nome" << endl;
    cout << "2 - Cerca per codice serie" << endl;
    tipoRicerca = leggiScelta();

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        vector<Cliente> clienti = databaseManager->caricaClienti();
        if (!databaseManager->ultimoErrore().empty())
        {
            cout << "Errore lettura CLIENTI da MySQL: "
                << databaseManager->ultimoErrore() << endl;
            return;
        }

        int trovati = 0;

        if (tipoRicerca == 1)
        {
            string nome;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Inserisci NOME cliente: ";
            getline(cin, nome);

            for (Cliente& cliente : clienti)
            {
                if (nomeClienteCorrisponde(cliente, nome))
                {
                    cout << "\nCliente trovato:\n";
                    cliente.stampaRecordCliente();
                    ++trovati;
                }
            }
        }
        else if (tipoRicerca == 2)
        {
            int serie = 0;
            cout << "Inserisci CODICE SERIE cliente: " << endl;
            serie = leggiScelta();

            for (Cliente& cliente : clienti)
            {
                if (cliente.getSerie() == serie)
                {
                    cout << "\nCliente trovato:\n";
                    cliente.stampaRecordCliente();
                    ++trovati;
                }
            }
        }
        else
        {
            cout << "Scelta non valida!" << endl;
            return;
        }

        if (trovati == 0)
            cout << "Cliente non trovato!" << endl;
        return;
    }

}

void Menu::modificaCliente() {
    string nomeRicerca;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Inserisci NOME cliente: ";
    getline(cin, nomeRicerca);

    vector<Cliente> clienti;
    Cliente* c = nullptr;

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        clienti = databaseManager->caricaClienti();
        if (!databaseManager->ultimoErrore().empty())
        {
            cout << "Errore lettura CLIENTI da MySQL: "
                << databaseManager->ultimoErrore() << endl;
            return;
        }

        c = scegliClientePerNome(clienti, nomeRicerca);
    }

    if (c == nullptr)
    {
        cout << "Cliente non trovato!" << endl;
        return;
    }

    cout << "\nCliente trovato:\n";
    c->stampaCliente();

    int scelta = 0;

    do
    {
        cout << "\n\n===== CAMPI MODIFICABILI =====" << endl;
        cout << "1 - Nome" << endl;
        cout << "2 - Serie" << endl;
        cout << "0 - Fine modifica" << endl;

        scelta = leggiScelta();

        switch (scelta)
        {
        case 1:
        {
            string nome;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Nuovo nome Cliente: ";
            getline(cin, nome);

            if (databaseManager != nullptr && databaseManager->isConnesso())
            {
                Cliente clienteAggiornato(nome, c->getSerie());
                if (!databaseManager->aggiornaCliente(c->getSerie(), clienteAggiornato))
                {
                    cout << "Errore aggiornamento cliente su MySQL: "
                        << databaseManager->ultimoErrore() << endl;
                    break;
                }
            }

            c->setNome(nome);
            break;
        }

        case 2:
        {
            int nuovaSerie;
            bool valida = true;

            while (valida)
            {
                cout << "Nuova serie Cliente: " << endl;
                nuovaSerie = leggiScelta();

                if (trovaClientePerSerie(clienti, nuovaSerie) == nullptr)
                {
                    if (databaseManager != nullptr && databaseManager->isConnesso())
                    {
                        Cliente clienteAggiornato(c->getNome(), nuovaSerie);
                        if (!databaseManager->aggiornaCliente(c->getSerie(), clienteAggiornato))
                        {
                            cout << "Errore aggiornamento cliente su MySQL: "
                                << databaseManager->ultimoErrore() << endl;
                            break;
                        }
                    }

                    c->setSerie(nuovaSerie);
                    valida = false;
                }
                else
                    cout << "Codice SERIE Cliente gia' in uso!" << endl;
            }

            break;
        }

        case 0:
            c->stampaCliente();
            cout << endl;
            cout << "\nCliente aggiornato correttamente.\n";
            break;

        default:
            cout << "Scelta non valida!" << endl;
        }

        if (scelta != 0)
        {
            cout << "\nRecord aggiornato:\n";
            c->stampaCliente();
        }

    } while (scelta != 0);
}

void Menu::eliminaCliente()
{
    int tipoRicerca = 0;
    int serie = 0;

    cout << "\n===== ELIMINA CLIENTE =====\n";
    cout << "1 - Elimina per nome" << endl;
    cout << "2 - Elimina per codice serie" << endl;
    tipoRicerca = leggiScelta();

    vector<Cliente> clienti;
    Cliente* c = nullptr;

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        clienti = databaseManager->caricaClienti();
        if (!databaseManager->ultimoErrore().empty())
        {
            cout << "Errore lettura CLIENTI da MySQL: "
                << databaseManager->ultimoErrore() << endl;
            return;
        }

        if (tipoRicerca == 1)
        {
            string nomeRicerca;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Inserisci NOME cliente da ELIMINARE: ";
            getline(cin, nomeRicerca);
            c = scegliClientePerNome(clienti, nomeRicerca);
        }
        else if (tipoRicerca == 2)
        {
            cout << "Inserisci CODICE SERIE cliente da ELIMINARE: " << endl;
            serie = leggiScelta();
            for (Cliente& cliente : clienti)
            {
                if (cliente.getSerie() == serie)
                {
                    c = &cliente;
                    break;
                }
            }
        }
        else
        {
            cout << "Scelta non valida!" << endl;
            return;
        }
    }

    if (c == nullptr)
    {
        cout << "\nCliente non trovato!" << endl;
        return;
    }

    serie = c->getSerie();

    cout << "\nCliente trovato:\n";
    c->stampaCliente();

    string conferma;

    do
    {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "\nConfermi l'eliminazione? (SI/NO): ";
        getline(cin, conferma);

        for (char& c : conferma)
        {
            c = toupper(c);
        }

    } while (conferma != "SI" && conferma != "NO");

    if (conferma == "NO")
    {
        cout << "\nOperazione annullata." << endl;
        return;
    }

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        if (!databaseManager->eliminaCliente(serie))
        {
            cout << "Errore eliminazione cliente su MySQL: "
                << databaseManager->ultimoErrore() << endl << endl;
            return;
        }
    }

    cout << "\nCliente eliminato correttamente!" << endl;
}

// --------------------------------------------------------------------------------> FILATI

void Menu::menuFilati()
{
    cout << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "|          GESTIONALE MAGLIFICIO           |" << endl;
    cout << "|          - DATABASE   FILATI -           |" << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "| 1 - Cerca Filato                         |" << endl;
    cout << "| 2 - Crea Nuovo Filato                    |" << endl;
    cout << "| 3 - Modifica Filato                      |" << endl;
    cout << "| 4 - Elimina Filato                       |" << endl;
    cout << "| 5 - Stampa DataBase Filati completo      |" << endl;
    cout << "| 0 - Indietro                             |" << endl;
    cout << "+------------------------------------------+" << endl;
    cout << endl;
}

void Menu::sceltoFilati()
{
    int scelta = 0;

    do
    {
        menuFilati();

        scelta = leggiScelta();

        switch (scelta)
        {
        case 1:
                cercaFilato();
            break;

        case 2:
                inserisciNewFilato();
            break;

        case 3:
                modificaFilato();
            break;

        case 4:
                eliminaFilato();
            break;
                
        case 5:
                stampaFilatiDaDatabase();
            break;

        case 0:
            break;

        default:
            cout << "Scelta non valida." << endl;
        }

    } while (scelta != 0);
}

void Menu::inserisciNewFilato()
{
    int codice = 0;
    string nome;
    string composizione;
    string titolo;
    string fornitore;

    cout << "\n===== NUOVO FILATO =====\n";

    cout << "Codice Filato: ";
    cin >> codice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "Nome Filato: ";
    getline(cin, nome);

    cout << "Composizione: ";
    getline(cin, composizione);

    cout << "Titolo: ";
    getline(cin, titolo);

    fornitore = scegliNomeFornitoreEsistente(databaseManager);
    if (fornitore.empty())
        return;

    Filato f = { codice, nome, composizione, titolo, fornitore };
    if (!databaseManager->inserisciFilato(f))
    {
        cout << "Errore inserimento filato su MySQL: "
            << databaseManager->ultimoErrore() << endl << endl;
        return;
    }

    f.stampaFilato();
    cout << endl;
    cout << "Nuovo Filato aggiunto correttamente!" << endl << endl;
}


void Menu::cercaFilato() {
    int tipoRicerca = 0;

    cout << "\n===== CERCA FILATO =====" << endl;
    cout << "1 - Cerca per nome" << endl;
    cout << "2 - Cerca per codice filato" << endl;
    tipoRicerca = leggiScelta();

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        vector<Filato> filati = databaseManager->caricaFilati();
        if (!databaseManager->ultimoErrore().empty())
        {
            cout << "Errore lettura FILATI da MySQL: "
                << databaseManager->ultimoErrore() << endl;
            return;
        }

        int trovati = 0;

        if (tipoRicerca == 1)
        {
            string nome;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Inserisci NOME filato: ";
            getline(cin, nome);

            for (Filato& filato : filati)
            {
                if (nomeFilatoCorrisponde(filato, nome))
                {
                    cout << "\nFilato trovato:\n";
                    filato.stampaRecordFilato();
                    ++trovati;
                }
            }
        }
        else if (tipoRicerca == 2)
        {
            int codice = 0;
            cout << "Inserisci CODICE filato: " << endl;
            codice = leggiScelta();

            for (Filato& filato : filati)
            {
                if (filato.getCodice() == codice)
                {
                    cout << "\nFilato trovato:\n";
                    filato.stampaRecordFilato();
                    ++trovati;
                }
            }
        }
        else
        {
            cout << "Scelta non valida!" << endl;
            return;
        }

        if (trovati == 0)
            cout << "Filato non trovato!" << endl;
        return;
    }

}

void Menu::modificaFilato() {
    string nomeRicerca;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Inserisci NOME filato: ";
    getline(cin, nomeRicerca);

    vector<Filato> filati;
    Filato* f = nullptr;

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        filati = databaseManager->caricaFilati();
        if (!databaseManager->ultimoErrore().empty())
        {
            cout << "Errore lettura FILATI da MySQL: "
                << databaseManager->ultimoErrore() << endl;
            return;
        }

        f = scegliFilatoPerNome(filati, nomeRicerca);
    }

    if (f == nullptr)
    {
        cout << "Filato non trovato!" << endl;
        return;
    }

    int codice = f->getCodice();

    cout << "\nFilato trovato:\n";
    f->stampaFilato();

    int scelta = 0;

    do
    {
        cout << "\n\n===== CAMPI MODIFICABILI =====" << endl;
        cout << "1 - Nome" << endl;
        cout << "2 - Composizione" << endl;
        cout << "3 - Titolo" << endl;
        cout << "4 - Fornitore" << endl;
        cout << "0 - Fine modifica" << endl;

        scelta = leggiScelta();

        switch (scelta)
        {
            case 1:
            {
                string nome;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                cout << "Nuovo NOME Filato: ";
                getline(cin, nome);

                if (databaseManager != nullptr && databaseManager->isConnesso())
                {
                    Filato filatoAggiornato(
                        codice,
                        nome,
                        f->getComposizione(),
                        f->getTitolo(),
                        f->getFornitore());
                    if (!databaseManager->aggiornaFilato(codice, filatoAggiornato))
                    {
                        cout << "Errore aggiornamento filato su MySQL: "
                            << databaseManager->ultimoErrore() << endl;
                        break;
                    }
                }

                f->setNome(nome);
                break;
            }
                
            case 2:
            {
                string composizione;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                cout << "Nuova COMPOSIZIONE Filato: ";
                getline(cin, composizione);

                if (databaseManager != nullptr && databaseManager->isConnesso())
                {
                    Filato filatoAggiornato(
                        codice,
                        f->getNome(),
                        composizione,
                        f->getTitolo(),
                        f->getFornitore());
                    if (!databaseManager->aggiornaFilato(codice, filatoAggiornato))
                    {
                        cout << "Errore aggiornamento filato su MySQL: "
                            << databaseManager->ultimoErrore() << endl;
                        break;
                    }
                }

                f->setComposizione(composizione);
                break;
                
            }

            case 3:
            {
                string titolo;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                cout << "Nuovo TITOLO Filato: ";
                getline(cin, titolo);

                if (databaseManager != nullptr && databaseManager->isConnesso())
                {
                    Filato filatoAggiornato(
                        codice,
                        f->getNome(),
                        f->getComposizione(),
                        titolo,
                        f->getFornitore());
                    if (!databaseManager->aggiornaFilato(codice, filatoAggiornato))
                    {
                        cout << "Errore aggiornamento filato su MySQL: "
                            << databaseManager->ultimoErrore() << endl;
                        break;
                    }
                }

                f->setTitolo(titolo);
                break;
            }
                
            case 4:
            {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                string fornitore = scegliNomeFornitoreEsistente(databaseManager);
                if (fornitore.empty())
                    break;

                Filato filatoAggiornato(
                    codice,
                    f->getNome(),
                    f->getComposizione(),
                    f->getTitolo(),
                    fornitore);
                if (!databaseManager->aggiornaFilato(codice, filatoAggiornato))
                {
                    cout << "Errore aggiornamento filato su MySQL: "
                        << databaseManager->ultimoErrore() << endl;
                    break;
                }

                f->setFornitore(fornitore);
                break;
            }

        case 0:
            f->stampaFilato();
                cout << endl;
            cout << "\nFilato aggiornato correttamente.\n";
            break;

        default:
            cout << "Scelta non valida!" << endl;
        }

        if (scelta != 0)
        {
            cout << "\nRecord aggiornato:\n";
            f->stampaFilato();
        }

    } while (scelta != 0);
}


void Menu::eliminaFilato()
{
    int scelta = 0;
    int codice = 0;
    vector<Filato> filati;
    Filato* f = nullptr;

    do {
        cout << "\n===== ELIMINA FILATO =====\n";
        cout << "1 - Elimina per nome" << endl;
        cout << "2 - Elimina per codice filato" << endl;
        cout << "0 - Indietro" << endl;
        scelta = leggiScelta();
	} while (scelta < 0 || scelta > 2);
	if (scelta == 0) return;

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        filati = databaseManager->caricaFilati();
        if (!databaseManager->ultimoErrore().empty())
        {
            cout << "Errore lettura FILATI da MySQL: "
                << databaseManager->ultimoErrore() << endl;
            return;
        }

        switch (scelta)
        {
            case 1:
            {
                string nomeRicerca;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Inserisci NOME filato da ELIMINARE: ";
                getline(cin, nomeRicerca);
                f = scegliFilatoPerNome(filati, nomeRicerca);
                break;
            }
            case 2:
            {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Inserisci CODICE filato da ELIMINARE: " << endl;
                codice = leggiScelta();
                for (Filato& filato : filati)
                {
                    if (filato.getCodice() == codice)
                    {
                        f = &filato;
                        break;
                    }
                }
            }
            default:
            {
                cout << "Scelta non valida!" << endl;
            }
        }

    }
    else
    {
		cout << "Errore: il database non è connesso. Non è possibile eliminare un filato." << endl;
        return;
    }

    if (f == nullptr)
    {
        cout << "\nFilato non trovato!" << endl;
        return;
    }

    codice = f->getCodice();

    cout << "\nFilato trovato:\n";
    f->stampaFilato();

    string conferma;
    do
    {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "\nConfermi l'eliminazione? (SI/NO): ";
        getline(cin, conferma);

        for (char& c : conferma)
        {
            c = toupper(c);
        }

    } while (conferma != "SI" && conferma != "NO");

    if (conferma == "NO")
    {
        cout << "\nOperazione annullata." << endl;
        return;
    }

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        if (!databaseManager->eliminaFilato(codice))
        {
            cout << "Errore eliminazione filato su MySQL: "<< databaseManager->ultimoErrore() << endl << endl;
            return;
        }
    }

    cout << "\nFilato eliminato correttamente!" << endl;

}

// --------------------------------------------------------------------------------> FORNITORI

void Menu::menuFornitori()
{
    cout << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "|          GESTIONALE MAGLIFICIO           |" << endl;
    cout << "|          - DATABASE FORNITORI -          |" << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "| 1 - Cerca Fornitore                      |" << endl;
    cout << "| 2 - Crea Nuovo Fornitore                 |" << endl;
    cout << "| 3 - Modifica Fornitore                   |" << endl;
    cout << "| 4 - Elimina Fornitore                    |" << endl;
    cout << "| 5 - Stampa DataBase Fornitori completo   |" << endl;
    cout << "| 0 - Indietro                             |" << endl;
    cout << "+------------------------------------------+" << endl;
    cout << endl;
}

void Menu::sceltoFornitori()
{
    int scelta = 0;

    do
    {
        menuFornitori();

        scelta = leggiScelta();

        switch (scelta)
        {
        case 1:
            cercaFornitore();
            break;

        case 2:
            inserisciNewFornitore();
            break;

        case 3:
            modificaFornitore();
            break;

        case 4:
            eliminaFornitore();
            break;

        case 5:
            stampaFornitoriDaDatabase();
            break;

        case 0:
            break;

        default:
            cout << "Scelta non valida." << endl;
        }

    } while (scelta != 0);
}

void Menu::inserisciNewFornitore()
{
    string nome;
    string telefono;
    string email;
    string indirizzo;
    string nazione;

    cout << "\n===== NUOVO FORNITORE =====\n";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "Nome Fornitore: ";
    getline(cin, nome);

    cout << "Telefono: ";
    getline(cin, telefono);

    cout << "Email: ";
    getline(cin, email);

    cout << "Indirizzo: ";
    getline(cin, indirizzo);

    cout << "Nazione: ";
    getline(cin, nazione);

    Fornitore fornitore(nome, telefono, email, indirizzo, nazione);
    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        if (!databaseManager->inserisciFornitore(fornitore))
        {
            cout << "Errore inserimento fornitore su MySQL: "
                << databaseManager->ultimoErrore() << endl << endl;
            return;
        }
    }

    fornitore.stampaFornitore();
    cout << endl;
    cout << "Nuovo Fornitore aggiunto correttamente!" << endl << endl;
}

void Menu::cercaFornitore()
{
	cout << endl << "===== CERCA FORNITORE =====" << endl;

    string nomeFornitore;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Inserisci NOME fornitore: ";
    getline(cin, nomeFornitore);

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        vector<Fornitore> fornitori = databaseManager->caricaFornitori();
        if (!databaseManager->ultimoErrore().empty())
        {
            cout << "Errore lettura FORNITORI da MySQL: "
                << databaseManager->ultimoErrore() << endl;
            return;
        }

        for (Fornitore& fornitore : fornitori)
        {
            if (nomeFornitoreCorrisponde(fornitore, nomeFornitore))
            {
                cout << "\nFornitore trovato:\n";
                fornitore.stampaRecordFornitore();
                return;
            }
        }
    }

    cout << "Fornitore non trovato!" << endl;
}

void Menu::modificaFornitore()
{
    string nomeFornitore;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Inserisci NOME fornitore: ";
    getline(cin, nomeFornitore);

    vector<Fornitore> fornitori;
    Fornitore* fornitore = nullptr;
    int idFornitore = 0;

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        fornitori = databaseManager->caricaFornitori();
        if (!databaseManager->ultimoErrore().empty())
        {
            cout << "Errore lettura FORNITORI da MySQL: "
                << databaseManager->ultimoErrore() << endl;
            return;
        }

        for (Fornitore& item : fornitori)
        {
            if (nomeFornitoreCorrisponde(item, nomeFornitore))
            {
                fornitore = &item;
                idFornitore = item.getId_Fornitore();
                break;
            }
        }
    }

    if (fornitore == nullptr)
    {
        cout << "Fornitore non trovato!" << endl;
        return;
    }

    cout << "\nFornitore trovato:\n";
    fornitore->stampaRecordFornitore();

    int scelta = 0;

    do
    {
        cout << "\n===== CAMPI MODIFICABILI =====" << endl;
        cout << "1 - Nome" << endl;
        cout << "2 - Telefono" << endl;
        cout << "3 - Email" << endl;
        cout << "4 - Indirizzo" << endl;
        cout << "5 - Nazione" << endl;
        cout << "0 - Fine modifica" << endl;

        scelta = leggiScelta();

        switch (scelta)
        {
        case 1:
        {
            string nome;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Nuovo nome Fornitore: ";
            getline(cin, nome);

            Fornitore aggiornato(
                idFornitore,
                nome,
                fornitore->getTelefono(),
                fornitore->getEmail(),
                fornitore->getIndirizzo(),
                fornitore->getNazione());

            if (databaseManager != nullptr && databaseManager->isConnesso()
                && !databaseManager->aggiornaFornitore(idFornitore, aggiornato))
            {
                cout << "Errore aggiornamento fornitore su MySQL: "
                    << databaseManager->ultimoErrore() << endl;
                break;
            }

            fornitore->setNome(nome);
            break;
        }

        case 2:
        {
            string telefono;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Nuovo telefono: ";
            getline(cin, telefono);

            Fornitore aggiornato(
                idFornitore,
                fornitore->getNome(),
                telefono,
                fornitore->getEmail(),
                fornitore->getIndirizzo(),
                fornitore->getNazione());

            if (databaseManager != nullptr && databaseManager->isConnesso()
                && !databaseManager->aggiornaFornitore(idFornitore, aggiornato))
            {
                cout << "Errore aggiornamento fornitore su MySQL: "
                    << databaseManager->ultimoErrore() << endl;
                break;
            }

            fornitore->setTelefono(telefono);
            break;
        }

        case 3:
        {
            string email;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Nuova email: ";
            getline(cin, email);

            Fornitore aggiornato(
                idFornitore,
                fornitore->getNome(),
                fornitore->getTelefono(),
                email,
                fornitore->getIndirizzo(),
                fornitore->getNazione());

            if (databaseManager != nullptr && databaseManager->isConnesso()
                && !databaseManager->aggiornaFornitore(idFornitore, aggiornato))
            {
                cout << "Errore aggiornamento fornitore su MySQL: "
                    << databaseManager->ultimoErrore() << endl;
                break;
            }

            fornitore->setEmail(email);
            break;
        }

        case 4:
        {
            string indirizzo;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Nuovo indirizzo: ";
            getline(cin, indirizzo);

            Fornitore aggiornato(
                idFornitore,
                fornitore->getNome(),
                fornitore->getTelefono(),
                fornitore->getEmail(),
                indirizzo,
                fornitore->getNazione());

            if (databaseManager != nullptr && databaseManager->isConnesso()
                && !databaseManager->aggiornaFornitore(idFornitore, aggiornato))
            {
                cout << "Errore aggiornamento fornitore su MySQL: "
                    << databaseManager->ultimoErrore() << endl;
                break;
            }

            fornitore->setIndirizzo(indirizzo);
            break;
        }

        case 5:
        {
            string nazione;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Nuova nazione: ";
            getline(cin, nazione);

            Fornitore aggiornato(
                idFornitore,
                fornitore->getNome(),
                fornitore->getTelefono(),
                fornitore->getEmail(),
                fornitore->getIndirizzo(),
                nazione);

            if (databaseManager != nullptr && databaseManager->isConnesso()
                && !databaseManager->aggiornaFornitore(idFornitore, aggiornato))
            {
                cout << "Errore aggiornamento fornitore su MySQL: "
                    << databaseManager->ultimoErrore() << endl;
                break;
            }

            fornitore->setNazione(nazione);
            break;
        }

        case 0:
            cout << "\nFornitore aggiornato correttamente.\n";
            break;

        default:
            cout << "Scelta non valida!" << endl;
        }

        if (scelta != 0)
        {
            cout << "\nRecord aggiornato:\n";
            fornitore->stampaRecordFornitore();
        }

    } while (scelta != 0);
}

void Menu::eliminaFornitore()
{
    string nomeFornitore;
    int idFornitore = 0;

    cout << "\n===== ELIMINA FORNITORE =====\n";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Inserisci NOME fornitore da ELIMINARE: ";
    getline(cin, nomeFornitore);

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        vector<Fornitore> fornitori = databaseManager->caricaFornitori();
        if (!databaseManager->ultimoErrore().empty())
        {
            cout << "Errore lettura FORNITORI da Database: "
                << databaseManager->ultimoErrore() << endl;
            return;
        }

        bool trovato = false;
        for (Fornitore& fornitore : fornitori)
        {
            if (nomeFornitoreCorrisponde(fornitore, nomeFornitore))
            {
                trovato = true;
                idFornitore = fornitore.getId_Fornitore();
                cout << "\nFornitore trovato:\n";
                fornitore.stampaRecordFornitore();
                break;
            }

            if (trovato) break;

        }

        if (!trovato)
        {
            cout << "Fornitore non trovato!" << endl;
            return;
        }
    }
    else
    {
		cout << "Errore connessione Database" << endl;
        return;
    }

    string conferma;

    do
    {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "\nConfermi l'eliminazione? (SI/NO): ";
        getline(cin, conferma);

        for (char& c : conferma)
        {
            c = toupper(c);
        }

    } while (conferma != "SI" && conferma != "NO");

    if (conferma == "NO")
    {
        cout << "\nOperazione annullata." << endl;
        return;
    }

    if (databaseManager != nullptr && databaseManager->isConnesso())
    {
        if (!databaseManager->eliminaFornitore(idFornitore))
        {
            cout << "Errore eliminazione fornitore su MySQL: "
                << databaseManager->ultimoErrore() << endl << endl;
            return;
        }
    }

    cout << "\nFornitore eliminato correttamente!" << endl;
}

// --------------------------------------------------------------------------------> TECNICI

void Menu::menuTecnici()
{
    cout << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "|          GESTIONALE MAGLIFICIO           |" << endl;
    cout << "|          - DATABASE  TECNICI -           |" << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "| 1 - Cerca Tecnico                        |" << endl;
    cout << "| 2 - Crea Nuovo Tecnico                   |" << endl;
    cout << "| 3 - Modifica Tecnico                     |" << endl;
    cout << "| 4 - Elimina Tecnico                      |" << endl;
    cout << "| 5 - Stampa DataBase Tecnici completo     |" << endl;
    cout << "| 0 - Indietro                             |" << endl;
    cout << "+------------------------------------------+" << endl;
    cout << endl;
}


void Menu::sceltoTecnici()
{
    int scelta=0;

    do
    {
        menuTecnici();

        scelta = leggiScelta();

        switch (scelta)
        {
        case 1:
            cercaTecnico();
            break;

        case 2:
            inserisciNewTecnico();
            break;

        case 3:
            modificaTecnico();
            break;

        case 4:
            eliminaTecnico();
            break;

        case 5:
                stampaTecniciDaDatabase();
            break;

        case 0:
            break;

        default:
            cout << "Scelta non valida." << endl;
        }

    } while (scelta != 0);
}

void Menu::inserisciNewTecnico()
{
    string cognome;
    string nome;
    Tecnico* tecnico = nullptr;
    vector<Tecnico> tecnici;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    do {
        tecnici = databaseManager->caricaTecnici();
        if (!databaseManager->ultimoErrore().empty())
        {
            cout << "Errore lettura TECNICI da MySQL: " << databaseManager->ultimoErrore() << endl;
            return;
        }

        cout << "\n===== NUOVO TECNICO =====\n";

        cout << "Cognome Tecnico: ";
        getline(cin, cognome);

        cout << "Nome Tecnico: ";
        getline(cin, nome);

        tecnico = trovaTecnico(tecnici, cognome, nome);

        if (tecnico != nullptr)
            cout << "Tecnico GIA' ESISTENTE, inserire nuovo tecnico!" << endl;

    } while (tecnico != nullptr);

    Tecnico newTecnico = { cognome, nome };
    if (!databaseManager->inserisciTecnico(newTecnico))
    {
        cout << "Errore inserimento tecnico su MySQL: "
            << databaseManager->ultimoErrore() << endl << endl;
        return;
    }

    newTecnico.stampaTecnico();
    cout << endl;
    cout << "Nuovo Tecnico aggiunto correttamente!" << endl << endl;
}

void Menu::cercaTecnico() {
    string cognome;
    string nome;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    do {
        cout << endl << "===== CERCA TECNICO =====" << endl;
        stampaTecniciDaDatabase();

        cout << "Inserisci dati di ricerca del tecnico:" << endl;
        cout << "Cognome Tecnico: ";
        getline(cin, cognome);

        cout << "Nome Tecnico: ";
        getline(cin, nome);

        if (cognome.empty() || nome.empty())
            cout << "Cognome e/o Nome non possono essere vuoti. Riprova." << endl;

    } while (cognome.empty() || nome.empty());

    vector<Tecnico> tecnici = databaseManager->caricaTecnici();
    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore lettura TECNICI da MySQL: " << databaseManager->ultimoErrore() << endl;
        return;
    }

    Tecnico* t = trovaTecnico(tecnici, cognome, nome);

    if (t == nullptr)
    {
        cout << "Tecnico non trovato!" << endl;
        return;
    }

    cout << "\nTecnico trovato:\n";
    t->stampaTecnico();
}

void Menu::modificaTecnico() {
    string cognome;
    string nome;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Inserisci COGNOME tecnico: " << endl;
    getline(cin, cognome);

    cout << "Inserisci NOME tecnico: " << endl;
    getline(cin, nome);

    vector<Tecnico> tecnici = databaseManager->caricaTecnici();
    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore lettura TECNICI da MySQL: " << databaseManager->ultimoErrore() << endl;
        return;
    }

    Tecnico* t = trovaTecnico(tecnici, cognome, nome);

    if (t == nullptr)
    {
        cout << "Tecnico non trovato!" << endl;
        return;
    }

    cout << "\nTecnico trovato:\n";
    t->stampaTecnico();

    int scelta = 0;

    do
    {
        cout << "\n===== CAMPI MODIFICABILI =====" << endl;
        cout << "1 - Cognome" << endl;
        cout << "2 - Nome" << endl;
        cout << "0 - Fine modifica" << endl;

        scelta = leggiScelta();

        switch (scelta)
        {
        case 1:
        {
            string newCognome;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Nuovo COGNOME Tecnico: ";
            getline(cin, newCognome);

            Tecnico tecnicoAggiornato(newCognome, t->getNome());
            if (!databaseManager->aggiornaTecnico(cognome, nome, tecnicoAggiornato))
            {
                cout << "Errore aggiornamento tecnico su MySQL: "
                    << databaseManager->ultimoErrore() << endl;
                break;
            }

            t->setCognome(newCognome);
            cognome = newCognome;
            break;
        }

        case 2:
        {
            string newNome;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Nuovo NOME Tecnico: ";
            getline(cin, newNome);

            Tecnico tecnicoAggiornato(t->getCognome(), newNome);
            if (!databaseManager->aggiornaTecnico(cognome, nome, tecnicoAggiornato))
            {
                cout << "Errore aggiornamento tecnico su MySQL: "
                    << databaseManager->ultimoErrore() << endl;
                break;
            }

            t->setNome(newNome);
            nome = newNome;
            break;
        }

        case 0:
            t->stampaTecnico();
            cout << endl;
            cout << "\nTecnico aggiornato correttamente.\n";
            break;

        default:
            cout << "Scelta non valida!" << endl;
        }

        if (scelta != 0)
        {
            cout << "\nRecord aggiornato:\n";
            t->stampaTecnico();
        }

    } while (scelta != 0);
}

void Menu::eliminaTecnico()
{
    string cognome;
    string nome;

    cout << "\n===== ELIMINA TECNICO =====\n";

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Inserisci COGNOME tecnico da ELIMINARE: " << endl;
    getline(cin, cognome);

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Inserisci NOME tecnico da ELIMINARE: " << endl;
    getline(cin, nome);

    vector<Tecnico> tecnici = databaseManager->caricaTecnici();
    if (!databaseManager->ultimoErrore().empty())
    {
        cout << "Errore lettura TECNICI da MySQL: " << databaseManager->ultimoErrore() << endl;
        return;
    }

    Tecnico* t = trovaTecnico(tecnici, cognome, nome);

    if (t == nullptr)
    {
        cout << "\nTecnico non trovato!" << endl;
        return;
    }

    cout << "\nTecnico trovato:\n";
    t->stampaTecnico();

    string conferma;

    do
    {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "\nConfermi l'eliminazione? (SI/NO): ";
        getline(cin, conferma);

        for (char& c : conferma)
        {
            c = toupper(c);
        }

    } while (conferma != "SI" && conferma != "NO");

    if (conferma == "NO")
    {
        cout << "\nOperazione annullata." << endl;
        return;
    }

    if (!databaseManager->eliminaTecnico(cognome, nome))
    {
        cout << "Errore eliminazione tecnico su MySQL: "
            << databaseManager->ultimoErrore() << endl << endl;
        return;
    }

    cout << "\nTecnico eliminato correttamente!" << endl;
}





