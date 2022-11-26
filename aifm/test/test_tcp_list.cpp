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

constexpr uint64_t kCacheSize = (128ULL << 20);
constexpr uint64_t kFarMemSize = (4ULL << 30);
constexpr uint32_t kNumGCThreads = 12;
constexpr int kListSize = 10000000;
constexpr static uint32_t kNumConnections = 300;

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
  std::chrono::duration<double> diff = end - start;
  std::ofstream ofs("time.txt");
  ofs << diff.count() << " s\n";
  std::cout << "Passed" << std::endl;
}

int argc;
void _main(void *arg) {
  char **argv = static_cast<char **>(arg);
  std::string ip_addr_port(argv[1]);
  auto raddr = helpers::str_to_netaddr(ip_addr_port);
  std::unique_ptr<FarMemManager> manager =
      std::unique_ptr<FarMemManager>(FarMemManagerFactory::build(
          kCacheSize, kNumGCThreads,
          new TCPDevice(raddr, kNumConnections, kFarMemSize)));
  do_work(manager.get());
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

  ret = runtime_init(conf_path, _main, argv);
  if (ret) {
    std::cerr << "failed to start runtime" << std::endl;
    return ret;
  }

  return 0;
}
