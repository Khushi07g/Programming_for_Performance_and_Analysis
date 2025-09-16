#include "cache_hierarchy.h"

void InclusiveCacheHierarchy::access(uint64_t address) {
    if (l2.access(address)) {
        return; // L2 hit
    }

    if (l3.access(address)) {
        l2.insert(address); // L3 hit, fetch into L2
        return;
    }

    CacheEntry victim = l3.insert(address); // L3 miss, fetch into L3
    if (victim.isValid()) {
        uint64_t victimAddress =
            (victim.getTag() * l3.getNumSets() + victim.getSet()) * l3.getBlockSize();
        l2.evict(victimAddress);
    }
    l2.insert(address);
}

void ExclusiveCacheHierarchy::access(uint64_t address) {
    if (l2.access(address)) {
        return; // L2 hit
    }

    if (l3.access(address)) {
        l3.evict(address);
    }

    CacheEntry victim = l2.insert(address); // L3 miss, fetch into L2
    if (victim.isValid()) {
        uint64_t victimAddress =
            (victim.getTag() * l2.getNumSets() + victim.getSet()) * l2.getBlockSize();
        l3.insert(victimAddress);
    }
}

void ExclusiveV2CacheHierarchy::access(uint64_t address) {
    if (l2.access(address)) {
        return; // L2 hit
    }

    if (l3.access(address)) {
        l3.evict(address);
        CacheEntry victim = l2.insert(address, false); // L3 hit, fetch into L2
        if (victim.isValid()) {
            uint64_t victimAddress =
                (victim.getTag() * l2.getNumSets() + victim.getSet()) * l2.getBlockSize();
            l3.insert(victimAddress, victim.isFirstTime());
        }
        return;
    }

    CacheEntry victim = l2.insert(address, true); // L3 miss, fetch into L2
    if (victim.isValid()) {
        uint64_t victimAddress =
            (victim.getTag() * l2.getNumSets() + victim.getSet()) * l2.getBlockSize();
        l3.insert(victimAddress, victim.isFirstTime());
    }
}

void NineCacheHierarchy::access(uint64_t address) {
    if (l2.access(address)) {
        return; // L2 hit
    }

    if (l3.access(address)) {
        l2.insert(address); // fetch into L2
        return;             // L3 hit
    }

    l3.insert(address); // L3 miss, fetch into L3
    l2.insert(address); // fetch into L2
}

void InclusiveNotifyCacheHierarchy::access(uint64_t address) {
    if (l2.access(address)) {
        return; // L2 hit
    }

    if (l3.access(address)) {
        CacheEntry victim = l2.insert(address); // L3 hit, fetch into L2
        if (victim.isValid()) {
            uint64_t victimAddress =
                (victim.getTag() * l2.getNumSets() + victim.getSet()) * l2.getBlockSize();
            l3.notifyL2Eviction(victimAddress);
        }
        l3.notifyL2Insertion(address);
        return;
    }
    CacheEntry victim = l3.insert(address); // L3 miss, fetch into L3
    if (victim.isValid()) {
        uint64_t victimAddress =
            (victim.getTag() * l3.getNumSets() + victim.getSet()) * l3.getBlockSize();
        l2.evict(victimAddress);
    }
    CacheEntry victim_l2 = l2.insert(address);
    if (victim_l2.isValid()) {
        uint64_t victimAddress =
            (victim_l2.getTag() * l2.getNumSets() + victim_l2.getSet()) * l2.getBlockSize();
        l3.notifyL2Eviction(victimAddress);
    }
    l3.notifyL2Insertion(address);
}

void OptPolicyCache::access(uint64_t address) {
    if (l2.access(address)) {
        l3.access(address); // to update OPT state in L3
        return;             // L2 hit
    }

    if (l3.access(address)) {
        l2.insert(address); // L3 hit, fetch into L2
        return;
    }

    CacheEntry victim = l3.insert(address); // L3 miss, fetch into L3
    if (victim.isValid()) {
        uint64_t victimAddress =
            (victim.getTag() * l3.getNumSets() + victim.getSet()) * l3.getBlockSize();
        l2.evict(victimAddress);
    }
    l2.insert(address);
}

void PrefetchCacheHierarchy::prefetch(uint64_t address) {
    auto [prefetch, prefetchBlkAddress] = prefetcher->getPrefetch(address);
    if (!prefetch || 32 * prefetchBlkAddress / 4096 != address / 4096)
        return;

    uint64_t prefetchAddress = prefetchBlkAddress * l2.getBlockSize();
    if (l2.isPresent(prefetchAddress))
        return;

    if (!l3.isPresent(prefetchAddress)) {
        CacheEntry victim = l3.insert(prefetchAddress);
        l3.setPrefetched(prefetchAddress, true);
        l3Prefetches++;
        if (victim.isValid()) {
            uint64_t victimAddress =
                (victim.getTag() * l3.getNumSets() + victim.getSet()) * l3.getBlockSize();
            l2.evict(victimAddress);
        }
    }
    l2.insert(prefetchAddress);
    l2.setPrefetched(prefetchAddress, true);
    l2Prefetches++;
}

void PrefetchCacheHierarchy::access(uint64_t address) {
    if (l2.access(address)) {
        if (l2.isPrefetched(address)) {
            usefulL2Prefetches++;
            l2.setPrefetched(address, false);
        }
        prefetch(address);
        return;
    }

    if (l3.access(address)) {
        if (l3.isPrefetched(address)) {
            usefulL3Prefetches++;
            l3.setPrefetched(address, false);
        }
        l2.insert(address); // L3 hit, fetch into L2
        prefetch(address);
        return;
    }

    CacheEntry victim = l3.insert(address); // L3 miss, fetch into L3
    if (victim.isValid()) {
        uint64_t victimAddress =
            (victim.getTag() * l3.getNumSets() + victim.getSet()) * l3.getBlockSize();
        l2.evict(victimAddress);
    }
    l2.insert(address);
    prefetch(address);
}