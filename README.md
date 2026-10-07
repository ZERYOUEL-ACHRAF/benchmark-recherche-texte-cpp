# Benchmark de Recherche Textuelle Massive (C++17)

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg?style=flat-square&logo=cplusplus)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Compiler](https://img.shields.io/badge/Compiler-GCC%2011.4--MinGW-orange.svg?style=flat-square)](https://gcc.gnu.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat-square)](LICENSE)

Analyse comparative de performance, d'empreinte mémoire et de compromis d'ingénierie de **huit algorithmes de recherche de motifs** au sein d'un corpus de données massif ($\ge 200$ Mo). Ce projet confronte les implémentations manuelles classiques aux outils hautement vectorisés de la bibliothèque standard C++ (`std::string::find` et `std::search`).

---

## 1. Vue d'ensemble du Système

Le cœur applicatif `bench.exe` propose un environnement interactif piloté par menu permettant d'isoler et de caractériser le comportement de chaque algorithme sans biais de mise en cache système ou d'entrées/sorties physiques.

```text
               +-------------------------------------------------------+
               |                  Menu Interactif CLI                  |
               +-------------------------------------------------------+
                     |                     |                     |
        [1-2] Fichier & Motif     [3-5] Exécution Bench     [6-8] Utilitaires
                     |                     |                     |
        * Lecture RAM (String)    * 1 Warm-up d'amorçage    * Générateur de texte
        * Config sensible/casse   * R passes temporelles    * Métriques OS (RSS)
        * Saisie robuste chemins  * R passes mémorielles    * Export CSV machine
```

### Outils de Validation et Micro-Tests
* **`run_tests.exe`** : Suite d'intégration et de non-régression validant la conformité fonctionnelle des algorithmes face aux cas aux limites (motifs vides, chevauchements multiples, caractères spéciaux multi-octets UTF-8, etc.).
* **`quick.exe`** : Micro-benchmark d'évaluation rapide (~2 Mo en RAM, 50 injections contrôlées) conçu pour l'itération rapide et la détection de régressions de performance locales.

---

## 2. Architecture logicielle et Cartographie des sources

Le projet est conçu de manière modulaire, respectant une séparation stricte entre la logique algorithmique, le traçage matériel, la gestion des entrées-sorties et la validation croisée.

```text
.
├── Makefile                     # Automatisation de la compilation multi-cible
├── main.cpp                     # Point d'entrée de l'application interactive CLI
├── tests.cpp                    # Suite de tests unitaires et de validation fonctionnelle
├── test_simple.cpp              # Script du micro-benchmark rapide (quick)
│
├── search_algos.h               # Implémentations des 8 moteurs de recherche
├── benchmark.h                  # Moteur d'évaluation temporelle et mémorielle
├── memory_tracker.h             # Surcharge globale de l'allocateur de tas (new/delete)
├── os_metrics.h                 # Requêtes de télémétrie bas niveau système (Win32)
├── text_generator.h             # Générateur déterministe pseudo-aléatoire (MT19937)
└── input_utils.h                # Primitives d'acquisition et assainissement de saisies CLI
```

### Matrice fonctionnelle des fichiers sources

| Fichier source | Rôle d'ingénierie | Caractère critique / Justification technique |
| :--- | :--- | :--- |
| `search_algos.h` | Moteurs de recherche | Regroupe les signatures uniformisées des 8 algorithmes analysés. |
| `memory_tracker.h` | Profilage mémoire | Intercepte les requêtes `new`/`delete` pour pister le pic d'allocation dynamique exact au niveau de l'octet près. |
| `os_metrics.h` | Télémétrie Noyau | Requête les API d'état de pagination OS pour monitorer la mémoire résidente réelle (RSS via `PeakWorkingSetSize`). |
| `text_generator.h` | Reproductibilité | Génère un jeu d'évaluation à entropie contrôlée via le générateur de nombres pseudo-aléatoires de Mersenne Twister. |

---

## 3. Analyse Algorithmique et Caractérisation de la Complexité

| ID | Identifiant | Famille de design | Complexité Spatiale | Complexité Temporelle (Pire) | Caractéristique d'implémentation |
| :---: | :--- | :--- | :---: | :---: | :--- |
| **1** | `stdFindSearch` | Référence standard | $\mathcal{O}(1)$ | $\mathcal{O}(n \cdot m)$ | Boucle sur `std::string::find`, optimisée par instructions vectorielles SIMD de la glibc/MSVC. |
| **2** | `stdSearchDefault` | Itérateur générique | $\mathcal{O}(1)$ | $\mathcal{O}(n \cdot m)$ | Abstraction standard via `std::default_searcher`. |
| **3** | `boyerMooreSearch` | Sauts heuristiques | $\mathcal{O}(|\Sigma| + m)$ | $\mathcal{O}(n \cdot m)$ | Implémentation standard `std::boyer_moore_searcher`. Sauts par mauvaise concordance de caractères et bons suffixes. |
| **4** | `horspoolSearch` | Sauts simplifiés | $\mathcal{O}(|\Sigma|)$ | $\mathcal{O}(n \cdot m)$ | Version standard `std::boyer_moore_horspool_searcher`. Moins de pré-calculs que Boyer-Moore. |
| **5** | `kmpSearch` | Automate fini | $\mathcal{O}(m)$ | $\mathcal{O}(n + m)$ | Manuel Knuth-Morris-Pratt. Utilise une table de décalage LPS (*Longest Prefix Suffix*) avec pré-filtrage SIMD. |
| **6** | `horspoolManual` | Sauts manuels | $\mathcal{O}(|\Sigma|)$ | $\mathcal{O}(n \cdot m)$ | Implémentation manuelle brute optimisée. Table de décalages sur pile de taille fixe (256 octets). |
| **7** | `rabinKarpSearch` | Hachage glissant | $\mathcal{O}(1)$ | $\mathcal{O}(n \cdot m)$ | Empreinte polynomiale sur 64 bits sans division (débordement naturel modulo $2^{64}$). |
| **8** | `memchrSearch` | Filtrage de pré-sélection | $\mathcal{O}(1)$ | $\mathcal{O}(n \cdot m)$ | Recherche accélérée par `memchr` (SIMD assembleur) combinée à un double pré-filtrage structurel. |

*Légende : $n$ représente la longueur du texte, $m$ la longueur de la phrase (motif), et $|\Sigma|$ la taille de l'alphabet.*

---

## 4. Protocole d'Évaluation Rigoureux (Anti-Biais)

Pour garantir l'intégrité scientifique et technique des mesures de performance, le banc d'essai respecte les spécifications suivantes :
1. **Isolation Horloge** : Recours exclusif à `std::chrono::steady_clock` assurant des mesures temporelles monotones insensibles aux dérives ou corrections de l'horloge système.
2. **Phase d'Amorçage (Warm-up)** : Une itération complète non comptabilisée est exécutée pour chaque algorithme afin de pré-charger les caches d'instructions, de données, ainsi que de stabiliser les prédictions de branchement matérielles.
3. **Robustesse Statistique** : Calcul des métriques statistiques sur $R$ répétitions successives. Utilisation systématique de la **médiane** comme indicateur central de référence pour éliminer les bruits transitoires du système d'exploitation.
4. **Anti-Optimisation Compilateur** : Toutes les écritures de sortie de recherche sont consommées par une barrière d'optimisation :
   ```cpp
   static volatile size_t sink;
   sink = result_vector.size();
   ```
   Ceci empêche le compilateur d'éliminer l'exécution d'un algorithme par élision de code mort.
5. **Isolation Mémoire** : Réinitialisation systématique de l'état de l'allocateur interne et des statistiques OS à chaque itération. Évaluation et métriques mémorielles exécutées lors de passes d'exécution distinctes des passes temporelles.

---

## 5. Résultats expérimentaux : Avant vs Après Optimisation

Le tableau ci-dessous synthétise l'impact des optimisations appliquées sur les implémentations logicielles.

* **Configuration de test** : Fichier de $200$ Mo (209 715 200 octets), 50 occurrences injectées, Phrase recherchée : `"the quick brown fox"` ($19$ octets). $R = 10$ répétitions.

| Algorithme | Temps AVANT (ms)<br>*(Compil. -O2)* | Temps APRÈS (ms)<br>*(Compil. -O3)* | Accélération<br>*(Gain absolu)* | Ratio Relatif AVANT<br>*(vs std::find)* | Ratio Relatif APRÈS<br>*(vs std::find)* |
| :--- | :---: | :---: | :---: | :---: | :---: |
| `stdFindSearch` (Réf.) | 1631.9 | 442.9 | $3.7\times$ | $1.00\times$ | $1.00\times$ |
| `stdSearchDefault` | 1246.4 | 711.3 | $1.8\times$ | $0.76\times$ | $1.61\times$ |
| `boyerMooreSearch` | 459.1 | 200.5 | $2.3\times$ | $0.28\times$ | $0.45\times$ |
| `horspoolSearch` | 379.6 | **123.5** | $3.1\times$ | $0.23\times$ | **0.28x** |
| `kmpSearch` | 1415.2 | 495.5 | $2.9\times$ | $0.87\times$ | $1.12\times$ |
| `horspoolManual` | 418.4 | 164.1 | $2.5\times$ | $0.26\times$ | $0.37\times$ |
| `rabinKarpSearch` | 5975.8 | 1046.2 | **$5.7\times$** | $3.66\times$ | $2.36\times$ |
| `memchrSearch` | 1264.4 | 479.3 | $2.6\times$ | $0.77\times$ | $1.08\times$ |

### Principales Optimisations Introduites (In-Place)
1. **Rabin-Karp sans division** : Remplacement des modulos arithmétiques coûteux par un calcul de hachage polynomial sur 64 bits non signé (`unsigned long long`), tirant parti du débordement naturel d'architecture processeur (équivalent mathématique d'un modulo $2^{64}$ gratuit).
2. **Hybridation KMP & `memchr`** : Introduction d'une phase d'accélération matérielle SIMD lors de la recherche du premier caractère candidat du motif dès lors que l'automate se trouve à l'état initial (LPS = 0).
3. **Heuristique de filtrage par queue** : Dans l'algorithme `memchrSearch`, ajout d'un pré-filtrage rapide validant l'égalité sur le dernier caractère du motif avant de déclencher l'appel coûteux de validation globale via `memcmp`.

---

## 6. Hypothèses Techniques et Analyse d'Environnement (H1 - H13)

### Méthodologie d'Analyse
* **H1** : La barrière d'optimisation `volatile` garantit l'intégrité de la boucle expérimentale.
* **H2** : `steady_clock` offre une résolution nanoseconde fiable et immune aux sauts temporels d'ajustement réseau (NTP).
* **H3** : L'exclusion de la première passe de mesures (Warm-up) fiabilise les résultats en masquant les surcoûts liés aux fautes de pages d'initialisation et à l'établissement de la topologie de cache CPU.
* **H4** : L'étanchéité absolue de l'exécution séquentielle par bloc évite toute pollution inter-algorithmique de l'état de la mémoire physique Noyau.
* **H5** : Les phases de vérification fonctionnelle (justesse) sont exécutées en dehors de l'enveloppe de chronométrage.
* **H6** : Le coût de construction de l'instance de recherche d'un motif est facturé une seule fois, au démarrage de la phase temporelle, pour simuler un usage industriel réaliste.
* **H7** : Les ratios d'échelle temporelle s'appuient exclusivement sur la médiane géométrique des mesures.

### Diagnostics bas niveau de l'Environnement
* **H8** : `std::string::find` est implémenté sous forme d'intrinsèque assembleur hautement optimisé par instructions AVX-2.
* **H9** : Le support UTF-8 considère les caractères non-ASCII comme une suite ordonnée d'octets distincts (sans décodage sémantique unicode).
* **H10** : Le déterminisme de l'évaluation est assuré par l'ancrage fixe de la graine du générateur de Mersenne Twister.
* **H11** (Liaison statique critique) : **L'utilisation des drapeaux `-static-libstdc++ -static-libgcc` est strictement requise**. En liaison dynamique sous MinGW, le moteur de run-time système instancie les structures de chaînes au sein de `libstdc++-6.dll`. Notre traqueur d'allocations globales interceptant l'opérateur d'effacement de mémoire (`operator delete`), cela génère une désynchronisation d'adressage (lecture erronée de l'en-tête de 16 octets), aboutissant invariablement à une violation d'accès ou à une erreur `STATUS_HEAP_CORRUPTION` (0xC0000374).
* **H12** (Analyse de la mémoire OS sous Windows) : La métrique de consommation résidente système (`PeakWorkingSetSize`) de l'API `GetProcessMemoryInfo` constitue un indicateur global cumulatif. Les appels de réinitialisation via `EmptyWorkingSet` ou `SetProcessWorkingSetSize` sous Windows n'effacent pas le pic historique du processus. Pour cette raison, la mesure fine de consommation par algorithme repose sur l'interception de tas via `memory_tracker.h`.
* **H13** (Restriction Smart App Control Windows) : L'environnement Windows Smart App Control (SAC) bloque de façon aléatoire les binaires compilés localement à la volée. En outre, toute redirection de flux standard (`<`, `>`) de binaires non signés sous PowerShell est formellement bloquée. L'évaluation automatisée exhaustive recourt donc à un script piloté par gdb (`gdb -batch -x gdbfull.txt bench.exe`).

---

## 7. Directives d'utilisation

### Compilation et Construction

Le projet est fourni avec un `Makefile` compatible `MinGW-make` / `GNU Make`.

```bash
# Compiler l'intégralité du projet (exécutables optimisés en -O3)
mingw32-make all

# Exécuter les tests de conformité fonctionnelle
mingw32-make test

# Lancer le micro-benchmark de validation rapide
mingw32-make quick

# Lancer l'environnement de benchmark interactif
mingw32-make run

# Nettoyer les artefacts de construction et fichiers temporaires
mingw32-make clean
```
