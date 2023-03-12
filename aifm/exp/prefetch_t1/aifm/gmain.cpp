#include "trend_generator.hpp"
#include "trend_writer.hpp"

constexpr int kTotalNum = 5;
constexpr int kSize = 3;

int main(int argc, char *argv[]) {
  trendtest::Generator<1, 9> generator;
  trendtest::Writer writer("trend.txt");
  for (int i = 0; i < kTotalNum; ++i) {
    writer.Write(generator.Generate(kSize));
  }
}

