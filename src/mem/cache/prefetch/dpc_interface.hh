#ifndef __DPC_INTERFACE_HH__
#define __DPC_INTERFACE_HH__
class DPC_Interface
{
    virtual void l2_prefetcher_initialize(int cpu_num) = 0;
    virtual int l2_get_set(unsigned long long int addr) = 0;
    virtual void l2_cache_fill(int cpu_num, unsigned long long int addr, int set, int way, int prefetch, unsigned long long int evicted_addr) = 0;
    virtual void l2_prefetcher_heartbeat_stats(int cpu_num) = 0;
    virtual void l2_prefetcher_warmup_stats(int cpu_num) = 0;
    virtual void l2_prefetcher_final_stats(int cpu_num) = 0;
};
#endif//__DPC_INTERFACE_HH__
