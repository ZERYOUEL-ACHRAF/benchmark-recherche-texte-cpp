#ifndef TEXT_GENERATOR_H
#define TEXT_GENERATOR_H

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <random>
#include <string>
#include <vector>

// genererFichier : écrit un fichier texte pseudo-aléatoire de taille cible (Mo) en insérant la phrase à des positions aléatoires
inline size_t genererFichier(const std::string& chemin, double tailleMo, unsigned graine,
                             const std::string& phrase, size_t insertions) {
    static const char* VOCABULAIRE[] = {
        "lorem", "ipsum", "dolor", "sit", "amet", "consectetur", "adipiscing", "elit",
        "sed", "eiusmod", "tempor", "incididunt", "labore", "dolore", "magna", "aliqua",
        "minim", "veniam", "quis", "nostrud", "exercitation", "ullamco", "laboris", "nisi",
        "aliquip", "ex", "ea", "commodo", "consequat", "duis", "aute", "irure", "in",
        "reprehenderit", "voluptate", "velit", "esse", "cillum", "eu", "fugiat", "nulla",
        "pariatur", "excepteur", "sint", "occaecat", "cupidatat", "non", "proident", "sunt",
        "culpa", "qui", "officia", "deserunt", "mollit", "anim", "id", "est", "laborum"};
    const size_t nbMots = sizeof(VOCABULAIRE) / sizeof(VOCABULAIRE[0]);
    std::ofstream fichier(chemin, std::ios::binary);
    if (!fichier) return 0;
    std::mt19937 gen(graine);
    std::uniform_int_distribution<size_t> distMot(0, nbMots - 1);
    const size_t cible = static_cast<size_t>(tailleMo * 1048576.0);
    const size_t motsEstimes = cible / 6 + 1;
    std::vector<size_t> positions;
    if (!phrase.empty() && insertions > 0) {
        std::uniform_int_distribution<size_t> distPos(0, motsEstimes * 8 / 10 + 1);
        positions.reserve(insertions);
        for (size_t i = 0; i < insertions; ++i) positions.push_back(distPos(gen));
        std::sort(positions.begin(), positions.end());
    }
    std::string tampon;
    tampon.reserve(1u << 20);
    size_t totalOctets = 0;
    size_t motIndex = 0;
    size_t insIdx = 0;
    while (true) {
        bool tailleAtteinte = totalOctets + tampon.size() >= cible;
        if (tailleAtteinte && insIdx >= positions.size()) break;
        if (tailleAtteinte) {
            while (insIdx < positions.size()) {
                tampon += phrase;
                tampon += ' ';
                ++insIdx;
            }
            break;
        }
        while (insIdx < positions.size() && positions[insIdx] <= motIndex) {
            tampon += phrase;
            tampon += ' ';
            ++insIdx;
        }
        tampon += VOCABULAIRE[distMot(gen)];
        ++motIndex;
        tampon += (motIndex % 12 == 0) ? '\n' : ' ';
        if (tampon.size() >= (1u << 20)) {
            fichier.write(tampon.data(), static_cast<std::streamsize>(tampon.size()));
            if (!fichier) return totalOctets;
            totalOctets += tampon.size();
            tampon.clear();
        }
    }
    if (!tampon.empty()) {
        fichier.write(tampon.data(), static_cast<std::streamsize>(tampon.size()));
        totalOctets += tampon.size();
    }
    fichier.close();
    return totalOctets;
}

#endif
