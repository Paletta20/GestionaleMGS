//
//  Prodotto.hpp
//  GestionaleMGS
//

#ifndef Prodotto_hpp
#define Prodotto_hpp

#include <stdio.h>
#include <string>

#include "Filato.hpp"
#include "Cliente.hpp"
#include "Tecnico.hpp"
#include "Immagine.hpp"

using namespace std;

class Prodotto {

private:
    int id_prodotto;
    short stagione;
    int anno;
    int articolo;
    int modello;
    string descrizione;
    Immagine immagine;
    Filato filato;
    Cliente cliente;
    Tecnico tecnico;

    static string generaPathImmagine(int articolo, int modello);
    void aggiornaPathImmagineAutomatico();


public:

    static int estraiCodiceFilatoDaArticolo(int articolo);

    static int estraiSerieClienteDaModello(int modello);

    bool haCodiciRelazionaliCoerenti() const;

    //Costruttore Prodotto
    Prodotto(
        int id_prodotto,
        short stagione,
        int anno,
        int articolo,
        int modello,
        string descrizione,
        string pathImmagine,
        Filato filato,
        Cliente cliente,
        Tecnico tecnico);

    Prodotto(
        short stagione,
        int anno,
        int articolo,
        int modello,
        string descrizione,
        string pathImmagine,
        Filato filato,
        Cliente cliente,
        Tecnico tecnico);

    Prodotto(
        int id_prodotto,
        short stagione,
        int anno,
        int articolo,
        int modello,
        string descrizione,
        Filato filato,
        Cliente cliente,
        Tecnico tecnico);

    Prodotto(
        int id_prodotto,
        short stagione,
        int anno,
        int articolo,
        int modello,
        string descrizione,
        Immagine immagine,
        Filato filato,
        Cliente cliente,
        Tecnico tecnico);

    Prodotto(
        short stagione,
        int anno,
        int articolo,
        int modello,
        string descrizione,
        Filato filato,
        Cliente cliente,
        Tecnico tecnico);

    Prodotto(
        short stagione,
        int anno,
        int articolo,
        int modello,
        string descrizione,
        Immagine immagine,
        Filato filato,
        Cliente cliente,
        Tecnico tecnico);

    //Metodi Get

    int getId_Prodotto() const;

    short getStagione() const;

    int getAnno() const;

    int getArticolo() const;

    int getModello() const;

    string getDescrizione() const;

    string getPathImmagine() const;

    const Immagine& getImmagine() const;

    const Filato& getFilato() const;

    const Cliente& getCliente() const;

    const Tecnico& getTecnico() const;

    //Metodi Set

    void setStagione(short newStagione);

    void setAnno(int newAnno);

    void setArticolo(int newArticolo);

    void setModello(int newModello);

    void setDescrizione(string newDescrizione);

    void setPathImmagine(string newPathImmagine);

    void setImmagine(const Immagine& nuovaImmagine);

    void setFilato(const Filato& nuovoFilato);

    void setCliente(const Cliente& nuovoCliente);

    void setTecnico(const Tecnico& nuovoTecnico);

    //Metodi Stampa

    void stampaProdotto();

    void stampaRecordProdotto();
};

#endif /* Prodotto_hpp */
