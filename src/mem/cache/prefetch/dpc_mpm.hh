#ifndef __MEM_CACHE_PREFETCH_DPC_MPM_HH__
#define __MEM_CACHE_PREFETCH_DPC_MPM_HH__

#include "mem/cache/prefetch/dpc_base.hh"
#include "params/MPM_DPCPrefetcher.hh"


#define AMPM_PAGE_COUNT 64
class MPM_DPCPrefetcher : public DPCPrefetcher
{

private:
     struct ampm_page
    {
        // page address
        unsigned long long int page;

        // The access map itself.
        // Each element is set when the corresponding cache line is accessed.
        // The whole structure is analyzed to make prefetching decisions.
        // While this is coded as an integer array, it is used conceptually as a single 64-bit vector.
        int access_map[64];

        // This map represents cache lines in this page that have already been prefetched.
        // We will only prefetch lines that haven't already been either demand accessed or prefetched.
        int pf_map[64];

        // used for page replacement
        unsigned long long int lru;
    };

    ampm_page ampm_pages[AMPM_PAGE_COUNT];

public:
    MPM_DPCPrefetcher(const MPM_DPCPrefetcherParams *p);

protected:
    void l2_prefetcher_initialize(int cpu_num) override;
    void l2_prefetcher_operate(int cpu_num, unsigned long long int addr, unsigned long long int ip, int cache_hit) override;
    void l2_cache_fill(int cpu_num, unsigned long long int addr, int set, int way, int prefetch, unsigned long long int evicted_addr) override;
// Total prefetcher state: 7 + 1536 + 2048 + 255 + 473 + 42 = 4361 bits


    void l2_prefetcher_heartbeat_stats(int cpu_num) override;
    void l2_prefetcher_warmup_stats(int cpu_num) override;
    void l2_prefetcher_final_stats(int cpu_num) override;

};
#endif//__MEM_CACHE_PREFETCH_DPC_MPM_HH__
