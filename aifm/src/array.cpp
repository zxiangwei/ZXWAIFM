#include "array.hpp"
#include "internal/ds_info.hpp"
#include "manager.hpp"
#include "pointer.hpp"

//#include "snappy.h"

namespace far_memory {

GenericArray::GenericArray(FarMemManager *manager, uint32_t item_size,
                           uint64_t num_items)
    : kNumItems_(num_items), kItemSize_(item_size),
      prefetcher_(manager->get_device(), reinterpret_cast<uint8_t *>(&ptrs_),
                  item_size) {
  ds_id_ = manager->allocate_ds_id();
  uint8_t params[sizeof(num_items) + sizeof(item_size)];
  __builtin_memcpy(&params[0], &num_items, sizeof(num_items));
  __builtin_memcpy(&params[sizeof(num_items)], &item_size, sizeof(item_size));
  manager->construct(kArrayDSType, ds_id_,
                     sizeof(num_items) + sizeof(item_size), params);

  preempt_disable();
  ptrs_.reset(new GenericUniquePtr[num_items]);
  preempt_enable();
  for (uint64_t i = 0; i < num_items; i++) {
    ptrs_[i] = manager->allocate_generic_unique_ptr(ds_id_, item_size,
                                                    sizeof(i), reinterpret_cast<uint8_t *>(&i));
  }
  kItemSize_ = item_size;
  kNumItems_ = num_items;

  cost_estimator_["snappy_compress"] =  std::make_shared<CostEstimator>();
}

GenericArray::~GenericArray() {
  FarMemManagerFactory::get()->destruct(ds_id_);
}

void GenericArray::flush() {
  if (!dirty_) return;
  dirty_ = false;
  std::vector<rt::Thread> threads;
  for (uint32_t tid = 0; tid < helpers::kNumCPUs; tid++) {
    threads.emplace_back([&, tid]() {
      auto num_tasks_per_threads =
          (kNumItems_ == 0)
          ? 0
          : (kNumItems_ - 1) / helpers::kNumCPUs + 1;
      auto left = num_tasks_per_threads * tid;
      auto right = std::min(left + num_tasks_per_threads, kNumItems_);
      for (uint64_t i = left; i < right; i++) {
        ptrs_[i].flush();
      }
    });
  }
  for (auto &thread : threads) {
    thread.Join();
  }
}

bool GenericArray::call(const std::string &method, const rpc::BufferPtr &args, rpc::BufferPtr &ret) {
  return FarMemManagerFactory::get()->call(ds_id_, method, args, ret);
}

void GenericArray::snappy_compress() {
  // 其实可以考虑在Array内部添加一个RPCRouter
  // 本地调用通过本地的RPCRouter来进行，远程调用通过ServerArray的RPCRouter来进行
  // 不过这要求两者具有相同的参数数量和类型
  auto estimator = cost_estimator_["snappy_compress"];
  uint64_t flush_bytes = 0, load_bytes = 0;
  // 统计 flush_bytes 和 load_bytes
  for (uint64_t i = 0; i < kNumItems_; ++i) {
    if (!ptrs_[i].is_present()) {
      load_bytes += kItemSize_;
    } else if (ptrs_[i].is_dirty()){
      flush_bytes += kItemSize_;
    }
  }
  if (estimator->SuggestPushdown(flush_bytes, load_bytes)) {
    estimator->StartBench();
    flush();
    estimator->FlushOver(flush_bytes);
    rpc::BufferPtr args, ret;
    args = rpc::SerializeArgsToBuffer();
    estimator->StartBench();
    FarMemManagerFactory::get()->call(ds_id_, "SnappyCompress", args, ret);
    estimator->ComputeInMemoryOver(ret->ReadableBytes());
  } else {
    estimator->StartBench();
    snappy_compress_local();
    estimator->ComputeInProcessorOver(load_bytes);
  }
}

void GenericArray::snappy_compress_local() {
//    snappy::Compress<kUncompressedFileNumBlocks, kUseTpAPI>(
//        this, kUncompressedFileSize, &out_str);
}

void GenericArray::disable_prefetch() {
  ACCESS_ONCE(dynamic_prefetch_enabled_) = false;
}

void GenericArray::enable_prefetch() {
  ACCESS_ONCE(dynamic_prefetch_enabled_) = true;
}

void GenericArray::static_prefetch(Index_t start, Index_t step, uint32_t num) {
  ACCESS_ONCE(dynamic_prefetch_enabled_) = false;
  prefetcher_.static_prefetch(start, step, num);
}

} // namespace far_memory
