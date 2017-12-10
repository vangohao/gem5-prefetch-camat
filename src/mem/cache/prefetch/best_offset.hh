#ifndef __MEM_CACHE_PREFETCH_DPC_BEST_OFFSET_HH__
#define __MEM_CACHE_PREFETCH_DPC_BEST_OFFSET_HH__

#include "mem/cache/prefetch/dpc_base.hh"
#include "params/BestOffsetPrefetcher.hh"

//####################################################################################o##
//                             PREFETCHER PARAMETERS
//######################################################################################

// Because prefetch cannot cross 4KB-page boundaries, there is no need to consider offsets
// greater than 63. However, with pages larger than 4KB, it would be beneficial to consider
// larger offsets.

#define RRINDEX 6
#define NOFFSETS 46

//#define DEFAULT_OFFSET 1

#define DELAYQSIZE 15



class BestOffsetPrefetcher : public DPCPrefetcher
{
public:
    BestOffsetPrefetcher(const BestOffsetPrefetcherParams *p);
private:
    using t_addr = long long;

//######################################################################################
//                               PREFETCHER STATE
//######################################################################################

    int prefetch_offset;   // 7 bits (6-bit value + 1 sign bit)
// TODO: change to none compile time const
// Recent Requests (RR) table: 2 banks, 64 entries per bank, RRTAG bits per entry
    int recent_request[2][1<<RRINDEX]; // 2x64x12 = 1536 bits

// 1 prefetch bit per L2 cache line : 256x8 = 2048 bits
    int prefetch_bit[L2_SET_COUNT][L2_ASSOCIATIVITY];


struct offsets_scores {
  int score[NOFFSETS];    // log2 SCORE_MAX = 5 bits per entry
  int max_score;          // log2 SCORE_MAX = 5 bits
  int best_offset;        // 7 bits (6-bit value + 1 sign bit)
  int round;              // log2 ROUND_MAX = 7 bits
  int p;                  // log2 NOFFSETS = 6 bits
};                     // 46x5+5+7+7+6 = 255 bits


struct delay_queue {
  int lineaddr[DELAYQSIZE]; // RRINDEX+RTAG = 18 bits
  int cycle[DELAYQSIZE];    // TIME_BITS = 12 bits
  int valid[DELAYQSIZE];    // 1 bit
  int tail;                 // log2 DELAYQSIZE = 4 bits
  int head;                 // log2 DELAYQSIZE = 4 bits
};                       // 15x(18+12+1)+4+4 = 473 bits


struct prefetch_throttle {
  int mshr_threshold;     // log2 L2_MSHR_COUNT = 4 bits
  int prefetch_score;     // log2 SCORE_MAX = 5 bits
  int llc_rate;           // log2 LLC_RATE_MAX = 8 bits
  int llc_rate_gauge;     // log2 GAUGE_MAX = 13 bits
  int last_cycle;         // TIME_BITS = 12 bits
};                     // 4+5+8+13+12 = 42 bits


    offsets_scores os;
    delay_queue dq;
    prefetch_throttle pt;

    void rr_init();
    int rr_tag(t_addr lineaddr);
    int rr_index_left(t_addr lineaddr);
    int rr_index_right(t_addr lineaddr);
    void rr_insert_left(t_addr lineaddr);
    void rr_insert_right(t_addr lineaddr);
    int rr_hit(t_addr lineaddr);
    void dq_init();
    void dq_push(t_addr lineaddr);
    int dq_ready();
    void dq_pop();
    void pt_init();
    void pt_update_mshr_threshold();
    void pt_llc_access();
    void os_reset();
    void os_learn_best_offset(t_addr lineaddr);
    int issue_prefetch(t_addr lineaddr, int offset);
protected:
    void l2_prefetcher_initialize(int cpu_num) override;
    void l2_prefetcher_operate(int cpu_num, unsigned long long int addr, unsigned long long int ip, int cache_hit) override;
    void l2_cache_fill(int cpu_num, unsigned long long int addr, int set, int way, int prefetch, unsigned long long int evicted_addr) override;
// Total prefetcher state: 7 + 1536 + 2048 + 255 + 473 + 42 = 4361 bits


    void l2_prefetcher_heartbeat_stats(int cpu_num) override;
    void l2_prefetcher_warmup_stats(int cpu_num) override;
    void l2_prefetcher_final_stats(int cpu_num) override;

};
#endif//__MEM_CACHE_PREFETCH_DPC_BEST_OFFSET_HH__
