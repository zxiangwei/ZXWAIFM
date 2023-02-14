#ifndef COST_ESTIMATOR_HPP_
#define COST_ESTIMATOR_HPP_

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

namespace far_memory {

#define COST_LOG_ON 1

#if COST_LOG_ON
#define COST_LOG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__);
#else
#define COST_LOG(fmt, ...)
#endif

// 注意要使用指针形式以使用多态
class CostEstimator {
 public:
  static constexpr double kDefaultPMRatio = 0.9;
  static constexpr double kDefaultPushdownRatio = 0.95;

  CostEstimator()
      : internet_speed_(0),
        compute_in_memory_time_(0),
        compute_in_processor_time_(0),
        ret_time_(0),
        pm_ratio_(kDefaultPMRatio),
        pushdown_ratio_(kDefaultPushdownRatio) {}

  void StartBench() {
    COST_LOG("StartBench");
    start_ = std::chrono::steady_clock::now();
  }
  void FlushOver(uint64_t flush_bytes) {
    uint64_t us = EndBench();
    uint64_t speed = flush_bytes / us;
    COST_LOG("flush %ld bytes with %ld us, speed: %ld", flush_bytes, us, speed);
    if (internet_speed_ == 0) { // 第一次
      internet_speed_ = speed;
    } else {
      internet_speed_ = (internet_speed_ + speed) >> 1;
    }
  }
  void ComputeInMemoryOver(uint64_t ret_bytes) {
    uint64_t us = EndBench();
    uint64_t rtime = ret_bytes / internet_speed_;
    uint64_t mtime = us - rtime;
    uint64_t ptime = static_cast<uint64_t>(static_cast<double>(mtime) * pm_ratio_);
    COST_LOG("compute in memory return %ld bytes with %ld us, rtime: %ld, mtime: %ld, ptime: %ld",
             ret_bytes, us, rtime, mtime, ptime);
    if (compute_in_memory_time_ == 0) { // 第一次
      ret_time_ = rtime;
      compute_in_memory_time_ = mtime;
      compute_in_processor_time_ = ptime;
    } else {
      ret_time_ = (ret_time_ + rtime) >> 1;
      compute_in_memory_time_ = (compute_in_memory_time_ + mtime) >> 1;
      compute_in_processor_time_ = (compute_in_processor_time_ + ptime) >> 1;
    }
  }
  void ComputeInProcessorOver(uint64_t load_bytes) {
    uint64_t us = EndBench();
    uint64_t ptime = us - load_bytes / internet_speed_;
    COST_LOG("compute in processor load %ld bytes with %ld us, ptime: %ld",
             load_bytes, us, ptime);
    compute_in_processor_time_ = (compute_in_processor_time_ + ptime) >> 1;
    pm_ratio_ = static_cast<double>(compute_in_processor_time_) /
        static_cast<double>(compute_in_memory_time_);
    COST_LOG("pm_ratio change to %f", pm_ratio_);
  }

  bool SuggestPushdown(uint64_t flush_bytes, uint64_t load_bytes) {
    if (compute_in_memory_time_ == 0) return true;
    uint64_t ptime = flush_bytes / internet_speed_ + compute_in_memory_time_ + ret_time_;
    uint64_t nptime = load_bytes / internet_speed_ + compute_in_processor_time_;
    uint64_t tmp = static_cast<uint64_t>(static_cast<double>(ptime) * pushdown_ratio_);
    COST_LOG("suggest_pushdown(flush %ld bytes, load %ld bytes, ptime: %ld, nptime: %ld, tmp: %ld): %s",
             flush_bytes, load_bytes, ptime, nptime, tmp,
             (nptime >= tmp) ? "true" : "false");
    return nptime >= tmp;
  }

 protected:
  uint64_t EndBench() {
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
  }

  uint64_t internet_speed_; // 单位为 byte/us
  uint64_t compute_in_memory_time_;  // 单位为 us
  uint64_t compute_in_processor_time_;  // 单位为 us
  uint64_t ret_time_; // 单位为 us
  double pm_ratio_; // processor计算时间 / memory计算时间
  double pushdown_ratio_;
  std::chrono::steady_clock::time_point start_;
};

using CostEstimatorPtr = std::shared_ptr<CostEstimator>;

class CostEstimatorMap {
 public:
  CostEstimatorMap() = default;

  void Set(const std::string &method, const CostEstimatorPtr &estimator) {
    method_to_estimator_[method] = estimator;
  }

  CostEstimatorPtr Get(const std::string &method) {
    if (!method_to_estimator_.count(method)) {
      method_to_estimator_[method] = DefaultEstimator();
    }
    return method_to_estimator_[method];
  }
 private:
  static CostEstimatorPtr DefaultEstimator() {
    return std::make_shared<CostEstimator>();
  }

  std::unordered_map<std::string, CostEstimatorPtr> method_to_estimator_;
};

} // namespace far_memory

#endif //COST_ESTIMATOR_HPP_
