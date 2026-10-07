#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "search_algos.h"

// genererTexteSimple : produit un texte de taille cible en insérant la phrase aux positions données
static std::string genererTexteSimple(size_t tailleCible, unsigned graine,
                                      const std::string& phrase, size_t insertions) {
    static const char* MOTS[] = {"lorem", "ipsum", "dolor", "sit", "amet", "consectetur",
                                 "adipiscing", "elit", "sed", "eiusmod", "tempor", "incididunt",
                                 "labore", "dolore", "magna", "aliqua", "minim", "veniam",
                                 "quis", "nostrud", "exercitation", "ullamco", "laboris",
                                 "nisi", "aliquip", "ex", "ea", "commodo", "consequat"};
    const size_t nbMots = sizeof(MOTS) / sizeof(MOTS[0]);
    std::mt19937 gen(graine);
    std::uniform_int_distribution<size_t> distMot(0, nbMots - 1);
    std::uniform_int_distribution<size_t> distPos(0, tailleCible * 8 / 10);
    std::vector<size_t> positions;
    for (size_t i = 0; i < insertions; ++i) positions.push_back(distPos(gen));
    std::sort(positions.begin(), positions.end());
    std::string texte;
    texte.reserve(tailleCible + insertions * (phrase.size() + 1));
    size_t insIdx = 0;
    while (texte.size() < tailleCible) {
        while (insIdx < positions.size() && positions[insIdx] <= texte.size()) {
            texte += phrase;
            texte += ' ';
            ++insIdx;
        }
        texte += MOTS[distMot(gen)];
        texte += ' ';
    }
    return texte;
}

// medianeMs : répète l'appel reps fois (1 warm-up non mesuré) et retourne la médiane en ms
static double medianeMs(FonctionRecherche algo, const std::string& texte,
                        const std::string& motif, int reps) {
    algo(texte, motif);
    std::vector<double> ms;
    ms.reserve(static_cast<size_t>(reps));
    for (int r = 0; r < reps; ++r) {
        auto debut = std::chrono::steady_clock::now();
        std::vector<size_t> res = algo(texte, motif);
        auto fin = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> duree = fin - debut;
        ms.push_back(duree.count());
        if (res.empty()) res.push_back(0);
    }
    std::sort(ms.begin(), ms.end());
    return ms[ms.size() / 2];
}

int main() {
    const std::string phrase = "the quick brown fox";
    const size_t taille = 2u * 1048576u;
    const size_t insertions = 50;
    const int reps = 5;

    std::string texte = genererTexteSimple(taille, 42, phrase, insertions);
    std::vector<AlgoRecherche> algos = listerAlgorithmes();
    std::vector<size_t> attendu = stdFindSearch(texte, phrase);

    std::cout << "Texte : " << (static_cast<double>(texte.size()) / 1048576.0)
              << " Mo, occurrences attendues : " << attendu.size() << ", R = " << reps << "\n\n";
    std::cout << std::left << std::setw(20) << "Algorithme"
              << std::right << std::setw(14) << "Occurrences"
              << std::setw(14) << "Medianes (ms)"
              << std::setw(14) << "Ratio" << "  Etat\n";
    std::cout << std::string(64, '-') << "\n";

    bool toutOk = true;
    double refMs = 0.0;
    for (const AlgoRecherche& algo : algos) {
        std::vector<size_t> obtenu = algo.fonction(texte, phrase);
        bool valide = (obtenu == attendu);
        double ms = medianeMs(algo.fonction, texte, phrase, reps);
        if (algo.nom == "stdFindSearch") refMs = ms;
        if (!valide) toutOk = false;
        double ratio = refMs > 0.0 ? ms / refMs : 1.0;
        std::cout << std::left << std::setw(20) << algo.nom
                  << std::right << std::setw(14) << obtenu.size()
                  << std::setw(14) << std::fixed << std::setprecision(3) << ms
                  << std::setw(14) << std::setprecision(2) << ratio << "x"
                  << "  " << (valide ? "[OK]" : "[ECHEC]") << "\n";
    }
    std::cout << "\n" << (toutOk ? "Tous les algorithmes sont corrects."
                                 : "ERREUR : au moins un algorithme diverge.") << std::endl;
    return toutOk ? 0 : 1;
}
