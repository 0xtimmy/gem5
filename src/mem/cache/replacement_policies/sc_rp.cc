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
    // Reset insertion tick
    std::static_pointer_cast<SCReplData>(
        replacement_data)->tickInserted = ++timeTicks;
}

void
SC::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // A touch does not modify the insertion tick
}

void
SC::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // Set insertion tick
    std::static_pointer_cast<SCReplData>(
        replacement_data)->tickInserted = ++timeTicks;
}

ReplaceableEntry*
SC::getVictim(const ReplacementCandidates& candidates) const
{
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);

    // Visit all candidates to find victim
    ReplaceableEntry* victim = candidates[0];
    for (const auto& candidate : candidates) {
        // Update victim entry if necessary
        if (std::static_pointer_cast<SCReplData>(
                    candidate->replacementData)->tickInserted <
                std::static_pointer_cast<SCReplData>(
                    victim->replacementData)->tickInserted) {
            victim = candidate;
        }
    }

    return victim;
}

std::shared_ptr<ReplacementData>
SC::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new SCReplData());
}

} // namespace replacement_policy
} // namespace gem5
