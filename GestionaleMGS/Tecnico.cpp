//
//  Tecnico.cpp
//  GestionaleMGS
//

#include "Tecnico.hpp"

#include <iostream>
#include <string>

using namespace std;

Tecnico::Tecnico(
    int id_tecnico,
    string cognome,
    string nome)
    : id_tecnico(id_tecnico),
    cognome(cognome),
    nome(nome)
{
}

Tecnico::Tecnico(
    string cognome,
    string nome)
    : cognome(cognome),
    nome(nome)
{
}

//Metodi Get

int Tecnico::getId_Tecnico() const {
    return id_tecnico;
}

string Tecnico::getCognome() const {
    return cognome;
}

string Tecnico::getNome() const {
    return nome;
}

//Metodi Set

void Tecnico::setCognome(string newCognome) {
    cognome = newCognome;
}

void Tecnico::setNome(string newNome) {
    nome = newNome;
}

//Metodi Stampa

void Tecnico::stampaTecnico() {
    cout << "Cognome: " << getCognome() << "  |  Nome: " << getNome() << endl;
}

void Tecnico::stampaRecordTecnico() {
    cout << "Cognome: " << getCognome() << "  |  Nome: " << getNome() << endl;
}

