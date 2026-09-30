//
//  Tecnico.hpp
//  GestionaleMGS
//

#ifndef Tecnico_hpp
#define Tecnico_hpp

#include <stdio.h>
#include <string>

using namespace std;

class Tecnico {

private:
    int id_tecnico;
    string cognome;
    string nome;

public:

    //Costruttore Tecnico
    Tecnico(
        int id_tecnico,
        string cognome,
        string nome);

    Tecnico(
        string cognome,
        string nome);

    //Metodi Get

    int getId_Tecnico() const;

    string getCognome() const;

    string getNome() const;

    //Metodi Set

    void setCognome(string newCognome);

    void setNome(string newNome);

    //Metodi Stampa

    void stampaTecnico();

    void stampaRecordTecnico();

};

#endif /* Tecnico_hpp */
