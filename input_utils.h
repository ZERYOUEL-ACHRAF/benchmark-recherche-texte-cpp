#ifndef INPUT_UTILS_H
#define INPUT_UTILS_H

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

// purgeLigne : consomme le reste de la ligne courante pour éviter les lectures parasites
inline void purgeLigne() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// lireLigne : lit une ligne entière saisie par l'utilisateur en réitérant après une erreur
inline std::string lireLigne(const std::string& invite) {
    while (true) {
        std::cout << invite;
        std::cout.flush();
        std::string ligne;
        if (std::getline(std::cin, ligne)) return ligne;
        if (std::cin.eof()) {
            std::cout << std::endl << "Entree terminee, sortie du programme." << std::endl;
            std::exit(0);
        }
        std::cin.clear();
        purgeLigne();
        std::cout << "Entree invalide, reessayez." << std::endl;
    }
}

// lireInt : lit un entier borné [min, max] en réitérant après une erreur de saisie
inline int lireInt(const std::string& invite, int min, int max) {
    while (true) {
        std::cout << invite;
        std::cout.flush();
        int valeur;
        if (std::cin >> valeur) {
            purgeLigne();
            if (valeur < min || valeur > max) {
                std::cout << "Valeur comprise entre " << min << " et " << max
                          << " attendue, reessayez." << std::endl;
                continue;
            }
            return valeur;
        }
        if (std::cin.eof()) {
            std::cout << std::endl << "Entree terminee, sortie du programme." << std::endl;
            std::exit(0);
        }
        std::cin.clear();
        purgeLigne();
        std::cout << "Entree invalide, reessayez." << std::endl;
    }
}

// lireSizeT : lit un entier non signé borné [min, max] en réitérant après une erreur de saisie
inline size_t lireSizeT(const std::string& invite, size_t min, size_t max) {
    while (true) {
        std::cout << invite;
        std::cout.flush();
        unsigned long long valeur;
        if (std::cin >> valeur) {
            purgeLigne();
            if (valeur < min || valeur > max) {
                std::cout << "Valeur comprise entre " << min << " et " << max
                          << " attendue, reessayez." << std::endl;
                continue;
            }
            return static_cast<size_t>(valeur);
        }
        if (std::cin.eof()) {
            std::cout << std::endl << "Entree terminee, sortie du programme." << std::endl;
            std::exit(0);
        }
        std::cin.clear();
        purgeLigne();
        std::cout << "Entree invalide, reessayez." << std::endl;
    }
}

// lireOuiNon : lit une réponse o/n et retourne true pour oui
inline bool lireOuiNon(const std::string& invite) {
    while (true) {
        std::string reponse = lireLigne(invite);
        if (!reponse.empty()) {
            char c = reponse[0];
            if (c == 'o' || c == 'O') return true;
            if (c == 'n' || c == 'N') return false;
        }
        std::cout << "Repondez par o (oui) ou n (non)." << std::endl;
    }
}

#endif
