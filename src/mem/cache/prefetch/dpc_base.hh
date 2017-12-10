#ifndef __MEM_CACHE_PREFETCH_DPC_HH__
#define __MEM_CACHE_PREFETCH_DPC_HH__
#include "mem/cache/prefetch/queued.hh"
#include "mem/cache/prefetch/dpc_interface.hh"
#include "params/DPCPrefetcher.hh"

#define FILL_L2 (1<<2)
#define FILL_LLC (1<<3)

#define CACHE_LINE_SIZE      64   // blkSize
#define PAGE_SIZE            4096 // pagebytes
#define L2_MSHR_COUNT        16   // cache->mshrQueue.
#define L2_READ_QUEUE_SIZE   32   //
#define L2_SET_COUNT         256  //(BaseSetAsso)  cache->tags->numSet
#define L2_ASSOCIATIVITY     8    //cache->tags-> assoc

class DPCPrefetcher : public QueuedPrefetcher, protected DPC_Interface
{
public:

    DPCPrefetcher(const DPCPrefetcherParams *p);
    void calculatePrefetch(const PacketPtr &pkt, std::vector<AddrPriority> &addresses) override;
    void setCache(BaseCache *_cache) override;

protected:
#if 0
    const unsigned CACHE_LINE_SIZE;  // blkSize;
    const Addr PAGE_SIZE;        // pagebytes
    const unsigned L2_MSHR_COUNT;    // cache->mshrQueue.
    const unsigned L2_READ_QUEUE_SIZE; // 32
    const unsigned L2_SET_COUNT;     //(BaseSetAsso)  cache->tags->numSet
    const unsigned L2_ASSOCIATIVITY; //cache->tags-> assoc
#endif

    int l2_get_set(unsigned long long int addr, bool is_secure=true);
    int l2_get_way(int cpu_num, unsigned long long int addr, int set, bool is_secure=true);

    unsigned long long int get_current_cycle(int cpu_num);
    int get_l2_mshr_occupancy(int cpu_num);
    int get_l2_read_queue_occupancy(int cpu_num);
    int l2_prefetch_line(int cpu_num, unsigned long long int base_addr, unsigned long long int pf_addr, int fill_level);

private:
    std::vector<AddrPriority> _addresses;

};

#endif //__MEM_CACHE_PREFETCH_DPC_HH__
