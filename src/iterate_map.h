#pragma once

#include <initializer_list>
#include <vector>
#include <functional>
#include <optional>
#include <string>
#include <memory>
#include <bitset>
#include <immintrin.h>

#include "map_def.h"

namespace {
    // Count the number of set bits in [start, start + count), where count is in bytes
    int64_t vectorized_popcnt(void* start, size_t count) {

    }
}

namespace Affine {
    struct ExecutionOpts {
        // Only a suggestion
        bool use_threads = false;
        // Maximum, only a suggestion
        int num_threads = 4;

        // Does not need to be thread-safe
        std::optional<std::function<void(double /* progress */)>> progress_callback;
        // In seconds
        int callback_frequency = 1;
    };

    struct IterateMapOpts : public ExecutionOpts {
        // Maximum number to iterate to, inclusive. Negative number means to compute till the max allowed entry.
        int64_t max = -1;
    };

    constexpr int64_t _DEFAULT_MAX_ENTRY = 1'000'000'000;

    // Callback of the form (i, bool reachable) that can be applied to any solution
    template<class T>
        concept SolutionLambda = requires(T x) {
            { x(int64_t{}, bool{}) } -> std::same_as<void>;
        };

    template <AffineMapSet, int64_t>
        class StandardIterateMap;

    // Base iterate map class that all analyzers should implement
    // @tparam maps Set of linear maps to analyze
    // @tparam max_entry Maximum number to consider when iterating, exclusive
    template <AffineMapSet Maps, int64_t max_entry=_DEFAULT_MAX_ENTRY>
        class IterateMap {
            template<AffineMapSet, int64_t>
            friend class StandardIterateMap;

        public:
            static constexpr int64_t DEFAULT_MAX_ENTRY = _DEFAULT_MAX_ENTRY; 
        protected:
            int64_t _max_reached = -1;

            std::vector<int64_t> _initial_values;

            void range_bounds_check(int64_t min, int64_t max) {
                min = (min < 0) ? 0 : min;
                if (max < min || max > _max_reached) {
                    throw std::runtime_error("Invalid bounds min=" + std::to_string(min)
                            + ", max=" + std::to_string(max));
                }
            }

            virtual int64_t count_solutions_impl(int64_t min, int64_t max) = 0;
        public:
            /**
             * Set the initial values from which the map will be iterated (e.g., { 1 })
             */
            virtual void set_initial(std::initializer_list<int64_t> initial) {
                _initial_values.clear();
                for (int64_t i : initial) {
                    if (i < 0 || i >= max_entry) {
                        throw std::runtime_error{"Invalid initial value " + std::to_string(i) + "; must be in range [0.."
                                + std::to_string(max_entry - 1)};
                                
                    }

                    _initial_values.push_back(i);
                }
            }

            /**
             * Read the contents of an iterate map from a precomputed status file
             */
            virtual void read_from_file(const char* filename) = 0;

            /**
             * 
             */
            virtual void compute_till(const IterateMapOpts& opts) = 0;

            /**
             * Clear all computed data (essentially start from scratch)
             */
            virtual void clear_data() = 0;

            /**
             * Maximum value reached, inclusive
             */
            int64_t max_reached() {
                return _max_reached;
            }

            /**
             * Get the underlying maps of the analyzer
             */
            decltype(Maps) maps() {
                return Maps;
            }

            /**
             * Whether a value is reachable (unchecked)
             */
            virtual bool is_reachable(int64_t i) = 0;

            /**
             * Whether a value is reachable (throws if hasn't been computed, or otherwise OOB)
             */
            virtual bool is_reachable_checked(int64_t i) {
                if (i <= 0 || i > _max_reached) {
                    throw std::runtime_error("Attempted to access value " + std::to_string(i) + " out of bounds [0.."
                            + std::to_string(_max_reached) + "]");
                }

                return is_reachable(i);
            }

            /**
             * Count the number of solutions in [min, max], inclusive
             */
            virtual int64_t count_solutions(int64_t min=0, int64_t max=-1) {
                if (max == -1) max = _max_reached;
                range_bounds_check(min, max); 

                return count_solutions_impl(min, max);
            }

            /**
             * Execute a function, accepting a value and a boolean representing the reachability state,
             * for all known values
             */
            template <SolutionLambda L>
            void for_each_solution(L l, int64_t min=0, int64_t max=-1) {
                if (max == -1) max = _max_reached;
                range_bounds_check(min, max); 

                {
                    auto m = dynamic_cast<StandardIterateMap<Maps, max_entry>*>(this);
                    if (m) { 
                        m->for_each_solution_impl(min, max, l);
                        return;
                    }
                }

                throw std::runtime_error("Could not downcast iterate map");
            }

            /**
             * Essentially save the current progress by writing to a file
             */
            virtual void write_to_file(const char* filename) = 0;
        };

    /**
     * Straightforward solution which uses a bitset and simply iterates over the numbers directly. This method
     * is also platform-agnostic.
     */
    template<AffineMapSet Maps, int64_t max_entry=_DEFAULT_MAX_ENTRY>
        class StandardIterateMap : public IterateMap<Maps, max_entry> {
            template<AffineMapSet, int64_t>
                friend class IterateMap;

        protected:
            using BitsetType = std::bitset<max_entry>;

            std::unique_ptr<BitsetType> entries;

            template<typename L>
            void for_each_solution_impl(int64_t min, int64_t max, L l) {
                for (int64_t i = min; i <= max; ++i) {
                    l(i, (*entries)[i]);
                }
            }

            int64_t count_solutions_impl(int64_t min, int64_t max) {
                return entries->count();
            }
        public:
            StandardIterateMap() {
                entries = std::make_unique<BitsetType>(); // initializes to 0 by default
            }

            void read_from_file(const char* filename) {

            }

            void compute_till(const IterateMapOpts& opts) {
                auto max = opts.max;
                auto max_reached = IterateMap<Maps, max_entry>::max_reached();

                // With the standard (boring!) method we entirely ignore threads.
                if (max < max_reached) {
                    return;
                }

                if (max >= max_entry) {
                    throw std::runtime_error("Max entry exceeded (max=" + std::to_string(max) +")");
                }

                if (max_reached == -1) {
                    for (int64_t i : this->_initial_values) {
                        (*entries)[i] = 1;
                    }

                    max_reached = 0;
                }

                const auto coeffs = Maps.get_coeffs();

                for (int i = max_reached; i <= max; ++i) {
                    for (auto &coeff_pair : coeffs) {
                        int64_t a = coeff_pair.first;
                        int64_t b = coeff_pair.second;

                        int64_t k = i - b;
                        if (k % a == 0 && k >= 0) {
                            if ((*entries)[k / a]) {
                                (*entries)[i] = true;
                                break;
                            }
                        }
                    }
                }

                this->_max_reached = max;
            }

            void clear_data() {

            }

            bool is_reachable(int64_t i) {

            }

            void write_to_file(const char* filename) {

            }
        };
}
