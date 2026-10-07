#include <cstddef>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "search_algos.h"

// formater : convertit un vecteur d'indices en texte lisible {0,1,2}
static std::string formater(const std::vector<size_t>& indices) {
    std::ostringstream flux;
    flux << "{";
    for (size_t i = 0; i < indices.size(); ++i) {
        if (i > 0) flux << ",";
        flux << indices[i];
    }
    flux << "}";
    return flux.str();
}

struct CasDeTest {
    std::string texte;
    std::string motif;
    std::string libelle;
};

// verifierCas : compare chaque algorithme à stdFindSearch sur un cas et enregistre les écarts
static void verifierCas(const std::vector<AlgoRecherche>& algos, const CasDeTest& cas,
                        std::vector<std::vector<std::string>>& echecs) {
    std::vector<size_t> attendu = stdFindSearch(cas.texte, cas.motif);
    for (size_t i = 0; i < algos.size(); ++i) {
        std::vector<size_t> obtenu = algos[i].fonction(cas.texte, cas.motif);
        if (obtenu != attendu) {
            if (echecs[i].size() < 3) {
                echecs[i].push_back(cas.libelle + " motif='" + cas.motif + "' attendu=" +
                                    formater(attendu) + " obtenu=" + formater(obtenu));
            }
        }
    }
}

int main() {
    std::vector<AlgoRecherche> algos = listerAlgorithmes();
    std::vector<std::vector<std::string>> echecs(algos.size());
    std::vector<CasDeTest> cas = {
        {"", "", "texte vide et motif vide"},
        {"", "abc", "texte vide, motif non vide"},
        {"abc", "", "motif vide"},
        {"abc", "abcdefg", "motif plus long que le texte"},
        {"hello", "hello", "motif egal au texte entier"},
        {"a", "a", "texte d'un seul caractere"},
        {"aaaa", "aa", "occurrences chevauchantes"},
        {"le chat mange du riz", "le", "motif en debut"},
        {"le chat mange du riz", "mange", "motif au milieu"},
        {"le chat mange du riz", "riz", "motif en fin"},
        {"le chat mange du riz", "zebre", "motif absent"},
        {"ligne1\nligne2\n", "ligne2", "motif apres un retour a la ligne"},
        {"ligne1\nligne2\n", "\n", "retours a la ligne successifs"},
        {"caf\xc3\xa9 na\xc3\xafve", "caf\xc3\xa9", "accent UTF-8 en debut"},
        {"caf\xc3\xa9 na\xc3\xafve", "\xc3\xafve", "accent UTF-8 en fin"},
        {"a\xc3\xa9" "a\xc3\xa9" "a\xc3\xa9", "a\xc3\xa9", "motif UTF-8 chevauchant"},
        {"ab", "b", "caractere final isolé"},
        {"x", "x", "motif sur texte d'un octet"},
    };
    for (const CasDeTest& c : cas) {
        verifierCas(algos, c, echecs);
    }

    const size_t tailles[6] = {0, 1, 2, 3, 10, 1000};
    for (size_t taille : tailles) {
        std::string texte;
        for (size_t i = 0; i < taille; ++i) texte.push_back(static_cast<char>('a' + (i % 3)));
        std::vector<std::string> motifs;
        motifs.push_back(texte);
        motifs.push_back("");
        if (taille > 0) {
            motifs.push_back(texte.substr(0, 1));
            motifs.push_back(texte.substr(taille / 2, 1));
            motifs.push_back(texte.substr(taille - 1, 1));
            motifs.push_back("zz");
        }
        for (const std::string& motif : motifs) {
            CasDeTest c{texte, motif, "taille " + std::to_string(taille)};
            verifierCas(algos, c, echecs);
        }
    }
    for (unsigned graine = 1; graine <= 1000; ++graine) {
        std::mt19937 gen(graine);
        size_t nbLettres = 2 + (graine % 3);
        std::string alphabet = std::string("abcd").substr(0, nbLettres);
        std::uniform_int_distribution<size_t> distLettre(0, alphabet.size() - 1);
        std::uniform_int_distribution<size_t> distTaille(0, 200);
        size_t n = distTaille(gen);
        std::string texte(n, 'a');
        for (size_t i = 0; i < n; ++i) texte[i] = alphabet[distLettre(gen)];
        std::vector<std::string> motifs;
        motifs.push_back(texte);
        motifs.push_back("zz");
        if (n > 0) {
            motifs.push_back(texte.substr(0, 1));
            motifs.push_back(texte.substr(n - 1, 1));
            motifs.push_back(texte.substr(n / 2, (n - n / 2) < 3 ? (n - n / 2) : 3));
            if (n >= 4) motifs.push_back(texte.substr(1, n - 2));
        }
        for (const std::string& motif : motifs) {
            CasDeTest c{texte, motif, "alea graine " + std::to_string(graine)};
            verifierCas(algos, c, echecs);
        }
    }
    bool toutOk = true;
    for (size_t i = 0; i < algos.size(); ++i) {
        if (echecs[i].empty()) {
            std::cout << "[OK] " << algos[i].nom << std::endl;
        } else {
            toutOk = false;
            std::cout << "[ECHEC] " << algos[i].nom << " :";
            for (const std::string& detail : echecs[i]) {
                std::cout << " " << detail << " ;";
            }
            std::cout << std::endl;
        }
    }
    if (toutOk) {
        std::cout << "Tous les algorithmes sont corrects." << std::endl;
        return 0;
    }
    return 1;
}


