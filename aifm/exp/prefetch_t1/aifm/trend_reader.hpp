#ifndef PATTERNGENERATOR__TREND_READER_HPP_
#define PATTERNGENERATOR__TREND_READER_HPP_

#include <optional>
#include <fstream>
#include <sstream>

#include "trend.hpp"

namespace trendtest {

class Reader {
 public:
  explicit Reader(std::string filename)
      : filename_(std::move(filename)),
        ifs_(filename_) {}

  ~Reader() { ifs_.close(); }

  std::optional<Trend> Read() {
    std::string content;
    if (!std::getline(ifs_, content)) return std::nullopt;
    std::stringstream ss(content);
    size_t size;
    ss >> size;
    std::vector<int> trend(size);
    for (size_t i = 0; i < size; ++i) {
      ss >> trend[i];
    }
    return Trend(std::move(trend));
  }

 private:
  std::string filename_;
  std::ifstream ifs_;
};

}

#endif //PATTERNGENERATOR__TREND_READER_HPP_
