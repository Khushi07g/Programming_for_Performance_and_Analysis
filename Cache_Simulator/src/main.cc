#include "base.h"
#include "cache.h"
#include "cache_hierarchy.h"
#include "lru.h"
#include "opt_policy.h"
#include "prefetcher.h"
#include "quadage.h"
#include "quadage_v2.h"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

uint64_t currentTime = 0;

int main(int argc, char *argv[]) {
    Quadage quadage = Quadage();
    QuadageV2 quadageV2 = QuadageV2();
    QuadageNotify quadageNotify = QuadageNotify();
    OptPolicy optPolicy =
        OptPolicy(32, 24, 2048); // L3: 32B block size, 24-way, 2048 sets, Quadage, 1.5 MB
    OptPolicy fullyAssocOptPolicy = OptPolicy(32, 24 * 2048, 1); // Fully associative L3

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <trace_file> <num_traces>" << std::endl;
        return 1;
    }

    InclusiveCacheHierarchy inclusiveCacheHierarchy(
        32, 8, 2048, new LRU(), // L2: 32B block size, 8-way, 2048 sets, LRU, 512 kB
        32, 24, 2048, &quadage  // L3: 32B block size, 24-way, 2048 sets, Quadage, 1.5 MB
    );

    ExclusiveCacheHierarchy exclusiveCacheHierarchy(
        32, 8, 2048, new LRU(), // L2: 32B block size, 8-way, 2048 sets, LRU, 512 kB
        32, 24, 2048, &quadage  // L3: 32B block size, 24-way, 2048 sets, Quadage, 1.5 MB
    );

    ExclusiveV2CacheHierarchy exclusiveV2CacheHierarchy(
        32, 8, 2048, new LRU(),  // L2: 32B block size, 8-way, 2048 sets, LRU, 512 kB
        32, 24, 2048, &quadageV2 // L3: 32B block size, 24-way, 2048 sets, Quadage, 1.5 MB
    );

    NineCacheHierarchy nineCacheHierarchy(
        32, 8, 2048, new LRU(), // L2: 32B block size, 8-way, 2048 sets, LRU, 512 kB
        32, 24, 2048, &quadage  // L3: 32B block size, 24-way, 2048 sets, Quadage, 1.5 MB
    );

    InclusiveNotifyCacheHierarchy inclusiveNotifyCacheHierarchy(
        32, 8, 2048, new LRU(),      // L2: 32B block size, 8-way, 2048 sets, LRU, 512 kB
        32, 24, 2048, &quadageNotify // L3: 32B block size, 24-way, 2048 sets, QuadageNotify, 1.5 MB
    );

    InclusiveCacheHierarchy fullyAssociativeL3(
        32, 8, 2048, new LRU(),    // L2: 32B block size, 8-way, 2048 sets, LRU, 512 kB
        32, 2048 * 24, 1, &quadage // L3: 32B block size, fully associate, 1.5 MB
    );

    OptPolicyCache optPolicyCache(
        32, 8, 2048, new LRU(),  // L2: 32B block size, 8-way, 2048 sets, LRU, 512 kB
        32, 24, 2048, &optPolicy // L3: 32B block size, 24-way, 2048 sets, OPT, 1.5 MB
    );

    OptPolicyCache fullyAssocOptPolicyCache(
        32, 8, 2048, new LRU(),                // L2: 32B block size, 8-way, 2048 sets, LRU, 512 kB
        32, 24 * 2048, 1, &fullyAssocOptPolicy // L3: 32B block size, fully associative, OPT, 1.5 MB
    );

    Prefetcher addressStridePrefetcher = Prefetcher(32);
    PrefetchCacheHierarchy pagePrefetchCacheHierarchy(32, 8, 2048, new LRU(), 32, 24, 2048,
                                                      &quadage, &addressStridePrefetcher);

    Prefetcher pcStridePrefetcher = Prefetcher(32);
    PrefetchCacheHierarchy pcPrefetchCacheHierarchy(32, 8, 2048, new LRU(), 32, 24, 2048, &quadage,
                                                    &pcStridePrefetcher);

    std::vector<std::pair<uint64_t, uint64_t>> addresses;

    FILE *fp;
    char input_name[256];
    int numtraces = atoi(argv[2]);
    char i_or_d;
    char type;
    unsigned long long addr;
    unsigned pc;
    for (int k = 0; k < numtraces; k++) {
        sprintf(input_name, "%s_%d", argv[1], k);
        fp = fopen(input_name, "rb");
        assert(fp != NULL);

        while (!feof(fp)) {
            fread(&i_or_d, sizeof(char), 1, fp);
            fread(&type, sizeof(char), 1, fp);
            fread(&addr, sizeof(unsigned long long), 1, fp);
            fread(&pc, sizeof(unsigned), 1, fp);

            if (type == 0)
                continue;

            addresses.emplace_back(addr, pc);

            fullyAssocOptPolicyCache.loadFutureAccesses()[addr / 32].push(currentTime);
            optPolicyCache.loadFutureAccesses()[addr / 32].push(currentTime);
            currentTime++;
        }
        fclose(fp);
        printf("Done reading file %d!\n", k);
    }

    for (auto &entry : optPolicyCache.loadFutureAccesses()) {
        entry.second.push(currentTime);
        currentTime++;
    }
    for (auto &entry : fullyAssocOptPolicyCache.loadFutureAccesses()) {
        entry.second.push(currentTime);
        currentTime++;
    }
    currentTime = 0;

    for (auto &[addr, pc] : addresses) {
        currentTime++;

        fullyAssocOptPolicyCache.access(addr);
        optPolicyCache.access(addr);
        inclusiveCacheHierarchy.access(addr);
        exclusiveCacheHierarchy.access(addr);
        exclusiveV2CacheHierarchy.access(addr);
        nineCacheHierarchy.access(addr);
        inclusiveNotifyCacheHierarchy.access(addr);
        fullyAssociativeL3.access(addr);
        pagePrefetchCacheHierarchy.access(addr, addr / 4096);
        pcPrefetchCacheHierarchy.access(addr, pc);
    }

    printf("\n%-18s | %-12s | %-12s\n", "Cache Type", "L3 Misses", "L2 Misses");
    printf("-------------------+--------------+--------------\n");
    printf("%-18s | %-12lu | %-12lu\n", "Inclusive", inclusiveCacheHierarchy.getL3().getMisses(),
           inclusiveCacheHierarchy.getL2().getMisses());
    printf("%-18s | %-12lu | %-12lu\n", "NINE", nineCacheHierarchy.getL3().getMisses(),
           nineCacheHierarchy.getL2().getMisses());
    printf("%-18s | %-12lu | %-12lu\n", "Exclusive", exclusiveCacheHierarchy.getL3().getMisses(),
           exclusiveCacheHierarchy.getL2().getMisses());
    printf("%-18s | %-12lu | %-12lu\n", "Inclusive Notify",
           inclusiveNotifyCacheHierarchy.getL3().getMisses(),
           inclusiveNotifyCacheHierarchy.getL2().getMisses());
    printf("%-18s | %-12lu | %-12lu\n", "Exclusive V2",
           exclusiveV2CacheHierarchy.getL3().getMisses(),
           exclusiveV2CacheHierarchy.getL2().getMisses());
    printf("-------------------------------------------------\n");

    printf(
        "Cold: %lu, Capacity: %lu, Conflict (quad-age L3): %lu, Conflict (OPT L3): %ld\n",
        fullyAssocOptPolicyCache.getL3().getColdMisses(),
        fullyAssocOptPolicyCache.getL3().getMisses() -
            fullyAssocOptPolicyCache.getL3().getColdMisses(),
        inclusiveCacheHierarchy.getL3().getMisses() - fullyAssocOptPolicyCache.getL3().getMisses(),
        (int64_t)optPolicyCache.getL3().getMisses() - fullyAssocOptPolicyCache.getL3().getMisses());
    printf("Cold: %lu, Capacity: %lu, Conflict (quad-age L3): %lu, Conflict (OPT L3): %ld\n",
           fullyAssociativeL3.getL3().getColdMisses(),
           fullyAssociativeL3.getL3().getMisses() - fullyAssociativeL3.getL3().getColdMisses(),
           inclusiveCacheHierarchy.getL3().getMisses() - fullyAssociativeL3.getL3().getMisses(),
           (int64_t)optPolicyCache.getL3().getMisses() - fullyAssociativeL3.getL3().getMisses());

    printf("-------------------------------------------------\n");

    printf("Page Prefetcher: L3 Misses: %lu, L2 Misses: %lu, L2 Accuracy: %.2f, L2 Coverage: "
           "%.2f, L3 Coverage %.2f\n",
           pagePrefetchCacheHierarchy.getL3().getMisses(),
           pagePrefetchCacheHierarchy.getL2().getMisses(),
           (pagePrefetchCacheHierarchy.getL2Prefetches() == 0
                ? 0.0
                : (1.0 * pagePrefetchCacheHierarchy.getL2UsefulPrefetches() /
                   pagePrefetchCacheHierarchy.getL2Prefetches())),
           pagePrefetchCacheHierarchy.getL2Prefetches() == 0
               ? 0.0
               : (1.0 * pagePrefetchCacheHierarchy.getL2UsefulPrefetches() /
                  (pagePrefetchCacheHierarchy.getL2().getMisses() +
                   pagePrefetchCacheHierarchy.getL2UsefulPrefetches())),
           pagePrefetchCacheHierarchy.getL3Prefetches() == 0
               ? 0.0
               : (1.0 * pagePrefetchCacheHierarchy.getL3UsefulPrefetches() /
                  (pagePrefetchCacheHierarchy.getL3().getMisses() +
                   pagePrefetchCacheHierarchy.getL3UsefulPrefetches())));

    printf("PC Prefetcher: L3 Misses: %lu, L2 Misses: %lu, L2 Accuracy: %.2f, L2 Coverage: "
           "%.2f, L3 Coverage %.2f\n",
           pcPrefetchCacheHierarchy.getL3().getMisses(),
           pcPrefetchCacheHierarchy.getL2().getMisses(),
           (pcPrefetchCacheHierarchy.getL2Prefetches() == 0
                ? 0.0
                : (1.0 * pcPrefetchCacheHierarchy.getL2UsefulPrefetches() /
                   pcPrefetchCacheHierarchy.getL2Prefetches())),
           pcPrefetchCacheHierarchy.getL2Prefetches() == 0
               ? 0.0
               : (1.0 * pcPrefetchCacheHierarchy.getL2UsefulPrefetches() /
                  (pcPrefetchCacheHierarchy.getL2().getMisses() +
                   pcPrefetchCacheHierarchy.getL2UsefulPrefetches())),
           pcPrefetchCacheHierarchy.getL3Prefetches() == 0
               ? 0.0
               : (1.0 * pcPrefetchCacheHierarchy.getL3UsefulPrefetches() /
                  (pcPrefetchCacheHierarchy.getL3().getMisses() +
                   pcPrefetchCacheHierarchy.getL3UsefulPrefetches())));

    return 0;
}
