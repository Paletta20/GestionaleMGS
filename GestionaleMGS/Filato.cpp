//
//  Filato.cpp
//  GestionaleMGS
//

#include "Filato.hpp"

#include <iostream>
#include <string>

using namespace std;

Filato::Filato(
    int codice,
    string nome,
    string composizione,
    string titolo,
    string fornitore)
    : codice(codice),
    nome(nome),
    composizione(composizione),
    titolo(titolo),
    fornitore(fornitore)
{
}

Filato::Filato(
    string nome,
    string composizione,
    string titolo,
    string fornitore)
    : nome(nome),
    composizione(composizione),
    titolo(titolo),
    fornitore(fornitore)
{
}

//Metodi Get

int Filato::getCodice() const {
    return codice;
}

string Filato::getNome() const {
    return nome;
}

string Filato::getComposizione() const {
    return composizione;
}

string Filato::getTitolo() const {
    return titolo;
}

string Filato::getFornitore() const {
    return fornitore;
}

//Metodi Set

void Filato::setNome(string newNome) {
    nome = newNome;
}

void Filato::setComposizione(string newComposizione) {
    composizione = newComposizione;
}

void Filato::setTitolo(string newTitolo) {
    titolo = newTitolo;
}

void Filato::setFornitore(string newFornitore) {
    fornitore = newFornitore;
}

//Metodi Stampa

void Filato::stampaFilato() {
    cout << "Nome Filato: " << getNome() << "  |  Codice Filato: " << getCodice() << endl;
}

void Filato::stampaRecordFilato() {
    cout << "Nome: " << getNome() << "  |  Codice: " << getCodice() << "  |  Composizione: " << getComposizione()
         << "  |  Titolo: " << getTitolo() << "  |  Fornitore: " << getFornitore() << endl;
}
