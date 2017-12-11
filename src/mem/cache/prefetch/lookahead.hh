#ifndef __MEM_CACHE_PREFETCH_DPC_LOOKAHEAD_HH__
#define __MEM_CACHE_PREFETCH_DPC_LOOKAHEAD_HH__

#include "mem/cache/prefetch/dpc_base.hh"
#include "params/LookaheadPrefetcher.hh"

#define SIG_LENGTH 12

#define MAX_ENTRY 4
#define BLOCK_SIZE 64
#define HISTORY 4
#define ST_SET 512
#define PT_SET ( 1 << (SIG_LENGTH) )

// Filter configuration
#define FILTER_SET 256
#define FILTER_WAY 2
#define PRINTF_TRACK 2048

class LookaheadPrefetcher : public DPCPrefetcher
{

private:
    class SPP_class {
        friend LookaheadPrefetcher;
    public:
        const std::string name() {return _name;}
        void setName(std::string name){_name = name;}
    private:
        LookaheadPrefetcher* owner;
        std::string _name;
//    public:
        ////////////////////////////////// Components
        // Common
        bool valid[MAX_ENTRY];
        int p_tag[MAX_ENTRY];
        int lru[MAX_ENTRY];

        // Signature Table (ST)
        int signature[MAX_ENTRY];
        int last_block[MAX_ENTRY];

        // Pattern Table (PT)
        int counter[MAX_ENTRY];
        int max_idx;

        // Prefetch Engine (PE)
        int filter_bitmap[FILTER_WAY][BLOCK_SIZE];

        // DEBUG
        int idx;
        int delta[MAX_ENTRY][HISTORY];
        int new_delta;
        int pf_candidate, la_candidate;

        /////////////////////////////////// Functions
        // Common
        SPP_class();
        void update_lru(int match_idx, bool invalid, int type);

        // ST stage
        int  access_ST(int tag, int curr_block, int type);
        int  update_signature(int assoc, int type);

        // PT stage
        void update_PT(int tag);
        int  read_PT(int conf, int la_depth);
        void update_counter(int match_idx, bool miss);

        // PE stage
        bool check_filter(int tag, int block, int type);
    };
    friend SPP_class;

    SPP_class ST[ST_SET];          // Signature table
    SPP_class PT[PT_SET];          // Pattern table
    SPP_class FILTER[FILTER_SET];  // Filter

// Statistics
    int ST_access, ST_hit, ST_miss;
    int PT_access, PT_hit, PT_miss;
    int PF_attempt, PF_local, PF_beyond, PF_filtered;
    int PF_issue, PF_inflight, PF_fail;
    int PF_L2_attempt, PF_L2_issue, PF_L2_fail;
    int PF_LLC_attempt,PF_LLC_issue, PF_LLC_fail;
    int AUX_attempt, AUX_local, AUX_beyond, AUX_filtered;
    int AUX_issue, AUX_inflight, AUX_fail;
    int AUX_L2_attempt, AUX_L2_issue, AUX_L2_fail;
    int AUX_LLC_attempt, AUX_LLC_issue, AUX_LLC_fail;
    int LA_attempt, LA_prefetch;
    int LA_depth[PRINTF_TRACK];
    int depth;
    int l2_hit, l2_miss, l2_access;
    int remain_mshr, remain_rq;

// Prefetch function
    void attempt_prefetch(Addr base_addr, int base_block, int PT_idx, int la_candidate, int pf_candidate);

// Lookahead function
    void lookahead(Addr base_addr, int base_block, int prev_conf, int PT_idx, int la_candidate);

// Auxiliary next line prefetcher
    void aux_prefetch(Addr base_addr);


public:
    LookaheadPrefetcher(const LookaheadPrefetcherParams *p);
protected:
    void l2_prefetcher_initialize(int cpu_num) override;
    void l2_prefetcher_operate(int cpu_num, unsigned long long int addr, unsigned long long int ip, int cache_hit) override;
    void l2_cache_fill(int cpu_num, unsigned long long int addr, int set, int way, int prefetch, unsigned long long int evicted_addr) override;

    void l2_prefetcher_heartbeat_stats(int cpu_num) override;
    void l2_prefetcher_warmup_stats(int cpu_num) override;
    void l2_prefetcher_final_stats(int cpu_num) override;

};
#endif//__MEM_CACHE_PREFETCH_DPC_LOOKAHEAD_HH__
