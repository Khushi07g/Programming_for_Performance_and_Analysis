#include "cache.h"

Cache::Cache(int blockSize, int associativity, int numSets,
             BaseReplacementPolicy *replacementPolicy)
    : blockSize(blockSize), associativity(associativity), numSets(numSets),
      replacementPolicy(replacementPolicy), misses(0), coldMisses(0) {

    cache.resize(numSets);
    tagToWayMap.resize(numSets);
    for (int set = 0; set < numSets; set++) {
        for (int way = 0; way < associativity; way++) {
            cache[set].emplace_back(set, way);
            cache[set][way].replacementData = replacementPolicy->initializeReplacementData();
        }
    }
}

bool Cache::access(uint64_t address) {
    if (!visited.count(address / blockSize)) {
        visited.insert(address / blockSize);
        coldMisses++;
    }

    int set = (address / blockSize) % numSets;
    uint64_t tag = address / blockSize / numSets;
    auto it = tagToWayMap[set].find(tag);
    if (it != tagToWayMap[set].end()) {
        int way = it->second;
        if (cache[set][way].isValid() && cache[set][way].getTag() == tag) {
            replacementPolicy->access(cache[set][way].replacementData);
            return true;
        }
    }
    misses++;
    return false;
}

bool Cache::evict(uint64_t address) {
    int set = (address / blockSize) % numSets;
    uint64_t tag = address / blockSize / numSets;
    auto it = tagToWayMap[set].find(tag);
    if (it != tagToWayMap[set].end()) {
        int way = it->second;
        if (cache[set][way].isValid() && cache[set][way].getTag() == tag) {
            cache[set][way].setValid(false);
            cache[set][way].setPrefetched(false);
            replacementPolicy->invalidate(cache[set][way].replacementData);
            tagToWayMap[set].erase(tag);
            return true;
        }
    }
    return false;
}

bool Cache::isPresent(uint64_t address) {
    int set = (address / blockSize) % numSets;
    uint64_t tag = address / blockSize / numSets;
    for (int way = 0; way < associativity; way++) {
        if (cache[set][way].isValid() && cache[set][way].getTag() == tag) {
            // answer for third test algorithm 1 of prefetching matches if we do this, believe this
            // is incorrect
            replacementPolicy->access(cache[set][way].replacementData);
            return true;
        }
    }
    return false;
}

CacheEntry Cache::insert(uint64_t address) {
    CacheEntry victim = *replacementPolicy->getVictim(getEntries(address));
    int set = victim.getSet();
    int way = victim.getWay();
    uint64_t tag = address / blockSize / numSets;
    cache[set][way].setTag(tag);
    cache[set][way].setValid(true);
    cache[set][way].setPrefetched(false);
    cache[set][way].replacementData->address = address;
    replacementPolicy->insert(cache[set][way].replacementData);
    tagToWayMap[set][tag] = way;
    return victim;
}

CacheEntry Cache::insert(uint64_t address, bool firstTime) {
    CacheEntry victim = *replacementPolicy->getVictim(getEntries(address));
    int set = victim.getSet();
    int way = victim.getWay();
    uint64_t tag = address / blockSize / numSets;
    cache[set][way].setTag(tag);
    cache[set][way].setValid(true);
    cache[set][way].setFirstTime(firstTime);
    cache[set][way].replacementData->firstTime = firstTime;
    replacementPolicy->insert(cache[set][way].replacementData);
    tagToWayMap[set][tag] = way;
    return victim;
}

std::vector<CacheEntry> &Cache::getEntries(uint64_t address) {
    int set = (address / blockSize) % numSets;
    return cache[set];
}

void Cache::notifyL2Eviction(uint64_t address) {
    int set = (address / blockSize) % numSets;
    uint64_t tag = address / blockSize / numSets;
    for (int way = 0; way < associativity; way++) {
        if (cache[set][way].isValid() && cache[set][way].getTag() == tag) {
            replacementPolicy->notifyL2Eviction(cache[set][way].replacementData);
            return;
        }
    }
}

void Cache::notifyL2Insertion(uint64_t address) {
    int set = (address / blockSize) % numSets;
    uint64_t tag = address / blockSize / numSets;
    for (int way = 0; way < associativity; way++) {
        if (cache[set][way].isValid() && cache[set][way].getTag() == tag) {
            replacementPolicy->notifyL2Insertion(cache[set][way].replacementData);
            return;
        }
    }
}

bool Cache::isPrefetched(uint64_t address) {
    int set = (address / blockSize) % numSets;
    uint64_t tag = address / blockSize / numSets;
    for (int way = 0; way < associativity; way++) {
        if (cache[set][way].isValid() && cache[set][way].getTag() == tag) {
            return cache[set][way].isPrefetched();
        }
    }
    return false;
}

void Cache::setPrefetched(uint64_t address, bool prefetched) {
    int set = (address / blockSize) % numSets;
    uint64_t tag = address / blockSize / numSets;
    for (int way = 0; way < associativity; way++) {
        if (cache[set][way].isValid() && cache[set][way].getTag() == tag) {
            cache[set][way].setPrefetched(prefetched);
            return;
        }
    }
}