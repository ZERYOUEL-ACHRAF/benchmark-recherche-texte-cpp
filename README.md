# Benchmark d'algorithmes de recherche de phrases

Comparaison de **8 algorithmes de recherche d'une phrase dans un texte** selon la consigne :
temps réel (`std::chrono::steady_clock`), mémoire (RSS OS + allocations trackées),
validation croisée systématique, et **`std::string::find()` comme référence (1.00x)**.

---

## 1. Présentation et objectifs

Le programme `bench.exe` permet, depuis un menu interactif :

1. charger un fichier texte (généré ou existant) ;
2. définir la phrase à rechercher (avec option casse) ;
3. définir le nombre de répétitions **R** ;
4. lancer le benchmark complet sur les 8 algorithmes (1 warm-up + R passes mesurées, par algorithme) ;
5. bench d'un seul algorithme ;
6. générer un fichier de test massif (taille + graine + phrase à insérer, occurrences connues) ;
7. exporter les derniers résultats en CSV ;
8. afficher les infos machine ;
0. quitter.

Deux programmes de test accompagnent le benchmark :

- `run_tests.exe` — tests de conformité formels (chevauchements, UTF-8, motif vide, casse, cas limites…) ;
- `quick.exe` — micro-benchmark simple et rapide (~2 Mo en mémoire, 50 insertions, < 1 s) idéal pour comparer deux configurations.

---

## 2. Architecture et fichiers

| Fichier | Rôle |
|---|---|
| `search_algos.h` | les 8 algorithmes + `listerAlgorithmes()` (signatures identiques pour tous) |
| `benchmark.h` | `mesurerTemps` / `mesurerMemoire` / `afficherResultats` / `exporterCSV` |
| `memory_tracker.h` | surcharges globales `operator new/delete` : en-tête 16 octets, compteurs + pic d'allocations |
| `os_metrics.h` | `PeakWorkingSetSize` via `GetProcessMemoryInfo` + `resetPeakRSS()` (`EmptyWorkingSet`) |
| `text_generator.h` | génération déterministe (graine `std::mt19937`), insertions connues |
| `input_utils.h` | saisies robustes (chemin, entier, phrase, o/n) |
| `main.cpp` | menu interactif — **aucune donnée codée en dur** (tout vient de l'utilisateur) |
| `tests.cpp` | conformité des 8 algorithmes |
| `test_simple.cpp` | micro-benchmark rapide `quick` |
| `Makefile` | cibles `all`, `run`, `test`, `quick`, `clean` |

Aucune donnée de test n'est codée en dur dans `main.cpp` (exigence de consigne) : tout est saisi ou généré à l'exécution.

---

## 3. Les 8 algorithmes

| # | Nom | Principe | Complexité |
|---|---|---|---|
| 1 | `stdFindSearch` | `std::string::find` en boucle, `pos = idx + 1` | **référence** |
| 2 | `stdSearchDefault` | `std::search` + `std::default_searcher` construit une fois | O(n·m) |
| 3 | `boyerMooreSearch` | `std::search` + `std::boyer_moore_searcher` | quasi O(n+m) |
| 4 | `horspoolSearch` | `std::search` + `std::boyer_moore_horspool_searcher` | O(n+m) |
| 5 | `kmpSearch` | Knuth-Morris-Pratt manuel (table `lps`, chevauchements) + saut `memchr` sur le 1er caractère | O(n+m) |
| 6 | `horspoolManual` | Horspool manuel : table 256 entrées sur pile, filtre 1er octet + `memcmp` | O(n+m) |
| 7 | `rabinKarpSearch` | hash glissant base 256 **sans division** (débordement 64 bits), filtres 1er octet + hash + `memcmp` | O(n) amorti |
| 8 | `memchrSearch` | `memchr` sur le 1er caractère, filtre dernier octet, validation `memcmp` | O(n) |

Tous renvoient `std::vector<size_t>` des positions, **résultats chevauchants** (`pos = idx + 1`),
motif vide → vecteur vide. **Vérification hors mesure** (positions comparées à la référence après timing).

---

## 4. Compilation et exécution

```bat
mingw32-make all     :: bench.exe + run_tests.exe (0 erreur / 0 warning)
mingw32-make test    :: 8x [OK] — conformité des algorithmes
mingw32-make quick   :: micro-benchmark < 1 s (texte 2 Mo, 50 insertions)
mingw32-make run     :: benchmark interactif complet
mingw32-make clean
```

Flags : `-std=c++17 -O3 -Wall -Wextra -static-libstdc++ -static-libgcc`
(le statique est **indispensable**, voir H11).

Session reproductible de référence (200 Mo, R=10) :
`gdb -batch -x gdbfull.txt bench.exe` avec `session_full.txt`
(génération 200 Mo → chargement → phrase `the quick brown fox` (19 o, sensible à la casse)
→ R=10 → benchmark complet → `resultats.csv` → 0).

Sortie CSV (`resultats.csv`, séparateur `;`, en-tête machine `#`) :

```
algorithme;occurrences;min_ms;mediane_ms;moyenne_ms;max_ms;ecart_type_ms;
ram_os_delta_mo;alloue_mo;ratio_vs_std_find;valide
```

---

## 5. Protocole de mesure (conforme consigne)

- **Horloge unique** : `std::chrono::steady_clock` partout (monotone, non affectée par l'horloge système).
- **Warm-up + répétitions** : 1 passe non mesurée puis **R passes mesurées par algorithme**, passes séparées par algorithme (aucun croisement).
- **Tendances** : min / **médiane** (utilisée pour les ratios) / moyenne / max / écart-type.
- **Ratio** = médiane de l'algorithme ÷ médiane de `stdFindSearch` (mêmes conditions).
- **Anti-optimisation** : `static volatile size_t sink` enregistre les positions retournées — le compilateur ne peut pas supprimer les appels (H1).
- **Validation hors timing** : les positions de chaque algo sont comparées à `stdFindSearch` (égalité stricte des vecteurs) → colonne `valide`.
- **Mémoire par algorithme** : `resetTracker()` (allocations) et `resetPeakRSS()` (pic OS) avant chaque passe ;
  colonnes `alloue_mo` (tracker, précis par algo) et `ram_os_delta_mo` (pic `PeakWorkingSetSize`).
- **Débit** : Mo/s du plus rapide affiché en fin de tableau.

---

## 6. Résultats (AVANT → APRÈS optimisation)

### 6.1 Session complète 200 Mo, R=10 — médianes en ms (`resultats_avant.csv` → `resultats.csv`)

| Algorithme | AVANT (ms) | APRÈS (ms) | Gain absolu | Ratio AVANT | Ratio APRÈS |
|---|---:|---:|---:|---:|---:|
| stdFindSearch (réf.) | 1631.9 | 442.9 | 3,7x | 1.00x | 1.00x |
| stdSearchDefault | 1246.4 | 711.3 | 1,8x | 0.76x | 1.61x |
| boyerMooreSearch | 459.1 | 200.5 | 2,3x | 0.28x | 0.45x |
| horspoolSearch | 379.6 | 123.5 | 3,1x | 0.23x | 0.28x |
| kmpSearch | 1415.2 | 495.5 | 2,9x | 0.87x | 1.12x |
| horspoolManual | 418.4 | 164.1 | 2,5x | 0.26x | 0.37x |
| rabinKarpSearch | 5975.8 | 1046.2 | **5,7x** | 3.66x | **2.36x** |
| memchrSearch | 1264.4 | 479.3 | 2,6x | 0.77x | 1.08x |

- Les 50 occurrences sont trouvées par les 8 algorithmes (`valide = oui` partout).
- Plus rapide : `horspoolSearch` ; plus lent : `rabinKarpSearch`.
- Les **ratios** évoluent aussi parce que la référence elle-même accélère avec `-O3`
  (les `find`/`search` de la lib standard sont des templates inclus dans nos TU).

### 6.2 Micro-benchmark `quick` (2 Mo, R=5) — médianes en ms

| Algorithme | AVANT (-O2) | APRÈS (-O3) | Ratio AVANT | Ratio APRÈS |
|---|---:|---:|---:|---:|
| stdFindSearch | 1.897 | 1.890 | 1.00x | 1.00x |
| stdSearchDefault | 1.289 | 1.454 | 0.68x | 0.77x |
| boyerMooreSearch | 0.597 | 0.599 | 0.31x | 0.32x |
| horspoolSearch | 0.480 | 0.483 | 0.25x | 0.26x |
| kmpSearch | 2.310 | 1.814 | **1.22x** | **0.96x** |
| horspoolManual | 0.845 | 0.683 | 0.45x | 0.36x |
| rabinKarpSearch | 12.956 | 2.856 | **6.83x** | **1.51x** |
| memchrSearch | 2.044 | 1.715 | 1.08x | 0.91x |

### 6.3 Optimisations appliquées (in place, signatures inchangées)

1. **rabinKarpSearch** : suppression des 4 `div/mod` par position (hash polynomial sans division,
   débordement contrôlé sur `unsigned long long`) + pré-filtre 1er octet avant `memcmp`.
2. **kmpSearch** : quand `j == 0` et échec, saut direct au prochain candidat 1er caractère via `memchr` (SIMD) au lieu de `++i`.
3. **memchrSearch** : vérification du **dernier** octet avant le `memcmp` complet.
4. **horspoolManual** : pointeurs bruts, table de décalages sur pile, filtre 1er octet avant `memcmp`.
5. **Makefile** : `-O2` → `-O3`.

`stdFindSearch` et `boyerMooreSearch` (les deux références de la lib standard) sont laissés intacts.
Validation complète : `make all` 0/0 → `make test` 8×`[OK]` → session 200 Mo `valide = oui` partout.

---

## 7. Hypothèses (H1–H13) et notes d'environnement

**Méthodologie**

- **H1** — `static volatile size_t sink` : les positions retournées sont écrites dans une variable
  `volatile` ; le compilateur ne peut donc pas éliminer les appels de recherche comme « code mort »
  (résultat jamais utilisé).
- **H2** — `steady_clock` est monotone : non affectée par les ajustements d'horloge système
  (contrairement à `system_clock`), donc fiable pour mesurer des durées.
- **H3** — 1 warm-up + médiane sur R passes : le warm-up amorce le cache/branch predictor,
  la médiane rejette les pics (anti-virus, préempt) bien mieux que la moyenne.
- **H4** — passes séparées par algorithme + remise à zéro des compteurs avant chaque passe :
  chaque algorithme part du même état mémoire (allocations et pic OS).
- **H5** — validation **hors mesure** : comparer les vecteurs de positions après le timing
  n'ajoute aucune bruit aux mesures, tout en garantissant la correction.
- **H6** — `std::search` : le searcher (`default`/`boyer_moore`/`horspool`) est construit **une seule
  fois** avant la boucle de recherche, puis réutilisé à chaque reprise après correspondance —
  le coût de construction du motif n'est payé qu'une fois.
- **H7** — ratio de médianes (pas de moyennes ni de mins) : le ratio devient robuste au bruit
  de mesure et comparable entre sessions.
- **H8** — `stdFindSearch` comme référence : `std::string::find` de la lib standard est extrêmement
  optimisé (souvent `memchr` intrinsèque) — le comparer à d'autres implémentations est le but du projet.
- **H9** — comparaison par octets : la phrase et le texte sont comparés octet par octet ;
  l'option casse (`o/n`) décide si `'A' == 'a'` ; le UTF-8 est traité comme des séquences d'octets.
- **H10** — génération déterministe : graine `std::mt19937` fixe + insertions à positions connues
  → occurrences exactes (50 dans les sessions), sessions reproductibles.

**Technique / environnement**

- **H11** — `-static-libstdc++ -static-libgcc` est **indispensable** : sans lien statique de la lib
  C++, `std::string` était construit dans `libstdc++-6.dll` (pas d'en-tête de 16 octets de notre
  `memory_tracker.h`) alors que `operator delete` inlined lisait cet en-tête → `free()` d'un mauvais
  pointeur → `STATUS_HEAP_CORRUPTION` (0xC0000374) au premier `free()`.
- **H12** — la colonne `ram_os_delta_mo` reflète le **pic global** `PeakWorkingSetSize` du processus :
  Windows ne sait pas réinitialiser ce pic (`EmptyWorkingSet` et `SetProcessWorkingSetSize` sont
  sans effet dessus — testé). La mémoire **par algorithme** est donc portée par `alloue_mo`
  (tracker d'allocations, remis à zéro avant chaque passe). Le pic ≈ 808 Mo est atteint au chargement
  du texte de 200 Mo.
- **H13** — exécution sous SAC (Smart App Control) : sur cette machine, les exécutables non signés
  sont bloqués aléatoirement (erreur 4551, événements CodeIntegrity 3033/3077/3118) ;
  en outre, **toute redirection d'entrée/sortie** (`<`, `>`, pipes) d'un exécutable de ce projet
  échoue au chargement. Les sessions batch passent donc par :
  `gdb -batch -x gdbfull.txt bench.exe` avec `run < session_full.txt` (gdb hérite des poignées
  sans redirection côté shell). Correctifs constatés : recompiler (nouveau hash) jusqu'au passage,
  ou compiler avec `-g` / `-s` (contenu différent → nouvelle évaluation). `make test` fonctionne
  directement (spawn hérité par make).

**Limites connues** : bruit machine notable (écart-types élevés sur les sessions 200 Mo, charge
SAC/cloud en arrière-plan) — d'où le recours systématique aux médianes et à la double mesure
(`quick` + session 200 Mo).
#   b e n c h m a r k - r e c h e r c h e - t e x t e - c p p  
 