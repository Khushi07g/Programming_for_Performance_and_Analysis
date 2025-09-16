#ifndef LRU_H
#define LRU_H
#include "base_replacement_policy.h"

class LRU : public BaseReplacementPolicy {
  private:
    struct LRUData : BaseReplacementData {
        uint64_t lastAccessTime;
        LRUData() : lastAccessTime(0) {} // 0 means invalid
    };
    uint64_t timer;

  public:
    LRU() : timer(0) {}

    void invalidate(BaseReplacementData *replacementData) override;
    void insert(BaseReplacementData *replacementData) override;
    void access(BaseReplacementData *replacementData) override;
    CacheEntry *getVictim(std::vector<CacheEntry> &candidates) override;

    BaseReplacementData *initializeReplacementData() override;
};

#endif // LRU_H