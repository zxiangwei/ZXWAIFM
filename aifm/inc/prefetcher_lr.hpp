#pragma once

#include "boyer_moore_vote.hpp"
#include "trend_predictor_lr.hpp"

#include "sync.h"
#include "thread.h"

#include "helpers.hpp"
#include "pointer.hpp"

#include <functional>
#include <type_traits>

namespace far_memory {

class FarMemDevice;

namespace lr {

template<typename InduceFn, typename InferFn, typename MappingFn>
class Prefetcher {
 private:
  using InduceFnTraits = helpers::FunctionTraits<InduceFn>;
  using InferFnTraits = helpers::FunctionTraits<InferFn>;
  using MappingFnTraits = helpers::FunctionTraits<MappingFn>;

  using Index_t = typename InduceFnTraits::template Arg<0>::Type;
  using Pattern_t = typename InduceFnTraits::ResultType;

  // InduceFn: (Index_t, Index_t)->Pattern_t
  static_assert(InduceFnTraits::Arity == 2);
  static_assert(
      std::is_same<Index_t,
                   typename InduceFnTraits::template Arg<1>::Type>::value);

  // InferFn: (Index_t, Pattern_t)->Index_t
  static_assert(InferFnTraits::Arity == 2);
  static_assert(std::is_same<
      Index_t, typename InferFnTraits::template Arg<0>::Type>::value);
  static_assert(
      std::is_same<Pattern_t,
                   typename InferFnTraits::template Arg<1>::Type>::value);
  static_assert(
      std::is_same<Index_t, typename InferFnTraits::ResultType>::value);

  // MappingFn: (uint8_t *&, Index_t)->GenericUniquePtr *
  static_assert(MappingFnTraits::Arity == 2);
  static_assert(
      std::is_same<uint8_t *&,
                   typename MappingFnTraits::template Arg<0>::Type>::value);
  static_assert(
      std::is_same<Index_t,
                   typename MappingFnTraits::template Arg<1>::Type>::value);
  static_assert(std::is_same<GenericUniquePtr *,
                             typename MappingFnTraits::ResultType>::value);

  struct SlaveStatus {
    GenericUniquePtr *task;
    bool is_exited;
    rt::CondVar cv;
  };

  TrendPredictor<Pattern_t, 4096> trend_predictor_;

  constexpr static uint32_t kHitTimesThresh = 2;
  constexpr static uint32_t kMaxSlaveWaitUs = 5;
  constexpr static uint32_t kMaxNumPrefetchSlaveThreads = 16;
  constexpr static uint32_t kPrefetchNum = 1;

  const uint32_t kPrefetchWinSize_; // In terms of number of objects.
  uint8_t *state_;
//  Pattern_t pattern_;
  uint32_t object_data_size_;
  Index_t last_idx_;
  uint64_t hit_times_ = 0;
  uint32_t num_objs_to_prefetch = 0;
  Index_t next_prefetch_idx_;
  bool nt_ = false;
  std::vector<rt::Thread> prefetch_threads_;
  CachelineAligned(SlaveStatus) slave_status_[kMaxNumPrefetchSlaveThreads];
  bool exit_ = false;

  void prefetch_slave_fn(uint32_t tid);
  void dispatch_prefetch_task(GenericUniquePtr *task);

 public:
  Prefetcher(FarMemDevice *device, uint8_t *state, uint32_t object_data_size);
  ~Prefetcher();
  NOT_COPYABLE(Prefetcher);
  NOT_MOVEABLE(Prefetcher);
  void add_trace(bool nt, Index_t idx);
  void static_prefetch(Index_t start_idx, Pattern_t pattern, uint32_t num);
  void update_state(uint8_t *state);
};

} // namespace lr

} // namespace far_memory

#include "internal/prefetcher_lr.ipp"
