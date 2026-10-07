#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "benchmark.h"
#include "input_utils.h"
#include "os_metrics.h"
#include "search_algos.h"
#include "text_generator.h"

// afficherMenu : affiche le menu principal
static void afficherMenu() {
    std::cout << std::endl;
    std::cout << "=== Benchmark de recherche de phrases ===" << std::endl;
    std::cout << "1) Saisir le chemin du fichier et le charger" << std::endl;
    std::cout << "2) Saisir la phrase a rechercher (option casse)" << std::endl;
    std::cout << "3) Saisir le nombre de repetitions R" << std::endl;
    std::cout << "4) Lancer le benchmark complet (tous les algos)" << std::endl;
    std::cout << "5) Benchmark d'un seul algorithme" << std::endl;
    std::cout << "6) Generer un fichier de test massif" << std::endl;
    std::cout << "7) Exporter les derniers resultats en CSV" << std::endl;
    std::cout << "8) Afficher les infos machine" << std::endl;
    std::cout << "0) Quitter" << std::endl;
}

// enMinusculesAscii : convertit une chaîne en minuscules ASCII indépendamment de la locale
static std::string enMinusculesAscii(const std::string& source) {
    std::string copie = source;
    std::transform(copie.begin(), copie.end(), copie.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return copie;
}

int main() {
    std::string texteSource;
    std::string texte;
    std::string phrase;
    bool casseSensible = true;
    bool fichierCharge = false;
    int repetitions = 10;
    std::vector<ResultatBenchmark> derniersResultats;

    while (true) {
        afficherMenu();
        std::cout << "Texte charge : " << (fichierCharge ? "oui" : "non")
                  << " | Phrase : " << (phrase.empty() ? "(aucune)" : "definie")
                  << " | R = " << repetitions << std::endl;
        int choix = lireInt("Choix : ", 0, 8);

        if (choix == 0) break;

        if (choix == 1) {
            std::string chemin = lireLigne("Chemin du fichier : ");
            std::ifstream fichier(chemin, std::ios::binary | std::ios::ate);
            if (!fichier) {
                std::cout << "Erreur : fichier introuvable (" << chemin << ")" << std::endl;
                continue;
            }
            std::streamoff taille = fichier.tellg();
            if (taille <= 0) {
                std::cout << "Erreur : fichier vide ou illisible." << std::endl;
                continue;
            }
            fichier.seekg(0, std::ios::beg);
            std::string contenu;
            contenu.resize(static_cast<size_t>(taille));
            fichier.read(&contenu[0], taille);
            if (!fichier) {
                std::cout << "Erreur : lecture du fichier impossible." << std::endl;
                continue;
            }
            texteSource = std::move(contenu);
            fichierCharge = true;
            texte = casseSensible ? texteSource : enMinusculesAscii(texteSource);
            std::cout << "Fichier charge : " << std::fixed << std::setprecision(2)
                      << (static_cast<double>(texteSource.size()) / 1048576.0) << " Mo ("
                      << texteSource.size() << " octets)" << std::endl;
            std::cout << "RAM du processus apres chargement : "
                      << (static_cast<double>(getCurrentRSS()) / 1048576.0) << " Mo" << std::endl;
            continue;
        }

        if (choix == 2) {
            phrase = lireLigne("Phrase a rechercher : ");
            casseSensible = lireOuiNon("Sensible a la casse ? (o/n) : ");
            if (!casseSensible) {
                texte = enMinusculesAscii(texteSource);
                phrase = enMinusculesAscii(phrase);
            } else {
                texte = texteSource;
            }
            std::cout << "Phrase definie (" << phrase.size() << " octets), casse : "
                      << (casseSensible ? "sensible" : "insensible") << std::endl;
            continue;
        }

        if (choix == 3) {
            repetitions = lireInt("Nombre de repetitions R (10 par defaut) : ", 1, 100000);
            std::cout << "R = " << repetitions << std::endl;
            continue;
        }

        if (choix == 4) {
            if (!fichierCharge || phrase.empty()) {
                std::cout << "Chargez le fichier (1) et saisissez la phrase (2) d'abord"
                          << std::endl;
                continue;
            }
            derniersResultats = lancerBenchmarkComplet(texte, phrase, repetitions);
            afficherResultats(derniersResultats, texte.size());
            continue;
        }

        if (choix == 5) {
            if (!fichierCharge || phrase.empty()) {
                std::cout << "Chargez le fichier (1) et saisissez la phrase (2) d'abord"
                          << std::endl;
                continue;
            }
            std::vector<AlgoRecherche> algos = listerAlgorithmes();
            for (size_t i = 0; i < algos.size(); ++i) {
                std::cout << (i + 1) << ") " << algos[i].nom << std::endl;
            }
            int selection = lireInt("Choix de l'algorithme : ", 1, static_cast<int>(algos.size()));
            derniersResultats =
                lancerBenchmarkUnAlgo(algos[static_cast<size_t>(selection) - 1], texte, phrase,
                                      repetitions);
            afficherResultats(derniersResultats, texte.size());
            continue;
        }

        if (choix == 6) {
            std::string chemin = lireLigne("Nom du fichier de test : ");
            int tailleMo = lireInt("Taille du fichier (Mo) : ", 1, 8192);
            unsigned graine = static_cast<unsigned>(
                lireSizeT("Graine : ", 0ULL, 4294967295ULL));
            std::string aInserer = lireLigne("Phrase a inserer : ");
            size_t insertions = lireSizeT("Nombre d'insertions : ", 0ULL, 10000000ULL);
            size_t ecrits =
                genererFichier(chemin, static_cast<double>(tailleMo), graine, aInserer, insertions);
            if (ecrits == 0) {
                std::cout << "Erreur : impossible de generer le fichier " << chemin << std::endl;
                continue;
            }
            std::cout << "Fichier genere : " << std::fixed << std::setprecision(2)
                      << (static_cast<double>(ecrits) / 1048576.0) << " Mo (" << ecrits
                      << " octets)" << std::endl;
            continue;
        }

        if (choix == 7) {
            if (derniersResultats.empty()) {
                std::cout << "Aucun resultat : lancez d'abord un benchmark (4 ou 5)" << std::endl;
                continue;
            }
            std::string chemin = lireLigne("Nom du fichier CSV : ");
            exporterCSV(chemin, derniersResultats, getMachineInfo());
            continue;
        }

        if (choix == 8) {
            std::cout << getMachineInfo() << std::endl;
            continue;
        }
    }

    std::cout << "Au revoir." << std::endl;
    return 0;
}
