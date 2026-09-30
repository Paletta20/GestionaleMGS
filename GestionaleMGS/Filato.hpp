//
//  Filato.hpp
//  GestionaleMGS
//

#ifndef Filato_hpp
#define Filato_hpp

#include <stdio.h>
#include <string>

using namespace std;

class Filato {

private:
    int codice;
    string nome;
    string composizione;
    string titolo;
    string fornitore;

public:

    //Costruttore Filato
    Filato(
        int codice,
        string nome,
        string composizione,
        string titolo,
        string fornitore);

    Filato(
        string nome,
        string composizione,
        string titolo,
        string fornitore);

    //Metodi Get

    int getCodice() const;

    string getNome() const;

    string getComposizione() const;

    string getTitolo() const;

    string getFornitore() const;

    //Metodi Set

    void setNome(string newNome);

    void setComposizione(string newComposizione);

    void setTitolo(string newTitolo);

    void setFornitore(string newFornitore);

    //Metodi Stampa

    void stampaFilato();

    void stampaRecordFilato();

};
#endif
