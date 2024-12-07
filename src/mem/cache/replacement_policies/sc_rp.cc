// -----------------------------------------------------------------------------
// ECE 565
// -----------------------------------------------------------------------------

#include "mem/cache/replacement_policies/sc_rp.hh"

#include <cassert>
#include <memory>

#include "params/SCRP.hh"
#include "sim/cur_tick.hh"

namespace gem5
{

namespace replacement_policy
{

SC::SC(const Params &p)
  : Base(p), numSCBlocks(p.num_sc_blocks)
{
}

void
SC::invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
{
    /*
        invalidate() {
            set is_valid to false
            is_touched -> false
            is_sc -> false
            if(is_sc) {
                find out which idx the block is at
                shift all is_touched and tick_touched to remove that row
            }
        }
    */

    std::static_pointer_cast<SCReplData>(replacement_data)->is_valid = false;
    std::static_pointer_cast<SCReplData>(replacement_data)->is_touched = false;
    //if (std::static_pointer_cast<SCReplData>(replacement_data)->is_sc) { }
    std::static_pointer_cast<SCReplData>(replacement_data)->is_sc = false;
}

void
SC::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    /*
        touch() {
            set tick_touched if is_touched is zero (for all entries)
        }
    */
    // A touch does not modify the insertion tick
    for (int i = 0; i < numSCBlocks; i++) {
        if (!std::static_pointer_cast<SCReplData>(replacement_data)->is_touched[i]) {
            std::static_pointer_cast<SCReplData>(replacement_data)->is_touched[i] = true;
            std::static_pointer_cast<SCReplData>(replacement_data)->tick_touched[i] = timeTicks;
        }
    }

}

void
SC::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    /*
        reset() {
            set is_valid to true
            set is_sc to true
            set tick_insert
            set is_touched to all false
        }
    */

    // Set insertion tick
    ++timeTicks
    std::static_pointer_cast<SCReplData>(replacement_data)->is_valid = true;
    std::static_pointer_cast<SCReplData>(replacement_data)->is_sc = true;
    std::static_pointer_cast<SCReplData>(replacement_data)->tick_insert = timeTicks;
}

ReplaceableEntry*
SC::getVictim(const ReplacementCandidates& candidates) const
{
    /*
        getVictim() {
            assert(candidates.size() > 0)
            assert <= 4 things in sc

            if empty slot, return that, and remove from fifo

            get oldest of sc fifo (loop until find oldest tick_insert and is_sc)
            go through all mc blocks and find youngest (including tail)
            shift tick_touched and is_touched for all blocks
            clear [0] for all blocks
            return oldest found earlier

            Random amongst untouched if not all elements have been put in.
        }
    */

    assert(candidates.size() > 0); // There must be at least one replacement candidate
    assert(candidates.size() - numSCBlocks > 0); // Cannot be more sc blocks than candidates
    int num_sc = 0;
    for (const auto& candidate : candidates)
        if(std::static_pointer_cast<SCReplData>(candidate->replacementData)->is_sc)
            num_sc++;

    assert(num_sc == numSCBlocks) // State should always start with the correct number of sc blocks
    
    // what state to blocks start in?

    // do the shifts
    void _shift_sc(int scidx) {
        // shift the imminence trackers
        for (const auto& candidate : candidates) {
            for (int i = scidx; i < numSCBlocks-1; i++) {
                std::static_pointer_cast<SCReplData>(candidate->replacementData)->tickTouched[i]
                    = std::static_pointer_cast<SCReplData>(candidate->replacementData)->tickTouched[i+1];
            } 
            std::static_pointer_cast<SCReplData>(candidate->replacementData)->tickTouched[numSCBlocks-1] = 0;
            std::static_pointer_cast<SCReplData>(candidate->replacementData)->isTouched[numSCBlocks-1] = false;
        }
    }

    void _victimize(ReplaceableEntry* victim) {
        // shift the imminence fifos
        if (std::static_pointer_cast<SCReplData>(candidate->replacementData)->is_sc) {
            int scidx = 0;
            for (const auto& candidate : candidates) {
                if(std::static_pointer_cast<SCReplData>(candidate->replacementData)->is_sc) {
                    if (std::static_pointer_cast<SCReplData>(candidate->replacementData)->tickInserted > 
                    std::static_pointer_cast<SCReplData>(victim->replacementData)->tickInserted) {
                        scidx++;
                    }
                }
            }
            _shift_sc(scidx);
        } else {
            std::static_pointer_cast<SCReplData>(rplsc->replacementData)->is_sc = false;
            // calling reset() on the candidate will make is sc, preserving the number of sc blocks
            _shift_sc(0);
        }
    }

    // find the next sc block
    Tick mintick = -1;
    for (const auto& candidate : candidates) {
        if (std::static_pointer_cast<SCReplData>(candidate->replacementData)->is_sc) {
            tin = std::static_pointer_cast<SCReplData>(candidate->replacementData)->tickInserted;
            if (mintick == Tick(-1) || tin < mintick) mintick = tin;
        }
    }
    ReplaceableEntry* rplsc = nullptr;
    for (const auto& candidate : candidates) {
        if (std::static_pointer_cast<SCReplData>(candidate->replacementData)->is_sc) {
            tin = std::static_pointer_cast<SCReplData>(candidate->replacementData)->tickInserted;
            if (mintick == tin) rplsc = candidate;
        }
    }
    assert(rplsc != nullptr); // assert than a next sc block was found

    // if there is an invalid block evict that one
    for (const auto& candidate : candidates) {
        if (!std::static_pointer_cast<SCReplData>(candidate->replacementData)->is_valid) {
            _victimize(candidate)
            return candidate;
        }
    }
            
    // Visit all candidates to find victim
    ReplaceableEntry* victim = candidates[0];
    for (const auto& candidate : candidates) {
        // victim will be the block that hasn't been touched or was touched more recentlyy
        if (!std::static_pointer_cast<SCReplData>(candidate->replacementData)->isTouched[0]) {
            // victimize an untouched cache block
            _victimize(candidate);
            return candidate;
        }
        if (std::static_pointer_cast<SCReplData>(candidate->replacementData)->tickTouched[0] >
            std::static_pointer_cast<SCReplData>(victim->replacementData)->tickTouched[0]) {
            // victimize a younger block
            victim = candidate;
        }
    }

    // victimize youngest block
    _victimize(victim);
    return victim;
}

std::shared_ptr<ReplacementData>
SC::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new SCReplData(numSCBlocks));
}

} // namespace replacement_policy
} // namespace gem5
