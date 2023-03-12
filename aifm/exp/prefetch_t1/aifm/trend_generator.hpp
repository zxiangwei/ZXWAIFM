#ifndef PATTERNGENERATOR__TREND_GENERATOR_HPP_
#define PATTERNGENERATOR__TREND_GENERATOR_HPP_

#include <random>

#include "trend.hpp"

namespace trendtest {

template<int kMin = 0, int kMax = 9>
class Generator {
  static_assert(kMin <= kMax);
 public:
  Generator() : dist_(kMin, kMax) {}

  Trend Generate(size_t size) {
    std::vector<int> trend(size);
    for (size_t i = 0; i < size; ++i) {
      trend[i] = dist_(rd_);
    }
    return Trend(std::move(trend));
  }

 private:
  std::random_device rd_;
  std::uniform_int_distribution<int> dist_;
};

}

#endif //PATTERNGENERATOR__TREND_GENERATOR_HPP_
