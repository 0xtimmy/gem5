// -----------------------------------------------------------------------------
// ECE 565
// -----------------------------------------------------------------------------

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_SC_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_SC_RP_HH__

#include "base/types.hh"
#include "mem/cache/replacement_policies/base.hh"

namespace gem5
{

struct SCRPParams;

namespace replacement_policy
{

class SC : public Base
{
  protected:
    /** SC-specific implementation of replacement data. */
    struct SCReplData : ReplacementData
    {
        /** Tick on which the entry was inserted. */
        Tick tickInserted;
        bool is_valid;
        bool is_sc;
        int curr_sc_count;

        Tick* tickTouched;
        bool* isTouched;
        /**
         * Default constructor. Invalidate data.
         */
        SCReplData()
          : tickInserted(0), is_valid(false), is_sc(false), 
          curr_sc_count(0), tickTouched(nullptr), isTouched(nullptr) {}

        SCReplData(int numsc) : tickInserted(0), is_valid(false), 
        is_sc(false), curr_sc_count(0) {
          tickTouched = new Tick[numsc];
          isTouched = new bool[numsc];
          for (int i = 0; i < numsc; i++) {
            tickTouched[i] = Tick(0);
            isTouched[i] = false;
          }
        }

        ~SCReplData() {
          delete[] tickTouched;
          delete[] isTouched;
        }
    };

  private:
    /**
     * A counter that tracks the number of
     * ticks since being created to avoid a tie
     */
    mutable Tick timeTicks;
    // whether or not the shepherd cache has been initialized
    mutable int numInitalizedFrames;
    const int numSCBlocks;

  public:
    typedef SCRPParams Params;
    SC(const Params &p);
    ~SC() = default;

    /**
     * Invalidate replacement data to set it as the next probable victim.
     * Reset insertion tick to 0.
     *
     * @param replacement_data Replacement data to be invalidated.
     */
    void invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
                                                                    override;

    /**
     * Touch an entry to update its replacement data.
     * Does not modify the replacement data.
     *
     * @param replacement_data Replacement data to be touched.
     */
    void touch(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Reset replacement data. Used when an entry is inserted.
     * Sets its insertion tick.
     *
     * @param replacement_data Replacement data to be reset.
     */
    void reset(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Find replacement victim using insertion timestamps.
     *
     * @param cands Replacement candidates, selected by indexing policy.
     * @return Replacement entry to be replaced.
     */
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const
                                                                     override;

    /**
     * Instantiate a replacement data entry.
     *
     * @return A shared pointer to the new replacement data.
     */
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_SC_RP_HH__
