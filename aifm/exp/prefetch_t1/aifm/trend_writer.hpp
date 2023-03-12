#ifndef PATTERNGENERATOR__TREND_WRITER_HPP_
#define PATTERNGENERATOR__TREND_WRITER_HPP_

#include <fstream>
#include <utility>

#include "trend.hpp"

namespace trendtest {

class Writer {
 public:
  explicit Writer(std::string filename)
      : filename_(std::move(filename)),
        ofs_(filename_) {}

  ~Writer() { ofs_.close(); }

  void Write(const Trend &trend) {
    ofs_ << trend.GetSize();
    trend.ForEach([this](size_t pattern) {
      ofs_ << " " << pattern;
    });
    ofs_ << "\n";
  }

 private:
  std::string filename_;
  std::ofstream ofs_;
};

}

#endif //PATTERNGENERATOR__TREND_WRITER_HPP_
