//
// Data Prefetching Championship Simulator 2
// Seth Pugsley, seth.h.pugsley@intel.com
//

/*
 * #002 Lookahead Prefetching with Signature Path
 *
 * Jinchun Kim, cienlux@tamu.edu
 * Paul V. Gratz, pgratz@gratz1.com
 * A. L. Narasimha Reddy, reddy@tamu.edu
 * Department of Electrical and Computer Engineering
 * Texas A&M University
 *
 * Compile command: g++ -Wall -o dpc2sim example_prefetchers/002_kim.cpp lib/dpc2sim.a
 *
 * NOTE: Use #define DEBUG to print out detailed info
 */

#include "mem/cache/prefetch/lookahead.hh"
#include "debug/HWPrefetch.hh"
#include <cstdio>
#include <cstdlib>
#include <assert.h>

//#define DEBUG
#define FILTER_ON
#define LOOKAHEAD_ON
#define AUX_ON

// Signature table configuration

#define ST_WAY 2
#define ST_TAG_BIT 8
#define SIG_SHIFT  3

// Pattern table configuration

#define PT_WAY 4
#define PT_TAG_BIT 7
#define COUNTER_BIT 3



// Useful numbers
#define PRIME1 509
#define PRIME2 257
#define PRIME3 251



#define MAX_INFLIGHT 4
#define MAX_CONFIDENCE 95
#define PF_THRESHOLD 50
#define LA_THRESHOLD 75
#define MSHR_THRESHOLD 8
#define RQ_THRESHOLD 16


enum {READ, UPDATE, DEMAND, PREFETCH, EVICT, FILL, CHECK, FROM_ST, FROM_PT, FROM_FILTER};
//typedef unsigned long long int Addr;


LookaheadPrefetcher::LookaheadPrefetcher(const LookaheadPrefetcherParams *p) :
    DPCPrefetcher(p),
    // Statistics
    ST_access(0), ST_hit(0), ST_miss(0),
    PT_access(0), PT_hit(0), PT_miss(0),
    PF_attempt(0), PF_local(0), PF_beyond(0), PF_filtered(0),
    PF_issue(0), PF_inflight(0), PF_fail(0),
    PF_L2_attempt(0), PF_L2_issue(0), PF_L2_fail(0),
    PF_LLC_attempt(0),PF_LLC_issue(0), PF_LLC_fail(0),
    AUX_attempt(0), AUX_local(0), AUX_beyond(0), AUX_filtered(0),
    AUX_issue(0), AUX_inflight(0), AUX_fail(0),
    AUX_L2_attempt(0), AUX_L2_issue(0), AUX_L2_fail(0),
    AUX_LLC_attempt(0), AUX_LLC_issue(0), AUX_LLC_fail(0),
    LA_attempt(0), LA_prefetch(0),
    depth(0),
    l2_hit(0), l2_miss(0), l2_access(0),
    remain_mshr(0), remain_rq(0)
{
    for (int i=0; i<PRINTF_TRACK; i++)
        LA_depth[i] = 0;
    for (int i=0; i<ST_SET; i++)
    {
        ST[i].owner = this;
        ST[i].setName(name()+".ST["+std::to_string(i)+"]");
    }

    for (int i=0; i<PT_SET; i++)
    {
        PT[i].owner = this;
        PT[i].setName(name()+".PT["+std::to_string(i)+"]");
    }

    for (int i=0; i<FILTER_SET; i++)
    {
        FILTER[i].owner = this;
        FILTER[i].setName(name()+".FILTER["+std::to_string(i)+"]");
    }

}

LookaheadPrefetcher*
LookaheadPrefetcherParams::create()
{
    return new LookaheadPrefetcher(this);
}

void LookaheadPrefetcher::l2_prefetcher_initialize(int cpu_num)
{
    // you can inspect these knob values from your code to see which configuration you're runnig in
//    printf("Knobs visible from prefetcher: %d %d %d\n", knob_scramble_loads, knob_small_llc, knob_low_bandwidth);

	// Used to measure average lookahead depth
	for (int i=0; i<PRINTF_TRACK; i++)
		LA_depth[i] = 0;

	printf("\nSignature Path Prefetching Configuration\n");
	printf("\tSignature Table\n");
	printf("\tSet: %4d\n", ST_SET);
	printf("\tWay: %4d\n", ST_WAY);
	printf("\tPattern Table\n");
	printf("\tSet: %4d\n", PT_SET);
	printf("\tWay: %4d\n", PT_WAY);
	printf("\tFilter\n");
	printf("\tSet: %4d\n", FILTER_SET);
	printf("\tWay: %4d\n", FILTER_WAY);
}

void LookaheadPrefetcher::l2_prefetcher_operate(int cpu_num, unsigned long long int addr, unsigned long long int ip, int cache_hit)
{
    // uncomment this line to see all the information available to make prefetch decisions
    //printf("(0x%llx 0x%llx %d %d %d) ", addr, ip, cache_hit, get_l2_read_queue_occupancy(0), get_l2_mshr_occupancy(0));
//    int i, j, total=0;

    Addr curr_page = (addr >> 12);
    int curr_offset = addr & 0xFFF,
    	curr_block = curr_offset/BLOCK_SIZE,
		ST_idx = curr_page % PRIME1,
		partial_pnum = curr_page & ((1 << ST_TAG_BIT) - 1),
		PT_update_idx = 0, PT_pref_idx = 0,
		la_candidate = 0, pf_candidate = 0,
		init_conf = 0;

	int FILTER_idx = curr_page % PRIME2;

	if (FILTER_idx >= 255)
		FILTER_idx = curr_page % PRIME3;

	// Remain L2 resources
	remain_mshr = 16 - get_l2_mshr_occupancy(0);
	remain_rq = 32 - get_l2_read_queue_occupancy(0);

    DPRINTF(HWPrefetch,"\ncycle: %llu ip: %llx  page: %llx ST_idx: %3d block: %2d hit: %d\n",
				get_current_cycle(0), ip, curr_page, ST_idx, curr_block, cache_hit);

	l2_hit += cache_hit;
	l2_access++;

	// Set demand block in filter
	// This demand might be already requested by prefetcher and set to "1" by the filter
	// However, we cannot drop demand request since participants don't have control over demand request
	FILTER[FILTER_idx].check_filter(partial_pnum, curr_block, DEMAND);

	// 1) Signature table stage
	ST[ST_idx].idx = ST_idx; // DEBUG

	// 1-1) Read signature and get PT_update_idx (used for PT update)
	PT_update_idx = ST[ST_idx].access_ST(partial_pnum, curr_block, READ);

	// 1-2) Update history signature and get PT_pref_index (used for prefetching)
	PT_pref_idx = ST[ST_idx].access_ST(partial_pnum, curr_block, UPDATE);

	// 2) Pattern table Stage
	PT[PT_pref_idx].idx = PT_pref_idx; // DEBUG

	// 2-1) Update PT with PT_update_idx
	// PT is not updated if stride is zero
	if ((PT_update_idx < PT_SET) && (ST[ST_idx].new_delta != 0))
		PT[PT_update_idx].update_PT(ST[ST_idx].new_delta);

	// 2-2) Read prefetch and lookahead candidates from PT
	if (PT_pref_idx > PT_SET) // Do not request prefetch when stride is zero
		return;
	init_conf = PT[PT_pref_idx].read_PT(100, 0);
	la_candidate = PT[PT_pref_idx].la_candidate;
	pf_candidate = PT[PT_pref_idx].pf_candidate;

	// 3) Prefetch engine stage
	// 3-1) Prefetch if there is possible candidate with enough confidence
	PF_inflight = 0;
	depth = 0;

	if (pf_candidate) // Prefetch with SPP
		attempt_prefetch(addr, curr_block, PT_pref_idx, la_candidate, pf_candidate);

#ifdef LOOKAHEAD_ON
	// 3-2) Lookahead
	if (la_candidate)
		lookahead(addr, curr_block, init_conf, PT_pref_idx, la_candidate);
	else
		DPRINTF(HWPrefetch,"\t[LOOKAHEAD] No more lookahead! next_la_candidate: %d\n", la_candidate);
#endif

	if (PF_inflight >= MAX_INFLIGHT) {
		DPRINTF(HWPrefetch,"\t[PREFETCH] PF_inflight reached threshold %d!\n", MAX_INFLIGHT);
		return;
	}

#ifdef AUX_ON
	// Auxiliary next line prefetcher
	if ((pf_candidate == 0) && (la_candidate == 0)) { // No prefetch and lookahead candidate
		DPRINTF(HWPrefetch,"\t[PT-READ] No pf_candidate: %d curr_block: %d\n", pf_candidate, curr_block);

		// Do not activate AUX across the page boundary
		if ((curr_block+1) > (BLOCK_SIZE - 1)) {
			DPRINTF(HWPrefetch,"\t[PREFETCH-AUX] Out of same page boundary!\n");
			AUX_beyond++;
			return;
		}

		// Do not activate AUX if the next line has been already demanded or prefetched
		if (!FILTER[FILTER_idx].check_filter(partial_pnum, curr_block+1, PREFETCH)) {
			AUX_filtered++; AUX_attempt++;
			return;
		}
		else // Call auxiliary next line prefetcher
			aux_prefetch(addr);
	}
#endif
	return;
}

void LookaheadPrefetcher::l2_cache_fill(int cpu_num, unsigned long long int addr, int set, int way, int prefetch, unsigned long long int evicted_addr)
{
    // uncomment this line to see the information available to you when there is a cache fill event
    //printf("0x%llx %d %d %d 0x%llx\n", addr, set, way, prefetch, evicted_addr);

	// L2 FILL
	Addr curr_page = addr >> 12;
	int curr_block = (addr >> 6) & 0x3F,
	    FILTER_idx = curr_page % PRIME2,
		partial_pnum = curr_page & ((1 << ST_TAG_BIT) - 1);

	if (FILTER_idx >= 255)
		FILTER_idx = curr_page % PRIME3;

	DPRINTF(HWPrefetch,"cycle: %llu [L2-FILL] page: %llx FILTER_idx: %3d block: %3d prefetch: %d set: %4d way: %d\n",
				get_current_cycle(0), curr_page, FILTER_idx, curr_block, prefetch, set, way);
	FILTER[FILTER_idx].check_filter(partial_pnum, curr_block, FILL);

	// L2 EVICT
	if (evicted_addr) {
		curr_page = evicted_addr >> 12;
		curr_block = (evicted_addr >> 6) & 0x3F;
		FILTER_idx = curr_page % PRIME2;
		partial_pnum = curr_page & ((1 << ST_TAG_BIT) - 1);

		if (FILTER_idx >= 255)
			FILTER_idx = curr_page % PRIME3;

		DPRINTF(HWPrefetch,"cycle: %llu [L2-EVICT] page: %llx FILTER_idx: %3d block: %3d set: %4d way: %d\n",
					get_current_cycle(0), curr_page, FILTER_idx, curr_block, set , way);
		FILTER[FILTER_idx].check_filter(partial_pnum, curr_block, EVICT);
	}
}

void LookaheadPrefetcher::l2_prefetcher_heartbeat_stats(int cpu_num)
{
  printf("Prefetcher heartbeat stats\n");
}

void LookaheadPrefetcher::l2_prefetcher_warmup_stats(int cpu_num)
{
	printf("Prefetcher warmup complete stats\n\n");
}

void LookaheadPrefetcher::l2_prefetcher_final_stats(int cpu_num)
{
	printf("Prefetcher final stats\n");

	int sum_depth = 0, num_total = 0, avg_depth = 0;
	for (int i=1; i<PRINTF_TRACK; i++) {
		num_total += LA_depth[i];
		sum_depth += i*LA_depth[i];
	}
	if (num_total)
		avg_depth = sum_depth/num_total;

	printf("[ST]  Hit: %d  Miss: %d  Access: %d  Hit_rate: %2d%%\n",
		  ST_hit, ST_miss, ST_access, (ST_hit*100)/ST_access);
	printf("[PT]  Hit: %d  Miss: %d  Access: %d  Hit_rate: %2d%%\n",
		  PT_hit, PT_miss, PT_access, (PT_hit*100)/PT_access);
	printf("[PF_attempt]: %d  local: %d  beyond: %d  PF_issue: %d  PF_filtered: %d PF_fail: %d\n",
			PF_attempt, PF_local, PF_beyond, PF_issue, PF_filtered, PF_fail);
	printf("[PF_L2_attempt]: %d  PF_L2_issue: %d  PF_L2_fail: %d\n", PF_L2_attempt, PF_L2_issue, PF_L2_fail);
	printf("[PF_LLC_attempt]: %d  PF_LLC_issue: %d  PF_LLC_fail: %d\n", PF_LLC_attempt, PF_LLC_issue, PF_LLC_fail);
	printf("[AUX_attempt]: %d  local: %d  beyond: %d  PF_issue: %d  PF_filtered: %d PF_fail: %d\n",
			AUX_attempt, AUX_local, AUX_beyond, AUX_issue, AUX_filtered, AUX_fail);
	printf("[AUX_L2_attempt]: %d  AUX_L2_issue: %d  AUX_L2_fail: %d\n", AUX_L2_attempt, AUX_L2_issue, AUX_L2_fail);
	printf("[AUX_LLC_attempt]: %d  AUX_LLC_issue: %d  AUX_LLC_fail: %d\n", AUX_LLC_attempt, AUX_LLC_issue, AUX_LLC_fail);
	printf("[LA_attempt]: %d  LA_prefetch: %d  AVG_LA_depth: %d\n",
			LA_attempt, LA_prefetch, avg_depth);
	printf("[L2_cache]  Hit: %d  Miss: %d  Access: %d\n", l2_hit, l2_access - l2_hit, l2_access);
}

// Class constructor
LookaheadPrefetcher::SPP_class::SPP_class()
{
	idx = 0;
	new_delta = 0;
	pf_candidate = 0;
	la_candidate = 0;
	max_idx = 0;

	for (int i=0; i<MAX_ENTRY; i++) {
		p_tag[i] = 0;
		valid[i] = false;
		last_block[i] = 0;
		lru[i] = 0;
		counter[i] = 0;
		signature[i] = 0;
		for (int j=0; j<HISTORY; j++)
			delta[i][j] = 0;
    }

    for (int i=0; i<FILTER_WAY; i++)
    for (int j=0; j<BLOCK_SIZE; j++)
        filter_bitmap[i][j] = 0;

}

// Return history signature
int LookaheadPrefetcher::SPP_class::access_ST(int tag, int curr_block, int type)
{
    int i, match_idx = -1;
	int min_lru = ST_WAY+1;
	for (i=0; i<ST_WAY; i++) {
		if (valid[i] && (p_tag[i] == tag)) { // Hit: Matching page

			// Do not update signature if stride is zero
			if (curr_block == last_block[i])
				return PT_SET+1;

			if (type == READ) // Return PT_update_idx before updating ST
				return update_signature(i, type);

			new_delta = curr_block - last_block[i];
			DPRINTF(HWPrefetch,"\t[ST-UPDATE] Index: %4x ST-Hit[%d]: %3x (partial_pnum) curr_block: %d last_block[%d]: %d\n",
						idx, i, tag, curr_block, i, last_block[i]);

			// Update ST
			last_block[i] = curr_block;
			SPP_class::update_lru(i, false, FROM_ST);
			owner->ST_hit++; owner->ST_access++;
			return update_signature(i, type);
		}
		else if (valid[i] == false) { // Invalid: First access to this page
			if (type == READ) {
				// No valid signature or stride yet
				// Use current block (first block accessed in this page) as an initial signature
				// Actual update is done by access_ST(UPDATE)
				return curr_block;
			}

			// Update ST
			valid[i] = true;
			p_tag[i] = tag;
			last_block[i] = curr_block;
			DPRINTF(HWPrefetch,"\t[ST-UPDATE] Index: %4x ST-Invalid[%d]: %x (partial_pnum) curr_block: %d last_block[%d]: %d\n",
						idx, i, tag, curr_block, i, last_block[i]);
			delta[match_idx][HISTORY-1] = curr_block; // DEBUG
			SPP_class::update_lru(i, true, FROM_ST);
			new_delta = 1; // Assume +1 stride. This +1 stride is not used until it gets confident in PT
			signature[i] = curr_block; // Initial signature is set to the first block used in this page
			owner->ST_miss++; owner->ST_access++;
			return curr_block;
		}
		if (lru[i] < min_lru) { // Track LRU
			match_idx = i;
			min_lru = lru[i];
		}
	}

	// Miss: No matching page
	if (type == READ)
		// Use current block as an initial signature
		return curr_block; // Use current block as an initial signature

	// Update ST
	p_tag[match_idx] = tag;
	last_block[match_idx] = curr_block;
	DPRINTF(HWPrefetch,"\t[ST-UPDATE] Index: %4x ST-Miss[%d]: %x (partial_pnum) does not exist => Clear history and replacing lru[%d]: %d\n",
				idx, match_idx, tag, match_idx, lru[match_idx]);

	for (int j=0; j<HISTORY; j++) // DEBUG
		delta[match_idx][j] = 0;
	delta[match_idx][HISTORY-1] = new_delta;

	update_lru(match_idx, false, FROM_ST);
	new_delta = 1;
	signature[match_idx] = curr_block;
	owner->ST_miss++; owner->ST_access++;
	return curr_block;
}

int LookaheadPrefetcher::SPP_class::update_signature(int assoc, int type)
{
	int temp_delta = new_delta & 0x7F; // Use 7-bit to represent (+,-) stride patterns

	if (type == UPDATE) { // Update signature
		int temp_sig = signature[assoc];
		signature[assoc] = ((temp_sig << SIG_SHIFT) ^ temp_delta) & ((1 << SIG_LENGTH) - 1);

		// DEBUG
		DPRINTF(HWPrefetch,"\t[ST-UPDATE] Index: %4x curr-History[%d]: ", idx, assoc);
		for (int i=0; i<(HISTORY-1); i++) {
			delta[assoc][i] = delta[assoc][i+1];
			DPRINTF(HWPrefetch,"%3d ", delta[assoc][i]);
		}
		delta[assoc][HISTORY-1] = temp_delta;
		DPRINTF(HWPrefetch,"%3d", delta[assoc][HISTORY-1]);
		DPRINTF(HWPrefetch," => curr-Signature: %3x\n", signature[assoc]);
	}
	else { // Read signature
		DPRINTF(HWPrefetch,"\t[ST-READ] Index: %4x prev-History[%d]: ", idx, assoc);
		for (int i=0; i<HISTORY; i++)
			DPRINTF(HWPrefetch,"%3d ", delta[assoc][i]);
		DPRINTF(HWPrefetch," => prev-Signature: %3x\n", signature[assoc]);
	}
	return signature[assoc];
}

void LookaheadPrefetcher::SPP_class::update_counter(int match_idx, bool miss)
{
	if (miss) { // No matching pattern
		for (int i=0; i<PT_WAY; i++)
            counter[i]--; // Decrease counter
        counter[match_idx] = 0; // Reset counter for replaced entry
	}
	else { // Pattern match
		if (counter[match_idx] < (1 << COUNTER_BIT))
			counter[match_idx]++; // Increase counter
	}
}

void LookaheadPrefetcher::SPP_class::update_PT(int tag)
{
    int i, match_idx = -1;
	int min_counter = (1 << COUNTER_BIT) + 1;

	for (i=0; i<PT_WAY; i++) {
		if (valid[i] && (p_tag[i] == tag)) { // Hit: Matching stride found
			SPP_class::update_counter(i, false);
			DPRINTF(HWPrefetch,"\t[PT-UPDATE] Index: %4x  PT-Hit[%d]: %d  counter: %d\n",
						idx, i, tag, counter[i]);
			SPP_class::update_lru(i, false, FROM_PT);
			owner->PT_hit++; owner->PT_access++;
			return;
		}
		else if (valid[i] == false) { // Invalid
			p_tag[i] = tag;
			valid[i] = true;
			DPRINTF(HWPrefetch,"\t[PT-UPDATE] Index: %4x  PT-Invalid[%d]: %d  counter: %d\n",
						idx, i, tag, counter[i]);
			SPP_class::update_lru(i, true, FROM_PT);
			owner->PT_miss++; owner->PT_access++;
			return;
		}
		if (counter[i] < min_counter) { // Track lowest counter
			match_idx = i;
			min_counter = counter[i];
		}
	}
	// Miss
	p_tag[match_idx] = tag;
	SPP_class::update_counter(match_idx, true);
	DPRINTF(HWPrefetch,"\t[PT-UPDATE] Index: %4x  PT-Miss[%d]: %d does not exist Replacing counter[%d]: %d\n",
				idx, match_idx, tag, match_idx, counter[match_idx]);
	SPP_class::update_lru(match_idx, false, FROM_PT);
	owner->PT_miss++; owner->PT_access++;
	return;
}

int LookaheadPrefetcher::SPP_class::read_PT(int conf, int la_depth)
{
	int i, acc_conf = 0, max_conf = 0, max_counter = -1;
	pf_candidate = 0, la_candidate = 0;
	max_idx = -1;

	for (i=0; i<PT_WAY; i++) {
		acc_conf = conf*counter[i]/(1 << COUNTER_BIT);
		if (acc_conf >= PF_THRESHOLD) {
			pf_candidate = pf_candidate | (1 << i); 	// Mark prefetch candidates
			DPRINTF(HWPrefetch,"\t[PT]  Index: %4x  Prefetch[%d]:  %3d  prev-conf: %3d  check-conf: %3d  la_depth: %2d\n",
						idx, i, p_tag[i], conf, acc_conf, la_depth);

			if (acc_conf >= LA_THRESHOLD) {
				la_candidate = la_candidate | (1 << i); // Mark lookahead candidates
//				DPRINTF(HWPrefetch,"\t[PT]  Index: %4x  Lookahead[%d]: %3d  prev-conf: %3d  check-conf: %3d  la_depth: %2d\n",
//							idx, i, p_tag[i], conf, acc_conf, la_depth);
				if (max_counter < counter[i]) { // Record an entry with max_counter
					max_idx = i;
					max_counter = counter[max_idx];
				}
				if ((counter[max_idx] == counter[i]) && (lru[max_idx] < lru[i])) {
					max_idx = i; // Tie braker for same max_counter, choose most recently used on
				}
			}
		}
	}

	if (owner->depth) // During lookahead, choose only one pf_candidate with maximum confidence
		pf_candidate = 1 << max_idx;

	if(la_candidate) {
		max_conf = MAX_CONFIDENCE*counter[max_idx]/(1 << COUNTER_BIT);
		DPRINTF(HWPrefetch,"\t[PT] Index: %4x  Lookahead[%d]: %3d  has Local-MAX-conf: %d  la_depth: %2d\n",
								idx, max_idx, p_tag[max_idx], max_conf, la_depth);
	}

	return conf*max_conf/100; // return accumulated confidence
}

void LookaheadPrefetcher::attempt_prefetch(Addr base_addr, int base_block, int PT_idx, int la_candidate, int pf_candidate)
{
	Addr pf_addr,
		 curr_page = base_addr >> 12;
	int	num_la = 0, num_pf = 0,
		pf_block = 0,
		partial_pnum = curr_page & ((1 << ST_TAG_BIT) - 1);
	bool pf_issue = false;

	int FILTER_idx = curr_page % PRIME2;
	if (FILTER_idx >= 255)
		FILTER_idx = curr_page % PRIME3;

	// DEBUG
	for (int i=0; i<PT_WAY; i++) {
		num_la += (la_candidate & (1 << i)) / (1 << i);
		num_pf += (pf_candidate & (1 << i)) / (1 << i);
	}
	DPRINTF(HWPrefetch,"\t[PT-READ]  Lookahead: %d  Prefetch: %d  remain_mshr: %d  remain_rq: %d\n",
				num_la, num_pf, remain_mshr, remain_rq);

	// Handle pf_candidate first before we lookahead
	for (int i=0; i<PT_WAY; i++) {
		if (pf_candidate & (1 << i)) {
			pf_issue = false;
			pf_block = base_block + PT[PT_idx].p_tag[i];
			DPRINTF(HWPrefetch,"\t[PT-READ]  Checking base_block: %llu  next-stride[%3x][%d]: %d  pf_block: %d\n",
						(base_addr >> 6) &0x3F, PT_idx, i, PT[PT_idx].p_tag[i], pf_block);

			if (pf_block < 0 || pf_block > 63) {
				PF_beyond++; PF_attempt++;
				DPRINTF(HWPrefetch,"\t[PT-READ]  Out of same page boundary!\n");
				continue;
			}

			// Set prefetching address
			pf_addr = (((base_addr >> 12) << 6) + pf_block) << 6;

			// Check remaining mshr and read queue to decide where to prefetch
			if ((remain_mshr > MSHR_THRESHOLD) && (remain_rq > RQ_THRESHOLD)) {
#ifdef FILTER_ON
				// Filtering redundant prefetch requests
				if (!FILTER[FILTER_idx].check_filter(partial_pnum, pf_block, PREFETCH)) {
					PF_filtered++; PF_attempt++;
					continue;
				}
#endif
				pf_issue = l2_prefetch_line(0, base_addr, pf_addr, FILL_L2); // Issue L2 prefetch
				if (pf_issue) {
					// Update statistics
					PF_inflight++;
					PF_issue++; PF_local++; PF_attempt++;
					PF_L2_issue++; PF_L2_attempt++;

					remain_mshr--;
					remain_rq--;

					if (depth)
						LA_prefetch++;

					DPRINTF(HWPrefetch,"\t[PREFETCH-L2]   page: %llx  pf_block: %llu  pf_issue: %d  depth: %d remain_mshr: %d  remain_rq: %d\n",
								pf_addr>>12, (pf_addr>>6)&0x3F, pf_issue, depth, remain_mshr, remain_rq);

				}
				else {
					DPRINTF(HWPrefetch,"\t[PREFETCH-L2]   Prefetch is NOT issued!\n");
					PF_fail++; PF_local++; PF_attempt++;
					PF_L2_fail++; PF_L2_attempt++;
				}
			}
			else {
				if ((remain_rq > RQ_THRESHOLD)) {
#ifdef FILTER_ON
					// Filtering redundant prefetch requests
					if (!FILTER[FILTER_idx].check_filter(partial_pnum, pf_block, PREFETCH)) {
						PF_filtered++; PF_attempt++;
						continue;
					}
#endif
					pf_issue = l2_prefetch_line(0, base_addr, pf_addr, FILL_LLC); // Issue LLC prefetch
					if (pf_issue) {
						// Update statistics
						PF_inflight++;
						PF_issue++; PF_local++; PF_attempt++;
						PF_LLC_issue++; PF_LLC_attempt++;

						remain_rq--;

						if (depth)
							LA_prefetch++;
						DPRINTF(HWPrefetch,"\t[PREFETCH-LLC]  page: %llx  pf_block: %llu  pf_issue: %d  depth: %d remain_mshr: %d  remain_rq: %d\n",
									pf_addr>>12, (pf_addr>>6)&0x3F, pf_issue, depth, remain_mshr, remain_rq);
					}
					else {
						DPRINTF(HWPrefetch,"\t[PREFETCH-LLC]   Prefetch is NOT issued!\n");
						PF_fail++; PF_local++; PF_attempt++;
						PF_LLC_issue++; PF_LLC_attempt++;
					}
				}
				else {
					DPRINTF(HWPrefetch,"\t[PREFETCH-LLC]   Not enough resource to prefetch!\n");
					return;
				}
			}

			if (PF_inflight >= MAX_INFLIGHT) {
				DPRINTF(HWPrefetch,"\t[PREFETCH] PF_inflight reached threshold %d!\n", MAX_INFLIGHT);
				return;
			}
		}
	}
	return;
}

void LookaheadPrefetcher::lookahead(Addr base_addr, int base_block, int prev_conf, int PT_idx, int la_candidate)
{
	// Lookahead stars with the highest confidence
	int start = PT[PT_idx].max_idx,
		temp_delta = PT[PT_idx].p_tag[start] & 0x7F,
		next_sig = ((PT_idx << SIG_SHIFT) ^ temp_delta) & ((1 << SIG_LENGTH) - 1),
		next_conf = 0,
		next_la_candidate = 0,
		next_pf_candidate = 0,
		next_base_block   = base_block + PT[PT_idx].p_tag[start];

	// DEBUG
	DPRINTF(HWPrefetch,"\t[LOOKAHEAD] next-Signature: %3x => %3x  next_base_block: %3d\n", PT_idx, next_sig, next_base_block);

	if (next_base_block < 0 || next_base_block > 63) {
	DPRINTF(HWPrefetch,"\t[LOOKAHEAD] Next base block is Out of same page boundary!\n");
		return;
	}

	// Read PT with next signature
	next_conf = PT[next_sig].read_PT(prev_conf, (++depth));
	next_la_candidate = PT[next_sig].la_candidate;
	next_pf_candidate = PT[next_sig].pf_candidate;

	if (next_pf_candidate)
		attempt_prefetch(base_addr, next_base_block, next_sig, next_la_candidate, next_pf_candidate);

	if (PF_inflight >= MAX_INFLIGHT) {
		DPRINTF(HWPrefetch,"\t[LOOKAHEAD] Stop LOOKAHEAD PF_inflight reached %d!\n", MAX_INFLIGHT);
		return;
	}

	// If there is la_candidate and enough l2 read queue resource, try lookahead further
	if ((next_la_candidate) && (remain_rq > RQ_THRESHOLD)) {
		LA_attempt++;
		LA_depth[depth]++;
		lookahead(base_addr, next_base_block, next_conf, next_sig, next_la_candidate);
	}
	else if (next_la_candidate == 0) {
		DPRINTF(HWPrefetch,"\t[LOOKAHEAD] No more lookahead! next_la_candidate: %d\n", next_la_candidate);
		return;
	}
	return;
}

// Return false if block is filtered
bool LookaheadPrefetcher::SPP_class::check_filter(int tag, int block, int type)
{
	int match_idx = -1, min_lru = FILTER_WAY+1;

	for (int i=0; i<FILTER_WAY; i++) {
		// Hit
		if (valid[i] && (p_tag[i] == tag)) {
			if (filter_bitmap[i][block]) { // filter_bitmap is already set
				switch (type) {
					case DEMAND:
						DPRINTF(HWPrefetch,"\t[FILTER] Hit: tag: %2x Filtering (demand) request block: %2d type: %d return: %d\n",
									tag, block, type, false);
						return false;
						break;
					case PREFETCH:
						DPRINTF(HWPrefetch,"\t[FILTER] Hit: tag: %2x Filtering (prefetch) request block: %2d type: %d return: %d\n",
									tag, block, type, false);
						return false;
						break;
					case FILL:
						DPRINTF(HWPrefetch,"\t[FILTER] Hit: tag: %2x filter_bitmap is already on (fill) block: %2d type: %d return: %d\n",
									tag, block, type, false);
						return false;
						break;
					case EVICT:
						DPRINTF(HWPrefetch,"\t[FILTER] Hit: tag: %2x Clear (evict) block: %2d type: %d return: %d\n",
									tag, block, type, true);
						filter_bitmap[i][block] = 0;
						return true;
						break;
					case CHECK:
						DPRINTF(HWPrefetch,"\t[FILTER] Hit: tag: %2x Checked inflight pf_block: %2d type: %d return: %d\n",
									tag, block, type, false);
						return false;
						break;
				}
			}
			else { // filter_bitmap is not set
				switch (type) {
					case DEMAND:
						DPRINTF(HWPrefetch,"\t[FILTER] Hit: tag: %2x Set (demand) block: %2d type: %d return: %d\n",
									tag, block, type, true);
						filter_bitmap[i][block] = 1;
						return true;
						break;
					case PREFETCH:
						DPRINTF(HWPrefetch,"\t[FILTER] Hit: tag: %2x Set (prefetch) block: %2d type: %d return: %d\n",
									tag, block, type, true);
						filter_bitmap[i][block] = 1;
						return true;
						break;
					case FILL:
						DPRINTF(HWPrefetch,"\t[FILTER] Hit: tag: %2x Set (fill) block: %2d type: %d return: %d\n",
									tag, block, type, true);
						filter_bitmap[i][block] = 1;
						return false;
						break;
					case EVICT:
						DPRINTF(HWPrefetch,"\t[FILTER] Hit: tag: %2x Clear (evict) block: %2d type: %d return: %d\n",
									tag, block, type, true);
						filter_bitmap[i][block] = 0;
						return true;
						break;
					case CHECK:
						DPRINTF(HWPrefetch,"\t[FILTER] Hit: tag: %2x Check no inflight pf_block: %2d type: %d return: %d\n",
									tag, block, type, true);
						return true;
						break;
				}
			}
			if (type != CHECK) // CHECK does not update lru
				update_lru(i, false, FROM_FILTER);
		}

		// Invalid
		else if (valid[i] == false) {
			valid[i] = true;
			p_tag[i] = tag;
			update_lru(i, true, FROM_FILTER);
			if (type == DEMAND) {
				filter_bitmap[i][block] = 1;
				DPRINTF(HWPrefetch,"\t[FILTER] Invalid: tag: %2x (demand) recording block: %2d type: %d return: %d\n",
							tag, block, type, true);
			}
			else { // This is possible due to inclusive policy
				filter_bitmap[i][block] = 0;
				DPRINTF(HWPrefetch,"\t[FILTER] Invalid: tag: %2x (evict) recording block: %2d type: %d return: %d\n",
							tag, block, type, true);
			}
			return true;
		}
		if (lru[i] < min_lru) {
			match_idx = i;
			min_lru = lru[i];
		}
	}

	// Miss
	if (type == DEMAND) {
		p_tag[match_idx] = tag;
		for (int i=0; i<FILTER_WAY; i++)
			for (int j=0; j<BLOCK_SIZE; j++)
				filter_bitmap[i][j] = 0;
		update_lru(match_idx, false, FROM_FILTER);
		DPRINTF(HWPrefetch,"\t[FILTER] Miss: tag: %2x Demand recording block: %2d type: %d return: %d\n",
					tag, block, type, true);
		return true;
	}

	// EVICT: Block is evicted and page is missing in filter => Do nothing
	// FILL: Block is filled in and page is missing in filter
	// => Filled cache block is not likely to be prefetched again
	// => Do nothing instead of replacing a page with useful filtering info
	else
		return true;
}

void LookaheadPrefetcher::SPP_class::update_lru(int match_idx, bool invalid, int type)
{
    int i, num_entry;
	switch (type) {
		case FROM_ST:
			num_entry = ST_WAY;
			break;
		case FROM_PT:
			num_entry = PT_WAY;
			break;
		case FROM_FILTER:
			num_entry = FILTER_WAY;
			break;
    default:
        num_entry = 0;
	}
    if (invalid) { // One more entry, others decrease by 1
        for (i=0; i<num_entry; i++) {
            if (valid[i] && i != match_idx) {
                (lru[i])--;
//              DPRINTF(HWPrefetch,"\tIndex: %4d Decrease tag[%d]: %d  lru[%d]: %d\n",
//                          idx, i, tag[i], i, lru[i]);
            }
        }
    }
    else {
        for (i=0; i<num_entry; i++) {
            if ((lru[match_idx] < lru[i]) && (i != match_idx)) {
                (lru[i])--;
//              DPRINTF(HWPrefetch,"\tIndex: %4d Decrease tag[%d]: %d  lru[%d]: %d\n",
//                          idx, i, tag[i], i, lru[i]);
            }
//			else if (i != match_idx)
//              DPRINTF(HWPrefetch,"\tIndex: %4d Maintain tag[%d]: %d  lru[%d]: %d\n",
//                          idx, i, tag[i], i, tag_lru[i]);
        }
    }
    lru[match_idx] = num_entry; // Promote LRU
//	DPRINTF(HWPrefetch,"\tIndex: %4d Promoting lru[%d]: %d\n", idx, match_idx, lru[match_idx]);
}

void LookaheadPrefetcher::aux_prefetch(Addr base_addr)
{
	if ((remain_mshr > MSHR_THRESHOLD) && (remain_rq > RQ_THRESHOLD)) { // Try AUX to L2
		if (l2_prefetch_line(0, base_addr, ((base_addr>>6)+1)<<6, FILL_L2)) {
			// Update statistics
			PF_inflight++;
			AUX_issue++; AUX_local++; AUX_attempt++;
			AUX_L2_issue++; AUX_L2_attempt++;

			remain_mshr--;
			remain_rq--;

			DPRINTF(HWPrefetch,"\t[PREFETCH-L2-AUX] page: %llx  pf_block: %d  pf_issue: %d  remain_mshr: %d  remain_rq: %d\n",
						(base_addr >> 12), ((base_addr >> 6) & 0x3F) + 1, true, remain_mshr, remain_rq);
		}
		else {
			DPRINTF(HWPrefetch,"\t[PREFETCH-L2-AUX] Prefetch is NOT issued!\n");
			AUX_fail++; AUX_local++; AUX_attempt++;
			AUX_L2_fail++; AUX_L2_attempt++;
		}
	}
	else {
		if ((remain_rq > RQ_THRESHOLD)) {
			if (l2_prefetch_line(0, base_addr, ((base_addr>>6)+1)<<6, FILL_LLC)) {
				PF_inflight++;
				AUX_issue++; AUX_local++; AUX_attempt++;
				AUX_LLC_issue++; AUX_LLC_attempt++;

				remain_rq--;

				DPRINTF(HWPrefetch,"\t[PREFETCH-LLC-AUX] page: %llx  pf_block: %d  pf_issue: %d  remain_mshr: %d  remain_rq: %d\n",
							(base_addr >> 12), ((base_addr >> 6) & 0x3F) + 1, true, remain_mshr, remain_rq);
			}
			else {
				DPRINTF(HWPrefetch,"\t[PREFETCH-LLC-AUX] Prefetch is NOT issued!\n");
				AUX_fail++; AUX_local++; PF_attempt++;
				AUX_LLC_fail++; AUX_LLC_fail++;
			}
		}
		else {
				DPRINTF(HWPrefetch,"\t[PREFETCH-AUX] Not enough resource to activate AUX  remain_mshr: %d  remain_rq: %d\n", remain_mshr, remain_rq);
		}
	}
}
