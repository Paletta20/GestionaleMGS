#include "Trattamento.hpp"

#include <iostream>
#include <utility>

Trattamento::Trattamento()
    : id_trattamento(0)
{
}

Trattamento::Trattamento(int idTrattamento, std::string descrizioneTrattamento)
    : id_trattamento(idTrattamento), trattamento(std::move(descrizioneTrattamento))
{
}

Trattamento::Trattamento(std::string descrizioneTrattamento)
    : Trattamento(0, std::move(descrizioneTrattamento))
{
}

int Trattamento::getId_Trattamento() const
{
    return id_trattamento;
}

std::string Trattamento::getTrattamento() const
{
    return trattamento;
}

bool Trattamento::isAssegnato() const
{
    return id_trattamento > 0;
}

void Trattamento::setTrattamento(std::string nuovoTrattamento)
{
    trattamento = std::move(nuovoTrattamento);
}

void Trattamento::stampaTrattamento() const
{
    std::cout << (trattamento.empty() ? "Nessun trattamento" : trattamento);
}

void Trattamento::stampaRecordTrattamento() const
{
    std::cout << "Trattamento: "
        << (trattamento.empty() ? "Nessun trattamento" : trattamento);
}
