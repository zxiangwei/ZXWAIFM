extern "C" {
#include <runtime/runtime.h>
}

#include "deref_scope.hpp"
#include "list.hpp"
#include "manager.hpp"

#include <iostream>
#include <chrono>
#include <fstream>

using namespace far_memory;

constexpr uint64_t kCacheSize = (1ULL << 10);
constexpr uint64_t kFarMemSize = (4ULL << 30);
constexpr uint32_t kNumGCThreads = 12;
constexpr int kListSize = 10000000;

namespace far_memory {

class FarMemTest {
 public:
  void do_work(FarMemManager *manager) {
    std::cout << "Running " << __FILE__ "..." << std::endl;
    DerefScope scope;
    List<int> list = FarMemManagerFactory::get()->allocate_list<int>(
        scope, /* enable_merge = */ true);
    for (int i = 0; i < kListSize; ++i) {
      list.push_back(scope, i);
    }
    auto start = std::chrono::high_resolution_clock::now();
    for (auto iter = list.begin(scope); iter != list.end(scope);
         iter.inc(scope)) {
      int val = iter.deref(scope);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end-start;
    std::ofstream ofs("time.txt");
    ofs << "Iter cost " << diff.count() << " s\n";
    std::cout << "Passed" << std::endl;
  }
};
} // namespace far_memory

void _main(void *arg) {
  std::unique_ptr<FarMemManager> manager =
      std::unique_ptr<FarMemManager>(FarMemManagerFactory::build(
          kCacheSize, kNumGCThreads, new FakeDevice(kFarMemSize)));
  FarMemTest test;
  test.do_work(manager.get());
}

int main(int argc, char *argv[]) {
  int ret;

  if (argc < 2) {
    std::cerr << "usage: [cfg_file]" << std::endl;
    return -EINVAL;
  }

  ret = runtime_init(argv[1], _main, NULL);
  if (ret) {
    std::cerr << "failed to start runtime" << std::endl;
    return ret;
  }

  return 0;
}
