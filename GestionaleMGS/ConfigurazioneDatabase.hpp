#ifndef ConfigurazioneDatabase_hpp
#define ConfigurazioneDatabase_hpp

#include <string>

struct ConfigurazioneDatabase
{
    std::string host;
    std::string utente;
    std::string password;
    std::string database;

    static bool carica(ConfigurazioneDatabase& configurazione, std::string& errore);
};

#endif
