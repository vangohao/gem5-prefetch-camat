#include <vector>

#include "debug/HWPrefetch.hh"
#include "mem/cache/cache.hh"
#include "mem/cache/prefetch/dpc_base.hh"
#include "mem/cache/tags/base_set_assoc.hh"

// Need to make sure BaseCache is at least type Cache


//#####################DPCPrefetcher###########################
DPCPrefetcher::DPCPrefetcher(const DPCPrefetcherParams *p) :
    QueuedPrefetcher(p),
    CACHE_LINE_SIZE(blkSize),  // blkSize;
    PAGE_SIZE(pageBytes),        // pagebytes
    L2_MSHR_COUNT(((::Cache*)cache)->mshrQueue.numEntries),    // cache->mshrQueue.
    L2_READ_QUEUE_SIZE(32), // 32
    L2_SET_COUNT(((BaseSetAssoc*)((::Cache*)cache)->tags)->numSets),     //(BaseSetAsso)  cache->tags->numSet
    L2_ASSOCIATIVITY(((BaseSetAssoc*)((::Cache*)cache)->tags)->assoc) //cache->tags-> assoc
{
    DPRINTF(HWPrefetch, "DPCPrefetcher%d",queueSize);
}

void DPCPrefetcher::calculatePrefetch(const PacketPtr &pkt, std::vector<AddrPriority> &addresses)
{}
int
DPCPrefetcher::l2_get_set(unsigned long long int addr, bool is_secure)
{
    const auto blk = ((::Cache *)cache)->findBlock(addr, is_secure);
    if (!blk)
        return -1;
    return blk->set;
    //return ((::Cache *)cache)->tags->extractSet(addr);
}

// Need to make sure Tag is at least BaseSetAsscoc
int
DPCPrefetcher::l2_get_way(int cpu_num, unsigned long long int addr, int set, bool is_secure)
{
    const auto blk = ((::Cache *)cache)->findBlock(addr, is_secure);
    if (!blk)
        return -1;
    if (blk->set != set)
        return -1;
    return blk->way;
}

int
DPCPrefetcher::get_l2_mshr_occupancy(int cpu_num)
{
    return ((::Cache *)cache)->mshrQueue.numInService();
}


int
DPCPrefetcher::get_l2_read_queue_occupancy(int cpu_num)
{
    // assume inf queue for now
    return 0;
}

int
DPCPrefetcher::l2_prefetch_line(int cpu_num, unsigned long long int base_addr, unsigned long long int pf_addr, int fill_level)
{
    // need to check not cross page
    // returns 1 all the time for now.
    _addresses.push_back(AddrPriority(pf_addr, 0));
    return 1;
}

#if 0
unsigned long long int DPCPrefetcher::get_current_cycle(int cpu_num)
{
    return 0;
}
#endif

//---------------------DPCPrefetcher---------------------------
