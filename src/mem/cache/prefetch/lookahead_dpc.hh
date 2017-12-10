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

#ifndef __MEM_CACHE_PREFETCH_LOOKAHEAD_HH__
#define __MEM_CACHE_PREFETCH_LOOKAHEAD_HH__

//#include <unordered_map>

#include "mem/cache/prefetch/queued.hh"
#include "params/LookaheadPrefetcher.hh"


// Signature table configuration
#define ST_SET 512
#define ST_WAY 2
#define SIG_LENGTH 12
#define PT_SET ( 1 << (SIG_LENGTH) )

#define MAX_ENTRY 4
#define HISTORY 4
#define BLOCK_SIZE 64

// Filter configuration
#define FILTER_SET 256
#define FILTER_WAY 2

#define PRINTF_TRACK 2048

class LookaheadPrefetcher : public QueuedPrefetcher
{


protected:
    const unsigned lPageSize;

    Addr pageTag(Addr a) const;

    class SPP_class {
    private:
        const std::string name() {return _name; }
        std::string _name;
    public:
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

        void setName(std::string name){_name = name;}

        LookaheadPrefetcher* owner;


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
#if 0
        // PE stage
        bool check_filter(int tag, int block, int type);
#endif
    };

    SPP_class ST[ST_SET];          // Signature table
    SPP_class PT[PT_SET];          // Pattern table
//    SPP_class FILTER[FILTER_SET];  // Filter

// Statistics


    int PF_attempt, PF_local, PF_beyond;//, PF_filtered;
#if 0
    int PF_issue=0, PF_inflight=0, PF_fail=0;
    int PF_L2_attempt=0, PF_L2_issue=0, PF_L2_fail=0;
    int PF_LLC_attempt=0,PF_LLC_issue=0, PF_LLC_fail=0;
    int AUX_attempt=0, AUX_local=0, AUX_beyond=0, AUX_filtered=0;
    int AUX_issue=0, AUX_inflight=0, AUX_fail=0;
    int AUX_L2_attempt=0, AUX_L2_issue=0, AUX_L2_fail=0;
    int AUX_LLC_attempt=0, AUX_LLC_issue=0, AUX_LLC_fail=0;

#endif
    int LA_attempt, LA_prefetch;
    int LA_depth[PRINTF_TRACK];
    int PT_access, PT_hit, PT_miss;
    int ST_access, ST_hit, ST_miss;
    int l2_hit, l2_miss, l2_access;

    int depth;
#if 0
    int remain_mshr=0, remain_rq=0;
#endif



public:
    LookaheadPrefetcher(const LookaheadPrefetcherParams *p);
    void calculatePrefetch(const PacketPtr &pkt,
                           std::vector<AddrPriority> &addresses);
protected:
    void l2_prefetcher_operate(Addr addr, Addr ip, int cache_hit,std::vector<AddrPriority> &addresses);
    void l2_prefetcher_initialize();


    // Prefetch function
    void attempt_prefetch(Addr base_addr, int base_block, int PT_idx, int la_candidate, int pf_candidate, std::vector<AddrPriority> &addresses);

// Lookahead function
    void lookahead(Addr base_addr, int base_block, int prev_conf, int PT_idx, int la_candidate, std::vector<AddrPriority> &addresses);
#if 0
// Auxiliary next line prefetcher
    void aux_prefetch(Addr base_addr);




#endif
};

#endif // __MEM_CACHE_PREFETCH_LOOKAHEAD_HH__
