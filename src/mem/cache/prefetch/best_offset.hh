/*
 * Copyright (c) 2012-2013, 2015 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Ron Dreslinski
 */

/**
 * @file
 * Describes a strided prefetcher.
 */

#ifndef __MEM_CACHE_PREFETCH_BO_HH__
#define __MEM_CACHE_PREFETCH_BO_HH__

#define RRINDEX 6
#define L2_SET_COUNT 256
#define L2_ASSOCIATIVITY 8

#include <unordered_map>

#include "mem/cache/prefetch/queued.hh"
#include "params/StridePrefetcher.hh"

class BestOffsetPrefetcher : public QueuedPrefetcher
{
//######################################################################################
//                               PREFETCHER STATE
//######################################################################################
protected:
int prefetch_offset;   // 7 bits (6-bit value + 1 sign bit)

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
} os;                     // 46x5+5+7+7+6 = 255 bits


struct delay_queue {
  int lineaddr[DELAYQSIZE]; // RRINDEX+RTAG = 18 bits
  int cycle[DELAYQSIZE];    // TIME_BITS = 12 bits
  int valid[DELAYQSIZE];    // 1 bit
  int tail;                 // log2 DELAYQSIZE = 4 bits
  int head;                 // log2 DELAYQSIZE = 4 bits
} dq;                       // 15x(18+12+1)+4+4 = 473 bits


struct prefetch_throttle {
  int mshr_threshold;     // log2 L2_MSHR_COUNT = 4 bits
  int prefetch_score;     // log2 SCORE_MAX = 5 bits
  int llc_rate;           // log2 LLC_RATE_MAX = 8 bits
  int llc_rate_gauge;     // log2 GAUGE_MAX = 13 bits
  int last_cycle;         // TIME_BITS = 12 bits
} pt;                     // 4+5+8+13+12 = 42 bits

// Total prefetcher state: 7 + 1536 + 2048 + 255 + 473 + 42 = 4361 bits

    typedef long long t_addr;

    void rr_init();
    int rr_tag(t_addr lineaddr);
    int rr_index_left(t_addr lineaddr);
    int rr_index_right(t_addr lineaddr);
    void rr_insert_left(t_addr lineaddr);
    void rr_insert_right(t_addr lineaddr);
    int rr_hit(t_addr lineaddr);

//######################################################################################
//                               DELAY QUEUE (DQ)
//######################################################################################
    void dq_init();
    void dq_push(t_addr lineaddr);
    int dq_ready();
    void dq_pop();

//######################################################################################
//                               PREFETCH THROTTLE (PT)
//######################################################################################
    void pt_init();
    void pt_update_mshr_threshold();
    void pt_llc_access();

//######################################################################################
//                               OFFSETS SCORES (OS)
//######################################################################################
    void os_reset();
    void os_learn_best_offset(t_addr lineaddr);

//######################################################################################
//                               OFFSET PREFETCHER
//######################################################################################
    int issue_prefetch(t_addr lineaddr, int offset);


//######################################################################################
//                               DPC2 INTERFACE
//######################################################################################
    void l2_prefetcher_initialize(int cpu_num);
    void l2_prefetcher_operate(int cpu_num, unsigned long long int addr, unsigned long long int ip, int cache_hit);
    void l2_cache_fill(int cpu_num, unsigned long long int addr, int set, int way, int prefetch, unsigned long long int evicted_addr);

public:
    void calculatePrefetch(const PacketPtr &pkt,
                           std::vector<AddrPriority> &addresses);



    #if 0
protected:
    const int maxConf;
    const int threshConf;
    const int minConf;
    const int startConf;

    const int pcTableAssoc;
    const int pcTableSets;

    const bool useMasterId;

    const int degree;

    struct StrideEntry
    {
        StrideEntry() : instAddr(0), lastAddr(0), isSecure(false), stride(0),
                        confidence(0)
        { }

        Addr instAddr;
        Addr lastAddr;
        bool isSecure;
        int stride;
        int confidence;
    };

    class PCTable
    {
      public:
        PCTable(int assoc, int sets, const std::string name) :
            pcTableAssoc(assoc), pcTableSets(sets), _name(name) {}
        StrideEntry** operator[] (int context) {
            auto it = entries.find(context);
            if (it != entries.end())
                return it->second;

            return allocateNewContext(context);
        }

        ~PCTable();
      private:
        const std::string name() {return _name; }
        const int pcTableAssoc;
        const int pcTableSets;
        const std::string _name;
        std::unordered_map<int, StrideEntry**> entries;

        StrideEntry** allocateNewContext(int context);
    };
    PCTable pcTable;

    bool pcTableHit(Addr pc, bool is_secure, int master_id, StrideEntry* &entry);
    StrideEntry* pcTableVictim(Addr pc, int master_id);

    Addr pcHash(Addr pc) const;
  public:

    StridePrefetcher(const StridePrefetcherParams *p);

    void calculatePrefetch(const PacketPtr &pkt,

                           std::vector<AddrPriority> &addresses);
    #endif
};

#endif // __MEM_CACHE_PREFETCH_BO_HH__
