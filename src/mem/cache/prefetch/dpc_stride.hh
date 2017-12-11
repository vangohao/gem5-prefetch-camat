#ifndef __MEM_CACHE_PREFETCH_DPC_STRIDE_HH__
#define __MEM_CACHE_PREFETCH_DPC_STRIDE_HH__

#include "mem/cache/prefetch/dpc_base.hh"
#include "params/Stride_DPCPrefetcher.hh"

#define IP_TRACKER_COUNT 1024

class Stride_DPCPrefetcher : public DPCPrefetcher
{

private:
    struct ip_tracker
    {
        // the IP we're tracking
        unsigned long long int ip;

        // the last address accessed by this IP
        unsigned long long int last_addr;
        // the stride between the last two addresses accessed by this IP
        long long int last_stride;

        // use LRU to evict old IP trackers
        unsigned long long int lru_cycle;
    };
    ip_tracker trackers[IP_TRACKER_COUNT];
public:
    Stride_DPCPrefetcher(const Stride_DPCPrefetcherParams *p);

protected:
    void l2_prefetcher_initialize(int cpu_num) override;
    void l2_prefetcher_operate(int cpu_num, unsigned long long int addr, unsigned long long int ip, int cache_hit) override;
    void l2_cache_fill(int cpu_num, unsigned long long int addr, int set, int way, int prefetch, unsigned long long int evicted_addr) override;
// Total prefetcher state: 7 + 1536 + 2048 + 255 + 473 + 42 = 4361 bits


    void l2_prefetcher_heartbeat_stats(int cpu_num) override;
    void l2_prefetcher_warmup_stats(int cpu_num) override;
    void l2_prefetcher_final_stats(int cpu_num) override;

};
#endif//__MEM_CACHE_PREFETCH_DPC_STRIDE_HH__
