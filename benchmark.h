#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "memory_tracker.h"
#include "os_metrics.h"
#include "search_algos.h"

static volatile size_t sink = 0;

struct StatsBenchmark {
    double minMs;
    double maxMs;
    double moyenneMs;
    double medianeMs;
    double ecartTypeMs;
};

struct MesureMemoire {
    size_t occurrences;
    double ramOsDeltaMo;
    double alloueMo;
};

struct ResultatBenchmark {
    std::string nomAlgo;
    size_t occurrences;
    StatsBenchmark temps;
    double ramOsDeltaMo;
    double alloueMo;
    double ratio;
    bool valide;
};

// mediane : calcule la médiane d'un échantillon de durées en millisecondes
inline double mediane(std::vector<double> echantillon) {
    if (echantillon.empty()) return 0.0;
    std::sort(echantillon.begin(), echantillon.end());
    size_t n = echantillon.size();
    if (n % 2 == 1) return echantillon[n / 2];
    return (echantillon[n / 2 - 1] + echantillon[n / 2]) / 2.0;
}

// calculerStats : calcule min, max, moyenne, médiane et écart-type d'un échantillon
inline StatsBenchmark calculerStats(const std::vector<double>& echantillon) {
    StatsBenchmark s{0.0, 0.0, 0.0, 0.0, 0.0};
    if (echantillon.empty()) return s;
    s.minMs = echantillon[0];
    s.maxMs = echantillon[0];
    double somme = 0.0;
    for (double v : echantillon) {
        somme += v;
        if (v < s.minMs) s.minMs = v;
        if (v > s.maxMs) s.maxMs = v;
    }
    s.moyenneMs = somme / static_cast<double>(echantillon.size());
    s.medianeMs = mediane(echantillon);
    double variance = 0.0;
    for (double v : echantillon) {
        double ecart = v - s.moyenneMs;
        variance += ecart * ecart;
    }
    s.ecartTypeMs = std::sqrt(variance / static_cast<double>(echantillon.size()));
    return s;
}

// mesurerTemps : exécute 1 warm-up puis R répétitions chronométrées séparément de l'appel seul
inline StatsBenchmark mesurerTemps(FonctionRecherche algo, const std::string& texte,
                                   const std::string& motif, int repetitions) {
    std::vector<double> r;
    r.reserve(static_cast<size_t>(repetitions));
    std::vector<size_t> premier = algo(texte, motif);
    sink = premier.size() + (premier.empty() ? 0 : premier.front());
    for (int i = 0; i < repetitions; ++i) {
        auto debut = std::chrono::steady_clock::now();
        std::vector<size_t> courant = algo(texte, motif);
        auto fin = std::chrono::steady_clock::now();
        sink = courant.size() + (courant.empty() ? 0 : courant.front());
        r.push_back(std::chrono::duration<double, std::milli>(fin - debut).count());
    }
    return calculerStats(r);
}

// mesurerMemoire : exécute une passe mémoire unique après reset du tracker et du pic RSS
inline MesureMemoire mesurerMemoire(FonctionRecherche algo, const std::string& texte,
                                    const std::string& motif) {
    memory_tracker::resetTracker();
    resetPeakRSS();
    size_t rssAvant = getCurrentRSS();
    std::vector<size_t> resultat = algo(texte, motif);
    size_t picRss = getPeakRSS();
    sink = resultat.size() + (resultat.empty() ? 0 : resultat.front());
    double delta = 0.0;
    if (picRss > rssAvant) {
        delta = static_cast<double>(picRss - rssAvant) / 1048576.0;
    }
    return MesureMemoire{resultat.size(), delta,
                         static_cast<double>(memory_tracker::trackerPeakBytes()) / 1048576.0};
}

// mesurerAlgo : mesure temps, mémoire et justesse d'un algorithme (justesse hors mesure)
inline ResultatBenchmark mesurerAlgo(const AlgoRecherche& algo, const std::string& texte,
                                     const std::string& motif, int repetitions,
                                     const std::vector<size_t>& reference) {
    ResultatBenchmark res;
    res.nomAlgo = algo.nom;
    res.temps = mesurerTemps(algo.fonction, texte, motif, repetitions);
    MesureMemoire memoire = mesurerMemoire(algo.fonction, texte, motif);
    res.occurrences = memoire.occurrences;
    res.ramOsDeltaMo = memoire.ramOsDeltaMo;
    res.alloueMo = memoire.alloueMo;
    std::vector<size_t> controle = algo.fonction(texte, motif);
    res.valide = (controle == reference);
    if (!res.valide) {
        std::cout << "[ERREUR] " << algo.nom << " diverge" << std::endl;
    }
    res.ratio = -1.0;
    return res;
}

// affecterRatios : remplit le ratio vs std::find (médiane) pour chaque résultat d'un même scénario
inline void affecterRatios(std::vector<ResultatBenchmark>& resultats) {
    double reference = -1.0;
    for (const ResultatBenchmark& r : resultats) {
        if (r.nomAlgo == "stdFindSearch") reference = r.temps.medianeMs;
    }
    for (ResultatBenchmark& r : resultats) {
        if (reference > 0.0) {
            r.ratio = r.temps.medianeMs / reference;
        } else {
            r.ratio = -1.0;
        }
    }
}

// lancerBenchmarkComplet : mesure les 8 algorithmes sur le texte et le motif donnés
inline std::vector<ResultatBenchmark> lancerBenchmarkComplet(const std::string& texte,
                                                             const std::string& motif,
                                                             int repetitions) {
    std::vector<AlgoRecherche> algos = listerAlgorithmes();
    std::vector<size_t> reference = stdFindSearch(texte, motif);
    std::vector<ResultatBenchmark> resultats;
    resultats.reserve(algos.size());
    for (const AlgoRecherche& algo : algos) {
        resultats.push_back(mesurerAlgo(algo, texte, motif, repetitions, reference));
    }
    affecterRatios(resultats);
    return resultats;
}

// lancerBenchmarkUnAlgo : mesure un seul algorithme avec std::find comme référence du ratio
inline std::vector<ResultatBenchmark> lancerBenchmarkUnAlgo(const AlgoRecherche& choix,
                                                            const std::string& texte,
                                                            const std::string& motif,
                                                            int repetitions) {
    std::vector<size_t> reference = stdFindSearch(texte, motif);
    std::vector<ResultatBenchmark> resultats;
    if (choix.fonction == &stdFindSearch) {
        resultats.push_back(mesurerAlgo(choix, texte, motif, repetitions, reference));
    } else {
        resultats.push_back(mesurerAlgo({"stdFindSearch", &stdFindSearch}, texte, motif,
                                        repetitions, reference));
        resultats.push_back(mesurerAlgo(choix, texte, motif, repetitions, reference));
    }
    affecterRatios(resultats);
    return resultats;
}

// formaterTemps : formate une durée en ms, ou en µs si la durée est inférieure à 0,01 ms
inline std::string formaterTemps(double millisecondes) {
    std::ostringstream flux;
    if (millisecondes < 0.01) {
        flux << std::fixed << std::setprecision(2) << (millisecondes * 1000.0) << " µs";
    } else {
        flux << std::fixed << std::setprecision(4) << millisecondes << " ms";
    }
    return flux.str();
}

// afficherResultats : affiche le tableau aligné puis le plus rapide, le plus lent et le débit
inline void afficherResultats(const std::vector<ResultatBenchmark>& resultats, size_t tailleTexte) {
    if (resultats.empty()) {
        std::cout << "Aucun resultat a afficher." << std::endl;
        return;
    }
    const int lAlgo = 20, lOcc = 12, lTemps = 15, lEcart = 12, lRam = 18, lAlloue = 12;
    const int lRatio = 19, lValide = 7;
    std::cout << "| " << std::left << std::setw(lAlgo) << "Algorithme"
              << " | " << std::setw(lOcc) << "Occurrences"
              << " | " << std::setw(lTemps) << "Min (ms)"
              << " | " << std::setw(lTemps) << "Médiane (ms)"
              << " | " << std::setw(lTemps) << "Moyenne (ms)"
              << " | " << std::setw(lTemps) << "Max (ms)"
              << " | " << std::setw(lEcart) << "Écart-type"
              << " | " << std::setw(lRam) << "RAM OS delta (Mo)"
              << " | " << std::setw(lAlloue) << "Alloué (Mo)"
              << " | " << std::setw(lRatio) << "Ratio vs std::find"
              << " | " << std::setw(lValide) << "Valide"
              << " |" << std::endl;
    std::cout << std::string(2 + lAlgo + 3 + lOcc + 3 + lTemps * 4 + 3 + lEcart + 3 + lRam + 3 +
                             lAlloue + 3 + lRatio + 3 + lValide + 2, '-')
              << std::endl;
    for (const ResultatBenchmark& r : resultats) {
        std::ostringstream ratio;
        if (r.ratio < 0.0) {
            ratio << "n/a";
        } else {
            ratio << std::fixed << std::setprecision(2) << r.ratio << "x";
        }
        std::ostringstream occ;
        occ << r.occurrences;
        std::cout << "| " << std::left << std::setw(lAlgo) << r.nomAlgo
                  << " | " << std::setw(lOcc) << occ.str()
                  << " | " << std::right << std::setw(lTemps) << formaterTemps(r.temps.minMs)
                  << " | " << std::setw(lTemps) << formaterTemps(r.temps.medianeMs)
                  << " | " << std::setw(lTemps) << formaterTemps(r.temps.moyenneMs)
                  << " | " << std::setw(lTemps) << formaterTemps(r.temps.maxMs)
                  << " | " << std::setw(lEcart) << formaterTemps(r.temps.ecartTypeMs)
                  << " | " << std::setw(lRam) << std::fixed << std::setprecision(3) << r.ramOsDeltaMo
                  << " | " << std::setw(lAlloue) << r.alloueMo
                  << " | " << std::left << std::setw(lRatio) << ratio.str()
                  << " | " << std::setw(lValide) << (r.valide ? "oui" : "NON")
                  << " |" << std::endl;
    }
    std::cout << "Note : une duree < 0,01 ms est affichee en us." << std::endl;
    const ResultatBenchmark* plusRapide = nullptr;
    const ResultatBenchmark* plusLent = nullptr;
    for (const ResultatBenchmark& r : resultats) {
        if (!r.valide) continue;
        if (plusRapide == nullptr || r.temps.medianeMs < plusRapide->temps.medianeMs) {
            plusRapide = &r;
        }
        if (plusLent == nullptr || r.temps.medianeMs > plusLent->temps.medianeMs) {
            plusLent = &r;
        }
    }
    if (plusRapide != nullptr) {
        std::cout << "Plus rapide : " << plusRapide->nomAlgo << std::endl;
        double secondes = plusRapide->temps.medianeMs / 1000.0;
        if (secondes > 0.0) {
            double debit = (static_cast<double>(tailleTexte) / 1048576.0) / secondes;
            std::cout << "Debit du plus rapide : " << std::fixed << std::setprecision(1) << debit
                      << " Mo/s" << std::endl;
        }
    }
    if (plusLent != nullptr) {
        std::cout << "Plus lent : " << plusLent->nomAlgo << std::endl;
    }
}

// exporterCSV : écrit les résultats en CSV (séparateur ;) avec les infos machine en en-tête
inline void exporterCSV(const std::string& chemin, const std::vector<ResultatBenchmark>& resultats,
                        const std::string& infosMachine) {
    std::ofstream fichier(chemin);
    if (!fichier) {
        std::cout << "Erreur : impossible d'ecrire le fichier " << chemin << std::endl;
        return;
    }
    std::istringstream fluxInfos(infosMachine);
    std::string ligne;
    while (std::getline(fluxInfos, ligne)) {
        fichier << "# " << ligne << "\n";
    }
    fichier << "algorithme;occurrences;min_ms;mediane_ms;moyenne_ms;max_ms;ecart_type_ms;"
              "ram_os_delta_mo;alloue_mo;ratio_vs_std_find;valide\n";
    fichier << std::fixed;
    for (const ResultatBenchmark& r : resultats) {
        fichier << r.nomAlgo << ';' << r.occurrences << ';' << std::setprecision(6)
                << r.temps.minMs << ';' << r.temps.medianeMs << ';' << r.temps.moyenneMs << ';'
                << r.temps.maxMs << ';' << r.temps.ecartTypeMs << ';' << r.ramOsDeltaMo << ';'
                << r.alloueMo << ';';
        if (r.ratio < 0.0) {
            fichier << "n/a";
        } else {
            fichier << std::setprecision(2) << r.ratio;
        }
        fichier << ';' << (r.valide ? "oui" : "non") << "\n";
    }
    std::cout << "Resultats exportes dans " << chemin << std::endl;
}

#endif
