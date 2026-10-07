#ifndef MEMORY_TRACKER_H
#define MEMORY_TRACKER_H

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>

namespace memory_tracker {

inline std::atomic<size_t>& compteurTotal() {
    static std::atomic<size_t> total{0};
    return total;
}

inline std::atomic<size_t>& compteurActuel() {
    static std::atomic<size_t> actuel{0};
    return actuel;
}

inline std::atomic<size_t>& compteurPic() {
    static std::atomic<size_t> pic{0};
    return pic;
}

inline std::atomic<size_t>& compteurBase() {
    static std::atomic<size_t> base{0};
    return base;
}

// trackerOnAlloc : enregistre une allocation (total cumulé, pic depuis le dernier reset)
inline void trackerOnAlloc(size_t octets) {
    compteurTotal().fetch_add(octets, std::memory_order_relaxed);
    size_t actuel = compteurActuel().fetch_add(octets, std::memory_order_relaxed) + octets;
    size_t base = compteurBase().load(std::memory_order_relaxed);
    size_t delta = actuel > base ? actuel - base : 0;
    size_t pic = compteurPic().load(std::memory_order_relaxed);
    while (delta > pic && !compteurPic().compare_exchange_weak(pic, delta, std::memory_order_relaxed)) {
    }
}

// trackerOnFree : enregistre la libération d'un bloc alloué par le tracker
inline void trackerOnFree(size_t octets) {
    size_t actuel = compteurActuel().load(std::memory_order_relaxed);
    while (actuel >= octets &&
           !compteurActuel().compare_exchange_weak(actuel, actuel - octets, std::memory_order_relaxed)) {
    }
}

// resetTracker : remet à zéro les compteurs de fenêtre de mesure (le pic repart du niveau courant)
inline void resetTracker() {
    compteurTotal().store(0, std::memory_order_relaxed);
    compteurBase().store(compteurActuel().load(std::memory_order_relaxed), std::memory_order_relaxed);
    compteurPic().store(0, std::memory_order_relaxed);
}

// trackerPeakBytes : retourne le pic d'octets alloués depuis le dernier resetTracker
inline size_t trackerPeakBytes() {
    return compteurPic().load(std::memory_order_relaxed);
}

// trackerTotalAllocated : retourne le total d'octets alloués depuis le dernier resetTracker
inline size_t trackerTotalAllocated() {
    return compteurTotal().load(std::memory_order_relaxed);
}

// trackerCurrentBytes : retourne le nombre d'octets actuellement alloués par le programme
inline size_t trackerCurrentBytes() {
    return compteurActuel().load(std::memory_order_relaxed);
}

}  // namespace memory_tracker

constexpr size_t kEnTeteOctets = 16;

// operator new : alloue via malloc avec un en-tête de 16 octets et compte les octets alloués
void* operator new(std::size_t octets) {
    if (octets == 0) octets = 1;
    if (octets > static_cast<std::size_t>(-1) - kEnTeteOctets) throw std::bad_alloc();
    void* brut = std::malloc(octets + kEnTeteOctets);
    if (brut == nullptr) throw std::bad_alloc();
    *static_cast<std::size_t*>(brut) = octets;
    memory_tracker::trackerOnAlloc(octets);
    return static_cast<char*>(brut) + kEnTeteOctets;
}

// operator new[] : variante tableau de l'allocation suivie
void* operator new[](std::size_t octets) {
    return operator new(octets);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"

// operator delete : libère un bloc du tracker en lisant sa taille dans l'en-tête
void operator delete(void* pointeur) noexcept {
    if (pointeur == nullptr) return;
    void* brut = static_cast<char*>(pointeur) - kEnTeteOctets;
    memory_tracker::trackerOnFree(*static_cast<std::size_t*>(brut));
    std::free(brut);
}

// operator delete[] : variante tableau de la libération suivie
void operator delete[](void* pointeur) noexcept {
    operator delete(pointeur);
}

// operator delete dimensionné : ignore la taille donnée et lit l'en-tête comme pour delete
void operator delete(void* pointeur, std::size_t) noexcept {
    operator delete(pointeur);
}

// operator delete[] dimensionné : variante tableau de la libération dimensionnée
void operator delete[](void* pointeur, std::size_t) noexcept {
    operator delete(pointeur);
}

#pragma GCC diagnostic pop

#endif
