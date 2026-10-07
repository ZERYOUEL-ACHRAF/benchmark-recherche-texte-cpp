#ifndef OS_METRICS_H
#define OS_METRICS_H

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <fstream>
#include <sstream>
#endif

// getCurrentRSS : retourne la RAM résidente actuelle du processus en octets
inline size_t getCurrentRSS() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<size_t>(pmc.WorkingSetSize);
    }
    return 0;
#elif defined(__linux__)
    std::ifstream fichier("/proc/self/status");
    std::string ligne;
    while (std::getline(fichier, ligne)) {
        if (ligne.rfind("VmRSS:", 0) == 0) {
            std::istringstream flux(ligne.substr(6));
            size_t ko = 0;
            flux >> ko;
            return ko * 1024ULL;
        }
    }
    return 0;
#else
    std::fputs("non supporte\n", stderr);
    return 0;
#endif
}

// getPeakRSS : retourne le pic de RAM résidente du processus en octets
inline size_t getPeakRSS() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<size_t>(pmc.PeakWorkingSetSize);
    }
    return 0;
#elif defined(__linux__)
    std::ifstream fichier("/proc/self/status");
    std::string ligne;
    while (std::getline(fichier, ligne)) {
        if (ligne.rfind("VmHWM:", 0) == 0) {
            std::istringstream flux(ligne.substr(6));
            size_t ko = 0;
            flux >> ko;
            return ko * 1024ULL;
        }
    }
    return 0;
#else
    std::fputs("non supporte\n", stderr);
    return 0;
#endif
}

// resetPeakRSS : réinitialise le pic de RAM résidente (échec ignoré silencieusement)
inline void resetPeakRSS() {
#ifdef _WIN32
    EmptyWorkingSet(GetCurrentProcess());
#elif defined(__linux__)
    std::ofstream fichier("/proc/self/clear_refs");
    if (fichier) fichier << "5";
#else
    std::fputs("non supporte\n", stderr);
#endif
}

// nomCPU : retourne le nom du processeur (inconnu si l'information n'est pas accessible)
inline std::string nomCPU() {
#ifdef __linux__
    std::ifstream fichier("/proc/cpuinfo");
    std::string ligne;
    while (std::getline(fichier, ligne)) {
        if (ligne.rfind("model name", 0) == 0) {
            size_t deuxPoints = ligne.find(':');
            if (deuxPoints != std::string::npos) {
                std::string nom = ligne.substr(deuxPoints + 1);
                while (!nom.empty() && nom[0] == ' ') nom.erase(0, 1);
                return nom;
            }
        }
    }
    return "inconnu";
#elif defined(_WIN32)
    const char* id = std::getenv("PROCESSOR_IDENTIFIER");
    if (id != nullptr && id[0] != '\0') return std::string(id);
    return "inconnu";
#else
    return "inconnu";
#endif
}

// ramTotale : retourne la RAM totale de la machine en Mo (inconnu si non accessible)
inline std::string ramTotale() {
#ifdef _WIN32
    MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
    if (GlobalMemoryStatusEx(&stat)) {
        return std::to_string(stat.ullTotalPhys / 1048576ULL) + " Mo";
    }
    return "inconnu";
#elif defined(__linux__)
    std::ifstream fichier("/proc/meminfo");
    std::string cle;
    unsigned long long ko = 0;
    std::string unite;
    while (fichier >> cle >> ko >> unite) {
        if (cle == "MemTotal:") return std::to_string(ko / 1024ULL) + " Mo";
    }
    return "inconnu";
#else
    return "inconnu";
#endif
}

// getMachineInfo : retourne les informations machine (compilateur, optimisation, coeurs, CPU, RAM, OS)
inline std::string getMachineInfo() {
    std::string info;
    info += "Compilateur : ";
#ifdef __VERSION__
    info += __VERSION__;
#else
    info += "inconnu";
#endif
    info += "\nOptimisation (-O) : ";
#ifdef __OPTIMIZE__
    info += "oui";
#else
    info += "non";
#endif
    info += "\nCoeurs logiques : " + std::to_string(std::thread::hardware_concurrency());
    info += "\nProcesseur : " + nomCPU();
    info += "\nRAM totale : " + ramTotale();
#if defined(_WIN32)
    info += "\nOS : Windows";
#elif defined(__linux__)
    info += "\nOS : Linux";
#else
    info += "\nOS : non supporte";
#endif
    info += "\nTaille de int : " + std::to_string(sizeof(int)) + " octets";
    return info;
}

#endif
