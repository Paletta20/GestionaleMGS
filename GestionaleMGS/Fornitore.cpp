#include "Fornitore.hpp"

#include <iostream>

using namespace std;

Fornitore::Fornitore(
    int id_fornitore,
    string nome,
    string telefono,
    string email,
    string indirizzo,
    string nazione,
    int attivo)
    : id_fornitore(id_fornitore),
    nome(nome),
    telefono(telefono),
    email(email),
    indirizzo(indirizzo),
    nazione(nazione),
    attivo(attivo)
{
}

Fornitore::Fornitore(
    string nome,
    string telefono,
    string email,
    string indirizzo,
    string nazione,
    int attivo)
    : id_fornitore(0),
    nome(nome),
    telefono(telefono),
    email(email),
    indirizzo(indirizzo),
    nazione(nazione),
    attivo(attivo)
{
}

int Fornitore::getId_Fornitore() const
{
    return id_fornitore;
}

string Fornitore::getNome() const
{
    return nome;
}

string Fornitore::getTelefono() const
{
    return telefono;
}

string Fornitore::getEmail() const
{
    return email;
}

string Fornitore::getIndirizzo() const
{
    return indirizzo;
}

string Fornitore::getNazione() const
{
    return nazione;
}

int Fornitore::getAttivo() const
{
    return attivo;
}

void Fornitore::setNome(string newNome)
{
    nome = newNome;
}

void Fornitore::setTelefono(string newTelefono)
{
    telefono = newTelefono;
}

void Fornitore::setEmail(string newEmail)
{
    email = newEmail;
}

void Fornitore::setIndirizzo(string newIndirizzo)
{
    indirizzo = newIndirizzo;
}

void Fornitore::setNazione(string newNazione)
{
    nazione = newNazione;
}

void Fornitore::setAttivo(int newAttivo)
{
    attivo = newAttivo;
}

void Fornitore::stampaFornitore()
{
    cout << "FORNITORE =====> ";
    cout << "Nome: " << nome << endl;
}

void Fornitore::stampaRecordFornitore()
{
    cout << "Nome: " << nome
        << "  |  Telefono: " << telefono
        << "  |  Email: " << email
        << "  |  Indirizzo: " << indirizzo
        << "  |  Nazione: " << nazione
        << endl;
}
