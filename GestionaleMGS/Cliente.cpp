//
//  Cliente.cpp
//  GestionaleMGS
//

#include <iostream>
#include <string>

#include "Cliente.hpp"

using namespace std;

Cliente::Cliente(int id_cliente,
    string nome,
    int serie)
    : id_cliente(id_cliente),
    nome(nome),
    serie(serie)
{
}

Cliente::Cliente(
    string nome,
    int serie)
    : nome(nome),
    serie(serie)
{
}

//Metodi Get

int Cliente::getId_Cliente() const {
    return id_cliente;
}

string Cliente::getNome() const {
    return nome;
}

int Cliente::getSerie() const {
    return serie;
}

//Metodi Set

void Cliente::setNome(string newNome) {
    nome = newNome;
}

void Cliente::setSerie(int newSerie) {
    serie = newSerie;
}

//Metodi Stampa

void Cliente::stampaCliente() {
    cout << "Nome Cliente: " << getNome() << "  |  Codice Serie: " << getSerie() << endl;
}

void Cliente::stampaRecordCliente() {
    cout << "Codice Serie: " << getSerie() << "  |  Nome: " << getNome() << endl;
}

