#ifndef PATTERNGENERATOR__TREND_HPP_
#define PATTERNGENERATOR__TREND_HPP_

#include <utility>
#include <vector>
#include <cassert>

namespace trendtest {

class Trend {
 public:
  explicit Trend(std::vector<int> trend)
      : trend_(std::move(trend)),
        size_(trend_.size()),
        pos_(0) {
    assert(size_ > 0);
  }

  size_t GetSize() const { return size_; }

  int NextPattern() {
    int ret = trend_[pos_];
    pos_ = (pos_ + 1) % size_;
    return ret;
  }

  template<typename Func>
  void LoopWithLimit(Func &&func, size_t limit) {
    for (size_t i = 0; i < limit; i += NextPattern()) {
      func(i);
    }
  }

  template<typename Func>
  void LoopWithTimes(Func &&func, size_t times = 1) const {
    size_t index = 0;
    func(index);
    for (size_t i = 0; i < times; ++i) {
      for (size_t j = 0; j < size_; ++j) {
        index += trend_[j];
        func(index);
      }
    }
  }

  template<typename Func>
  void ForEach(Func &&func) const {
    for (int pattern: trend_) {
      func(pattern);
    }
  }

 private:
  std::vector<int> trend_;
  size_t size_;
  size_t pos_;
};

}

#endif //PATTERNGENERATOR__TREND_HPP_
