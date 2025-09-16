#include "prefetcher.h"
#include "base.h"
#include <cstdint>

Prefetcher::Prefetcher(int n) : n(n) {
    table = new PrefetchTableEntry[n];
    for (int i = 0; i < n; i++) {
        table[i].valid = false;
        table[i].stride = 0;
        table[i].blkAddress = 0;
        table[i].key = 0;
        table[i].lastAccessTime = 0;
    }
}

std::pair<bool, uint64_t> Prefetcher::getPrefetch(uint64_t address) {
    bool prefetch = false;
    bool found = false;

    uint64_t nextAddress = 0;
    uint64_t blkAddress = address / 32;
    for (int i = 0; i < n; i++) {
        if (!table[i].valid || table[i].key != key)
            continue;

        found = true;

        if (table[i].blkAddress == blkAddress) {
            table[i].lastAccessTime = currentTime;
            break;
        }

        if (table[i].stride == 0) {
            table[i].stride = blkAddress - table[i].blkAddress;
            table[i].blkAddress = blkAddress;
            table[i].lastAccessTime = currentTime;
            break;
        }

        if (table[i].blkAddress + table[i].stride == blkAddress) {
            table[i].blkAddress = blkAddress;
            table[i].lastAccessTime = currentTime;
            prefetch = true;
            nextAddress = blkAddress + table[i].stride;
            break;
        }

        table[i].stride = blkAddress - table[i].blkAddress;
        table[i].blkAddress = blkAddress;
        table[i].lastAccessTime = currentTime;
        break;
    }

    if (!found)
        updateState(address);

    return std::make_pair(prefetch, nextAddress);
}

void Prefetcher::updateState(uint64_t address) {
    uint64_t blkAddress = address / 32;

    int victim = findVictim();

    table[victim].valid = true;
    table[victim].key = key;
    table[victim].blkAddress = blkAddress;
    table[victim].lastAccessTime = currentTime;
    table[victim].stride = 0;
}

int Prefetcher::findVictim() {
    for (int i = 0; i < n; i++) {
        if (!table[i].valid)
            return i;
    }

    int ind = 0;
    for (int i = 1; i < n; i++) {
        if (table[i].lastAccessTime < table[ind].lastAccessTime) {
            ind = i;
        }
    }

    return ind;
}