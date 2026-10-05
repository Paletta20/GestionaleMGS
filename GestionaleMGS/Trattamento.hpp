#ifndef Trattamento_hpp
#define Trattamento_hpp

#include <string>

class Trattamento
{
private:
    int id_trattamento;
    std::string trattamento;

public:
    Trattamento();
    Trattamento(int idTrattamento, std::string descrizioneTrattamento);
    explicit Trattamento(std::string descrizioneTrattamento);

    int getId_Trattamento() const;
    std::string getTrattamento() const;
    bool isAssegnato() const;

    void setTrattamento(std::string nuovoTrattamento);

    void stampaTrattamento() const;
    void stampaRecordTrattamento() const;
};

#endif
