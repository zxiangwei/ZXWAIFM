#pragma once

#include "device.hpp"

#include <optional>

//#define PREFECHER_LR_LOG 1

#ifdef PREFECHER_LR_LOG
#include <iostream>
#endif

namespace far_memory {

namespace lr {

template <typename InduceFn, typename InferFn, typename MappingFn>
FORCE_INLINE Prefetcher<InduceFn, InferFn, MappingFn>::Prefetcher(
    FarMemDevice *device, uint8_t *state, uint32_t object_data_size)
    : kPrefetchWinSize_(device->get_prefetch_win_size() / (object_data_size)),
      state_(state), object_data_size_(object_data_size) {
  for (uint32_t i = 0; i < kMaxNumPrefetchSlaveThreads; i++) {
    auto &status = slave_status_[i].data;
    status.task = nullptr;
    status.is_exited = false;
    wmb();
    prefetch_threads_.emplace_back([&, i]() { prefetch_slave_fn(i); });
  }
}

template <typename InduceFn, typename InferFn, typename MappingFn>
FORCE_INLINE Prefetcher<InduceFn, InferFn, MappingFn>::~Prefetcher() {
  exit_ = true;
  wmb();
  for (uint32_t i = 0; i < kMaxNumPrefetchSlaveThreads; i++) {
    auto &status = slave_status_[i].data;
    while (!ACCESS_ONCE(status.is_exited)) {
      status.cv.Signal();
      thread_yield();
    }
  }
  for (auto &thread : prefetch_threads_) {
    thread.Join();
  }
}


template <typename InduceFn, typename InferFn, typename MappingFn>
FORCE_INLINE void
Prefetcher<InduceFn, InferFn, MappingFn>::dispatch_prefetch_task(GenericUniquePtr *task) {
  if (!task) return;
  bool dispatched = false;
  std::optional<uint32_t> inactive_slave_id = std::nullopt;
  for (uint32_t i = 0; i < kMaxNumPrefetchSlaveThreads; i++) {
    auto &status = slave_status_[i].data;
    if (status.cv.HasWaiters()) {
      inactive_slave_id = i;
      continue;
    }
    if (ACCESS_ONCE(status.task) == nullptr) {
      ACCESS_ONCE(status.task) = task;
      dispatched = true;
      break;
    }
  }
  if (!dispatched) {
    if (likely(inactive_slave_id)) {
      auto &status = slave_status_[*inactive_slave_id].data;
      status.task = task;
      wmb();
      status.cv.Signal();
    } else {
      DerefScope scope;
      task->swap_in(nt_);
    }
  }
}

template <typename InduceFn, typename InferFn, typename MappingFn>
FORCE_INLINE void
Prefetcher<InduceFn, InferFn, MappingFn>::prefetch_slave_fn(uint32_t tid) {
  auto &status = slave_status_[tid].data;
  GenericUniquePtr **task_ptr = &status.task;
  bool *is_exited = &status.is_exited;
  rt::CondVar *cv = &status.cv;
  cv->Wait();

  while (likely(!ACCESS_ONCE(exit_))) {
    if (likely(ACCESS_ONCE(*task_ptr))) {
      GenericUniquePtr *task = *task_ptr;
      ACCESS_ONCE(*task_ptr) = nullptr;
      DerefScope scope;
      task->swap_in(nt_);
    } else {
      auto start_us = microtime();
      while (ACCESS_ONCE(*task_ptr) == nullptr &&
          microtime() - start_us <= kMaxSlaveWaitUs) {
        cpu_relax();
      }
      if (unlikely(ACCESS_ONCE(*task_ptr) == nullptr)) {
        cv->Wait();
      }
    }
  }
  ACCESS_ONCE(*is_exited) = true;
}

template <typename InduceFn, typename InferFn, typename MappingFn>
FORCE_INLINE void
Prefetcher<InduceFn, InferFn, MappingFn>::add_trace(bool nt, Index_t idx) {
  InduceFn inducer;
  InferFn inferer;
  MappingFn mapper;

  auto new_pattern = inducer(last_idx_, idx);
  trend_predictor_.AddHistory(new_pattern);
  last_idx_ = idx;
  if (trend_predictor_.MatchTrend()) {
    hit_times_++;
    Index_t prefetch_idx = idx;
    for (uint32_t i = 0; i < kPrefetchNum; ++i) {
      auto pat = trend_predictor_.GetTrend(i);
      prefetch_idx = inferer(prefetch_idx, pat);
#ifdef PREFECHER_LR_LOG
      printf("visit(%d), prefetch(%d-%d)\n", idx, i, prefetch_idx);
#endif
      GenericUniquePtr *task = mapper(state_, prefetch_idx);
      dispatch_prefetch_task(task);
    }
  } else {
    hit_times_ = 0;
  }
}

template <typename InduceFn, typename InferFn, typename MappingFn>
FORCE_INLINE void Prefetcher<InduceFn, InferFn, MappingFn>::static_prefetch(
    Index_t start_idx, Pattern_t pattern, uint32_t num) {
  next_prefetch_idx_ = start_idx;
//  pattern_ = pattern;
  num_objs_to_prefetch = num;
}

template <typename InduceFn, typename InferFn, typename MappingFn>
FORCE_INLINE void
Prefetcher<InduceFn, InferFn, MappingFn>::update_state(uint8_t *state) {
  ACCESS_ONCE(state_) = state;
}

} // namespace lr

} // namespace far_memory
