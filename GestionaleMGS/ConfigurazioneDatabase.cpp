#include "ConfigurazioneDatabase.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

#include <Windows.h>

namespace
{
    std::string leggiVariabile(const char* nome)
    {
        char* valore = nullptr;
        std::size_t dimensione = 0;
        if (_dupenv_s(&valore, &dimensione, nome) != 0 || valore == nullptr)
            return {};

        const std::string risultato(valore);
        std::free(valore);
        return risultato;
    }

    void assegnaSePresente(std::string& destinazione, const std::string& valore)
    {
        if (!valore.empty()) destinazione = valore;
    }

    std::filesystem::path cartellaEseguibile()
    {
        std::vector<wchar_t> buffer(32768);
        const DWORD lunghezza = GetModuleFileNameW(
            nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (lunghezza == 0 || lunghezza >= buffer.size()) return {};
        return std::filesystem::path(buffer.data()).parent_path();
    }

    void leggiFileConfigurazione(
        const std::filesystem::path& percorso,
        ConfigurazioneDatabase& configurazione)
    {
        std::ifstream file(percorso);
        std::string riga;
        while (std::getline(file, riga))
        {
            const std::size_t separatore = riga.find('=');
            if (separatore == std::string::npos) continue;

            std::string chiave = riga.substr(0, separatore);
            const std::string valore = riga.substr(separatore + 1);
            if (chiave.size() >= 3 &&
                static_cast<unsigned char>(chiave[0]) == 0xEF &&
                static_cast<unsigned char>(chiave[1]) == 0xBB &&
                static_cast<unsigned char>(chiave[2]) == 0xBF)
            {
                chiave.erase(0, 3);
            }

            if (chiave == "host") configurazione.host = valore;
            else if (chiave == "utente") configurazione.utente = valore;
            else if (chiave == "password") configurazione.password = valore;
            else if (chiave == "database") configurazione.database = valore;
        }
    }
}

bool ConfigurazioneDatabase::carica(ConfigurazioneDatabase& configurazione, std::string& errore)
{
    errore.clear();
    configurazione = {};

    const std::string appData = leggiVariabile("APPDATA");
    if (!appData.empty())
    {
        leggiFileConfigurazione(
            std::filesystem::path(appData) / "GestionaleMGS" / "database.conf",
            configurazione);
    }

    if (configurazione.host.empty() || configurazione.utente.empty() ||
        configurazione.password.empty() || configurazione.database.empty())
    {
        leggiFileConfigurazione(cartellaEseguibile() / "database.conf", configurazione);
    }

    assegnaSePresente(configurazione.host, leggiVariabile("MGS_DB_HOST"));
    assegnaSePresente(configurazione.utente, leggiVariabile("MGS_DB_USER"));
    assegnaSePresente(configurazione.password, leggiVariabile("MGS_DB_PASSWORD"));
    assegnaSePresente(configurazione.database, leggiVariabile("MGS_DB_NAME"));

    if (configurazione.host.empty() || configurazione.utente.empty() ||
        configurazione.password.empty() || configurazione.database.empty())
    {
        errore = "Configurazione database incompleta. Impostare MGS_DB_HOST, MGS_DB_USER, "
            "MGS_DB_PASSWORD e MGS_DB_NAME oppure il file %APPDATA%/GestionaleMGS/database.conf.";
        return false;
    }
    return true;
}
