#include "lru.h"

void LRU::invalidate(BaseReplacementData *replacementData) {
    static_cast<LRUData *>(replacementData)->lastAccessTime = 0;
}

void LRU::insert(BaseReplacementData *replacementData) {
    static_cast<LRUData *>(replacementData)->lastAccessTime = ++timer;
}

void LRU::access(BaseReplacementData *replacementData) {
    static_cast<LRUData *>(replacementData)->lastAccessTime = ++timer;
}

CacheEntry *LRU::getVictim(std::vector<CacheEntry> &candidates) {
    CacheEntry* victim = &candidates[0];
    for (auto &candidate : candidates) {
        if (static_cast<LRUData *>(candidate.replacementData)->lastAccessTime <
            static_cast<LRUData *>(victim->replacementData)->lastAccessTime) {
            victim = &candidate;
        }
    }

    return victim;
}

BaseReplacementData *LRU::initializeReplacementData() { return new LRUData(); }