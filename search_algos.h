#ifndef SEARCH_ALGOS_H
#define SEARCH_ALGOS_H

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

using FonctionRecherche = std::vector<size_t> (*)(const std::string&, const std::string&);

struct AlgoRecherche {
    std::string nom;
    FonctionRecherche fonction;
};

// stdFindSearch : trouve toutes les occurrences avec std::string::find en boucle (pos = idx + 1)
inline std::vector<size_t> stdFindSearch(const std::string& texte, const std::string& motif) {
    std::vector<size_t> res;
    if (motif.empty() || motif.size() > texte.size()) return res;
    size_t pos = 0;
    while (true) {
        size_t idx = texte.find(motif, pos);
        if (idx == std::string::npos) break;
        res.push_back(idx);
        pos = idx + 1;
    }
    return res;
}

// stdSearchDefault : std::search avec std::default_searcher construit une fois puis relancé en boucle
inline std::vector<size_t> stdSearchDefault(const std::string& texte, const std::string& motif) {
    std::vector<size_t> res;
    if (motif.empty() || motif.size() > texte.size()) return res;
    std::default_searcher<std::string::const_iterator> chercheur(motif.begin(), motif.end());
    size_t pos = 0;
    while (pos + motif.size() <= texte.size()) {
        auto it = std::search(texte.cbegin() + static_cast<std::ptrdiff_t>(pos), texte.cend(), chercheur);
        if (it == texte.cend()) break;
        size_t idx = static_cast<size_t>(it - texte.cbegin());
        res.push_back(idx);
        pos = idx + 1;
    }
    return res;
}

// boyerMooreSearch : std::search avec std::boyer_moore_searcher construit une fois puis relancé en boucle
inline std::vector<size_t> boyerMooreSearch(const std::string& texte, const std::string& motif) {
    std::vector<size_t> res;
    if (motif.empty() || motif.size() > texte.size()) return res;
    std::boyer_moore_searcher<std::string::const_iterator> chercheur(motif.begin(), motif.end());
    size_t pos = 0;
    while (pos + motif.size() <= texte.size()) {
        auto it = std::search(texte.cbegin() + static_cast<std::ptrdiff_t>(pos), texte.cend(), chercheur);
        if (it == texte.cend()) break;
        size_t idx = static_cast<size_t>(it - texte.cbegin());
        res.push_back(idx);
        pos = idx + 1;
    }
    return res;
}

// horspoolSearch : std::search avec std::boyer_moore_horspool_searcher construit une fois puis relancé en boucle
inline std::vector<size_t> horspoolSearch(const std::string& texte, const std::string& motif) {
    std::vector<size_t> res;
    if (motif.empty() || motif.size() > texte.size()) return res;
    std::boyer_moore_horspool_searcher<std::string::const_iterator> chercheur(motif.begin(), motif.end());
    size_t pos = 0;
    while (pos + motif.size() <= texte.size()) {
        auto it = std::search(texte.cbegin() + static_cast<std::ptrdiff_t>(pos), texte.cend(), chercheur);
        if (it == texte.cend()) break;
        size_t idx = static_cast<size_t>(it - texte.cbegin());
        res.push_back(idx);
        pos = idx + 1;
    }
    return res;
}

// kmpSearch : Knuth-Morris-Pratt manuel avec table de préfixes lps, chevauchements et saut memchr
inline std::vector<size_t> kmpSearch(const std::string& texte, const std::string& motif) {
    std::vector<size_t> res;
    const size_t n = texte.size();
    const size_t m = motif.size();
    if (m == 0 || m > n) return res;
    std::vector<size_t> lps(m, 0);
    for (size_t i = 1, len = 0; i < m;) {
        if (motif[i] == motif[len]) lps[i++] = ++len;
        else if (len != 0) len = lps[len - 1];
        else lps[i++] = 0;
    }
    const unsigned char premier = static_cast<unsigned char>(motif[0]);
    for (size_t i = 0, j = 0; i < n;) {
        if (texte[i] == motif[j]) {
            ++i;
            ++j;
            if (j == m) {
                res.push_back(i - j);
                j = lps[j - 1];
            }
        } else if (j != 0) {
            j = lps[j - 1];
        } else {
            const void* trou = std::memchr(texte.data() + i + 1, premier, n - (i + 1));
            if (trou == nullptr) break;
            i = static_cast<size_t>(static_cast<const char*>(trou) - texte.data());
        }
    }
    return res;
}

// horspoolManual : Horspool manuel avec table de décalages de 256 entrées sur pile et filtre premier octet
inline std::vector<size_t> horspoolManual(const std::string& texte, const std::string& motif) {
    std::vector<size_t> res;
    const size_t n = texte.size();
    const size_t m = motif.size();
    if (m == 0 || m > n) return res;
    size_t decale[256];
    for (size_t k = 0; k < 256; ++k) decale[k] = m;
    for (size_t k = 0; k + 1 < m; ++k) decale[static_cast<unsigned char>(motif[k])] = m - 1 - k;
    const char* td = texte.data();
    const char* md = motif.data();
    const unsigned char premier = static_cast<unsigned char>(motif[0]);
    size_t i = 0;
    while (i + m <= n) {
        if (static_cast<unsigned char>(td[i]) == premier && std::memcmp(td + i, md, m) == 0) {
            res.push_back(i);
            ++i;
        } else {
            i += decale[static_cast<unsigned char>(td[i + m - 1])];
        }
    }
    return res;
}

// rabinKarpSearch : hash glissant base 256 sans division (débordement sur 64 bits), filtres octet + hash, vérifié par memcmp
inline std::vector<size_t> rabinKarpSearch(const std::string& texte, const std::string& motif) {
    std::vector<size_t> res;
    const size_t n = texte.size();
    const size_t m = motif.size();
    if (m == 0 || m > n) return res;
    const unsigned char premier = static_cast<unsigned char>(motif[0]);
    unsigned long long puissance = 1ULL;
    for (size_t i = 1; i < m; ++i) puissance *= 256ULL;
    unsigned long long hTexte = 0ULL;
    unsigned long long hMotif = 0ULL;
    for (size_t i = 0; i < m; ++i) {
        hTexte = hTexte * 256ULL + static_cast<unsigned char>(texte[i]);
        hMotif = hMotif * 256ULL + static_cast<unsigned char>(motif[i]);
    }
    for (size_t i = 0; i + m <= n; ++i) {
        if (static_cast<unsigned char>(texte[i]) == premier && hTexte == hMotif &&
            std::memcmp(texte.data() + i, motif.data(), m) == 0) {
            res.push_back(i);
        }
        if (i + m < n) {
            hTexte = (hTexte - static_cast<unsigned char>(texte[i]) * puissance) * 256ULL +
                     static_cast<unsigned char>(texte[i + m]);
        }
    }
    return res;
}

// memchrSearch : cherche le premier caractère avec memchr, filtre le dernier octet puis valide par memcmp
inline std::vector<size_t> memchrSearch(const std::string& texte, const std::string& motif) {
    std::vector<size_t> res;
    const size_t n = texte.size();
    const size_t m = motif.size();
    if (m == 0 || m > n) return res;
    const unsigned char premier = static_cast<unsigned char>(motif[0]);
    const char dernier = motif[m - 1];
    size_t pos = 0;
    while (pos + m <= n) {
        const void* trou = std::memchr(texte.data() + pos, premier, n - pos);
        if (trou == nullptr) break;
        size_t idx = static_cast<size_t>(static_cast<const char*>(trou) - texte.data());
        if (idx + m > n) break;
        if (texte[idx + m - 1] == dernier && std::memcmp(texte.data() + idx, motif.data(), m) == 0) {
            res.push_back(idx);
        }
        pos = idx + 1;
    }
    return res;
}

// listerAlgorithmes : retourne la liste numérotée des 8 algorithmes disponibles
inline std::vector<AlgoRecherche> listerAlgorithmes() {
    return {
        {"stdFindSearch", &stdFindSearch},
        {"stdSearchDefault", &stdSearchDefault},
        {"boyerMooreSearch", &boyerMooreSearch},
        {"horspoolSearch", &horspoolSearch},
        {"kmpSearch", &kmpSearch},
        {"horspoolManual", &horspoolManual},
        {"rabinKarpSearch", &rabinKarpSearch},
        {"memchrSearch", &memchrSearch}
    };
}

#endif
