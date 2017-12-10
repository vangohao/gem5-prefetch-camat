#include "mem/cache/prefetch/dpc_base.hh"
#include "mem/cache/cache.hh"
#include "mem/cache/tags/base_set_assoc.hh"
#include "debug/HWPrefetch.hh"
#include <vector>

// Need to make sure BaseCache is at least type Cache


//#####################DPCPrefetcher###########################
DPCPrefetcher::DPCPrefetcher(const DPCPrefetcherParams *p) :
    QueuedPrefetcher(p)
#if 0
    ,CACHE_LINE_SIZE(blkSize),
    PAGE_SIZE(pageBytes),
    L2_MSHR_COUNT(((::Cache*)cache)->mshrQueue.numEntries),
    L2_READ_QUEUE_SIZE(32),
    L2_SET_COUNT(((BaseSetAssoc*)((::Cache*)cache)->tags)->numSets),
    L2_ASSOCIATIVITY(((BaseSetAssoc*)((::Cache*)cache)->tags)->assoc)
#endif
{


}

void DPCPrefetcher::setCache(BaseCache *_cache)
{
    QueuedPrefetcher::setCache(_cache);

    DPRINTF(HWPrefetch,"CACHE_LINE_SIZE = %d\n",(blkSize));
    DPRINTF(HWPrefetch,"PAGE_SIZE = %d\n",(pageBytes));
    DPRINTF(HWPrefetch,"L2_MSHR_COUNT = %d\n",(((::Cache*)cache)->mshrQueue.numEntries));
    DPRINTF(HWPrefetch,"L2_READ_QUEUE_SIZE = %d\n",(32));
    DPRINTF(HWPrefetch,"L2_SET_COUNT = %d\n",(((BaseSetAssoc*)((::Cache*)cache)->tags)->numSets));
    DPRINTF(HWPrefetch,"L2_ASSOCIATIVITY = %d\n",(((BaseSetAssoc*)((::Cache*)cache)->tags)->assoc));

    assert(CACHE_LINE_SIZE==(blkSize));
    assert(PAGE_SIZE==(pageBytes));
    assert(L2_MSHR_COUNT==(((::Cache*)cache)->mshrQueue.numEntries));
    assert(L2_READ_QUEUE_SIZE==(32));
    assert(L2_SET_COUNT==(((BaseSetAssoc*)((::Cache*)cache)->tags)->numSets));
    assert(L2_ASSOCIATIVITY==(((BaseSetAssoc*)((::Cache*)cache)->tags)->assoc));

    int cpu_num = 0;
    l2_prefetcher_initialize(cpu_num);
}

void DPCPrefetcher::calculatePrefetch(const PacketPtr &pkt, std::vector<AddrPriority> &addresses)
{
    if (!pkt->req->hasPC()) {
        DPRINTF(HWPrefetch, "Ignoring request with no PC.\n");
        return;
    }
    int cpu_num = 0; //    uint32_t core_id = pkt->req->hasContextId() ? pkt->req->contextId() : -1;           ?
    Addr pkt_addr = pkt->getAddr();
    bool is_secure = pkt->isSecure();
    l2_prefetcher_operate(cpu_num, pkt->getAddr(), pkt->req->getPC(), inCache(pkt_addr, is_secure)||inMissQueue(pkt_addr, is_secure));
    //DPRINTF(HWPrefetch,"queue size before : %d\n ", addresses.size());
    addresses.insert(addresses.end(), _addresses.begin(), _addresses.end());
    //DPRINTF(HWPrefetch,"queue size after : %d\n ", addresses.size());
    _addresses.clear();
}

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
//    DPRINTF(HWPrefetch,"Prefetching %#x, now %d queueing\n", pf_addr, _addresses.size());
    return 1;
}

unsigned long long int DPCPrefetcher::get_current_cycle(int cpu_num)
{
    return curTick();
}


//---------------------DPCPrefetcher---------------------------
