extern "C" {
#include <runtime/runtime.h>
}

#include "region.hpp"
#include "array.hpp"
#include "deref_scope.hpp"
#include "device.hpp"
#include "helpers.hpp"
#include "manager.hpp"

#include <algorithm>
#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>
#include <unistd.h>

using namespace far_memory;

constexpr uint64_t kCacheSize = 64 * Region::kSize;
constexpr uint64_t kFarMemSize = 20ULL << 30;
constexpr uint64_t kNumGCThreads = 15;
constexpr uint64_t kNumConnections = 600;
constexpr uint64_t kArrayLen = 1000000;
constexpr int kLoopTimes = 100000;

using namespace std;

std::unique_ptr<Array<int, kArrayLen>> array_ptr;

void init_array() {
  for (int i = 0; i < kArrayLen; i++) {
    DerefScope scope;
    array_ptr->write(i, i);
  }
}

void fm_array_bench() {
  string out_str;
  init_array();
  array_ptr->disable_prefetch();
  auto start = chrono::steady_clock::now();

  for (int i = 0; i < kLoopTimes; ++i) {
    long long result = 0;
    int step = kArrayLen / 1000;
    for (int j = 0; j < kArrayLen; j += step) {
      result += array_ptr->read(j);
    }
    DONT_OPTIMIZE(result);
  }

  auto end = chrono::steady_clock::now();
  cout << "Elapsed time in microseconds : "
       << chrono::duration_cast<chrono::microseconds>(end - start).count()
       << " µs" << endl;
}

void do_work(netaddr raddr) {
  auto manager = std::unique_ptr<FarMemManager>(FarMemManagerFactory::build(
      kCacheSize, kNumGCThreads,
      new TCPDevice(raddr, kNumConnections, kFarMemSize)));
  array_ptr.reset( manager->allocate_array_heap<int, kArrayLen>());
  fm_array_bench();

  std::cout << "Force existing..." << std::endl;
  exit(0);
}

int argc;
void my_main(void *arg) {
  char **argv = (char **) arg;
  std::string ip_addr_port(argv[1]);
  do_work(helpers::str_to_netaddr(ip_addr_port));
}

int main(int _argc, char *argv[]) {
  int ret;

  if (_argc < 3) {
    std::cerr << "usage: [cfg_file] [ip_addr:port]" << std::endl;
    return -EINVAL;
  }

  char conf_path[strlen(argv[1]) + 1];
  strcpy(conf_path, argv[1]);
  for (int i = 2; i < _argc; i++) {
    argv[i - 1] = argv[i];
  }
  argc = _argc - 1;

  ret = runtime_init(conf_path, my_main, argv);
  if (ret) {
    std::cerr << "failed to start runtime" << std::endl;
    return ret;
  }

  return 0;
}
