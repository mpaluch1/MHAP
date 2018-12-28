#include <MinHashSketch.hpp>
#include <HashUtils.hpp>
#include <HitCounter.hpp>

#include <unordered_map>

//public
MinHashSketch::MinHashSketch(const std::string &seq,
                             size_t nGramSize,
                             size_t numHashes,
                             bool doReverseCompliment,
                             double repeatWeight)
    : minHashes_(computeNgramMinHashesWeighted(seq,
                                               nGramSize,
                                               numHashes,
                                               doReverseCompliment,
                                               repeatWeight)) {}

//protected
std::vector<int32_t>
MinHashSketch::computeNgramMinHashesWeighted(const std::string &seq,
                                             size_t nGramSize,
                                             size_t numHashes,
                                             bool doReverseCompliment,
                                             double repeatWeight) {
    auto kmerHashes{HashUtils::computeSeqHashesLong(seq, nGramSize, 0U, doReverseCompliment)};

    std::unordered_map<int64_t, HitCounter> hitMap;

    /**
     * Find all unique kmers with their frequencies
     */
     // faster to copy 32bit int
    for (const auto kmer : kmerHashes) {
        //TODO: maybe implement kmerFilter later
        if (hitMap.find(kmer) == hitMap.end()) {
            hitMap.emplace(kmer, 1U);
        } else {
            hitMap[kmer].addHit();
        }
    }

    std::vector<int32_t> hashes(std::max(1UL, numHashes), 0);
    std::vector<int64_t> best(numHashes, std::numeric_limits<int64_t>::max());

    //size_t numberValid = 0;

    /**
     * According to repeatWeight, choose n smallest
     * with greater probability when repeats
     */
    for (const auto &mapEntry : hitMap) {

        int64_t key = mapEntry.first;
        int32_t weight = mapEntry.second.count;

        if (repeatWeight < 0.0f) {
            weight = 1.0f;
        }

        //is this even occurs?
        //FIXME: this does not occur (always one or more)
        if (weight <= 0) {
            continue;
        }

        int64_t x = key;

        for (size_t word = 0U; word < numHashes; ++word) {
            //dunno which type is best here actually
            for (int32_t count = 0; count < weight; ++count) {
                // XORShift Random Number Generators
                // https://www.javamex.com/tutorials/random_numbers/xorshift.shtml
                // probably from here ^^
                x ^= (x << 21);
                //unsigned shift from Java equivalent
                x ^= (int64_t)((uint64_t)x >> 35);
                x ^= (x << 4);

                if (x < best[word]) {
                    best[word] = x;

                    if (word % 2 == 0) {
                        hashes[word] = static_cast<int32_t>(key);
                    } else {
                        hashes[word] = static_cast<int32_t>((int64_t)((uint64_t)key >> 32));
                        //unsigned shift no effect on unsigned value
                    }
                }
            }
        }
    }

    return hashes;
}