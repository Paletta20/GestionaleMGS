//
//  Cliente.hpp
//  GestionaleMGS
//

#ifndef Cliente_hpp
#define Cliente_hpp

#include <stdio.h>
#include <string>

using namespace std;

class Cliente {

private:
    int id_cliente;
    string nome;
    int serie;

public:

    //Costruttore Cliente
    Cliente(
        int id_cliente,
        string nome,
        int serie);

    Cliente(
        string nome,
        int serie);

    //Metodi Get

    int getId_Cliente() const;

    string getNome() const;

    int getSerie() const;

    //Metodi Set

    void setNome(string newNome);

    void setSerie(int newSerie);

    //Metodi Stampa

    void stampaCliente();

    void stampaRecordCliente();
};

#endif /* Cliente_hpp */
