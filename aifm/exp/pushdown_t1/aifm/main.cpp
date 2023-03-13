extern "C" {
#include <runtime/runtime.h>
}

#include "snappy.h"

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
#include <thread>
#include <unistd.h>

constexpr uint64_t kCacheSize = 256 * Region::kSize;
constexpr uint64_t kFarMemSize = 20ULL << 30;
constexpr uint64_t kNumGCThreads = 15;
constexpr uint64_t kNumConnections = 600;
constexpr uint64_t kUncompressedFileSize = 1000000000;
constexpr uint64_t kUncompressedFileNumBlocks =
    ((kUncompressedFileSize - 1) / snappy::FileBlock::kSize) + 1;
constexpr uint32_t kNumUncompressedFiles = 16;
constexpr bool kUseTpAPI = false;

using namespace std;

#define LOG_ASSERT(expr, fmt, ...) \
  if (!(expr)) {                     \
    printf(fmt "\n", ##__VA_ARGS__);                                 \
  }

#define LOG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__);

alignas(4096) snappy::FileBlock file_block;
std::unique_ptr<Array<snappy::FileBlock, kUncompressedFileNumBlocks>>
    fm_array_ptrs[kNumUncompressedFiles];

void write_file_to_string(const string &file_path, const string &str) {
  std::ofstream fs(file_path);
  fs << str;
  fs.close();
}

void flush_cache() {
  for (uint32_t k = 0; k < kNumUncompressedFiles; k++) {
    fm_array_ptrs[k]->disable_prefetch();
  }
  for (uint32_t i = 0; i < kUncompressedFileNumBlocks; i++) {
    for (uint32_t k = 0; k < kNumUncompressedFiles; k++) {
      file_block = fm_array_ptrs[k]->read(i);
      ACCESS_ONCE(file_block.data[0]);
    }
  }
//  for (uint32_t k = 0; k < kNumUncompressedFiles; k++) {
//    fm_array_ptrs[k]->enable_prefetch();
//  }
}

void read_files_to_fm_array(const string &in_file_path) {
  int fd = open(in_file_path.c_str(), O_RDONLY | O_DIRECT);
  if (fd == -1) {
    helpers::dump_core();
  }
  LOG("open success");
  for (uint32_t k = 0; k < kNumUncompressedFiles; k++) {
    fm_array_ptrs[k]->disable_prefetch();
  }
  // Read file and save data into the far-memory array.
  int64_t sum = 0, cur = snappy::FileBlock::kSize, tmp;
  while (sum != kUncompressedFileSize) {
    BUG_ON(cur != snappy::FileBlock::kSize);
    cur = 0;
    while (cur < (int64_t)snappy::FileBlock::kSize) {
      tmp = read(fd, file_block.data + cur, snappy::FileBlock::kSize - cur);
      if (tmp <= 0) {
        break;
      }
      cur += tmp;
    }
    for (uint32_t i = 0; i < kNumUncompressedFiles; i++) {
      DerefScope scope;
      fm_array_ptrs[i]->at_mut(scope, sum / snappy::FileBlock::kSize) =
          file_block;
    }
    sum += cur;
    if ((sum % (1 << 20)) == 0) {
      cerr << "Have read " << sum << " bytes." << endl;
    }
  }
  if (sum != kUncompressedFileSize) {
    helpers::dump_core();
  }
  LOG("Read over");

  // Flush the cache to ensure there's no pending dirty data.
  flush_cache();

  close(fd);
}


template<uint64_t kNumBlocks, bool TpAPI>
void do_something(Array<snappy::FileBlock, kNumBlocks> *fm_array_ptr,
                  size_t input_length, std::string *compressed) {
//  snappy::FarMemArraySource<kNumBlocks, TpAPI> reader(input_length, fm_array_ptr);
  for (uint64_t i = 0; i < kNumBlocks; ++i) {
//    if (i % 4 == 3) continue; // 1 1 2
    uint64_t m = i % 6;
    if (m == 2 || m == 4 || m == 5) continue; // 1 2 3
    auto block = fm_array_ptr->read(i);
    DONT_OPTIMIZE(block);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

template<uint64_t kNumBlocks, bool TpAPI>
void call_compress(Array<snappy::FileBlock, kNumBlocks> *fm_array_ptr) {
  fm_array_ptr->register_local("SnappyCompress", [fm_array_ptr]() {
    std::string out_str;
    snappy::Compress<kUncompressedFileNumBlocks, kUseTpAPI>(
        fm_array_ptr, kUncompressedFileSize, &out_str);
  });
  rpc::BufferPtr args, ret;
  args = rpc::SerializeArgsToBuffer();
//  LOG("Start Call SnappyCompress");
  bool success = fm_array_ptr->call("SnappyCompress", args, ret);
  LOG_ASSERT(success, "Call SnappyCompress Failed");
  success = fm_array_ptr->call("SnappyCompress", args, ret);
  LOG_ASSERT(success, "Call SnappyCompress Failed");
}

std::string func_name(int i) {
  return "Func" + std::to_string(i);
}

void register_decision_funcs() {
  constexpr int kBaseTime = 2500;
  constexpr int kTimeStep = 1000;
  constexpr int kFuncNum = 16;
  static_assert(kFuncNum == kNumUncompressedFiles);
  for (int i = 0; i < kFuncNum; ++i) {
    fm_array_ptrs[i]->register_local(func_name(i), [i]() {
      for (size_t j = 0; j < kUncompressedFileNumBlocks; ++j) {
        auto block = fm_array_ptrs[i]->read(j);
        DONT_OPTIMIZE(block);
      }
      int sleep_time = static_cast<int>((kBaseTime + kTimeStep * i) * CostEstimator::kDefaultPMRatio);
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    });
  }
}

void call_decision_funcs() {
  for (uint32_t i = 0; i < kNumUncompressedFiles; i++) {
    auto start = chrono::steady_clock::now();
    rpc::BufferPtr args, ret;
    args = rpc::SerializeArgsToBuffer();
    bool success = fm_array_ptrs[i]->call(func_name(i), args, ret);
    LOG_ASSERT(success, "Call %d Failed", i);
    auto end = chrono::steady_clock::now();
    LOG("Call %d cost %ld us", i, chrono::duration_cast<chrono::microseconds>(end - start).count());
  }
}

template<uint64_t kNumBlocks, bool TpAPI>
void bench_farmem_load(Array<snappy::FileBlock, kNumBlocks> *fm_array_ptr,
                  size_t input_length, std::string *compressed) {
//  snappy::FarMemArraySource<kNumBlocks, TpAPI> reader(input_length, fm_array_ptr);
  fm_array_ptr->disable_prefetch();
  for (uint64_t i = 0; i < kNumBlocks; ++i) {
    auto block = fm_array_ptr->read(i);
    DONT_OPTIMIZE(block);
//    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

void fm_compress_files_bench(const string &in_file_path,
                             const string &out_file_path) {
  string out_str;
  LOG("Start");
  read_files_to_fm_array(in_file_path);
  register_decision_funcs();
//  call_decision_funcs();
  auto start = chrono::steady_clock::now();
  call_decision_funcs();
//  for (uint32_t i = 0; i < kNumUncompressedFiles; i++) {
//    std::cout << "Compressing file " << i << std::endl;
//    fm_array_ptrs[i]->snappy_compress();
//    fm_array_ptrs[i]->flush();
//    snappy::Compress<kUncompressedFileNumBlocks, kUseTpAPI>(
//        fm_array_ptrs[i].get(), kUncompressedFileSize, &out_str);
//    call_compress<kUncompressedFileNumBlocks, kUseTpAPI>(
//        fm_array_ptrs[i].get());
//    do_something<kUncompressedFileNumBlocks, kUseTpAPI>(
//        fm_array_ptrs[i].get(), kUncompressedFileSize, &out_str);
//    bench_farmem_load<kUncompressedFileNumBlocks, kUseTpAPI>(
//        fm_array_ptrs[i].get(), kUncompressedFileSize, &out_str);
//  }
  auto end = chrono::steady_clock::now();
  cout << "Elapsed time in microseconds : "
       << chrono::duration_cast<chrono::microseconds>(end - start).count()
       << " µs" << endl;

  // write_file_to_string(out_file_path, out_str);
}

void array_test() {
  constexpr uint64_t kIntArraySize = 1000;
  LOG("Start allocating");
  auto int_array = FarMemManagerFactory::get()->allocate_array_heap<int, kIntArraySize>();
  LOG("Start writing");
  for (uint64_t i = 0; i < kIntArraySize; ++i) {
    int_array->write(static_cast<int>(i), i);
  }
  LOG("Start flushing");
  int_array->flush();
  LOG("Start reading");
  for (uint64_t i = 0; i < kIntArraySize; ++i) {
    int v = int_array->read(i);
    LOG_ASSERT(v == (int)(i), "%d != %ld", v, i);
  }

  LOG("Start calling Add");
  rpc::BufferPtr args, ret;
  args = rpc::SerializeArgsToBuffer(2, 3);
  bool success = int_array->call("Add", args, ret);
  LOG_ASSERT(success, "Call Add Failed");
  int ans = rpc::GetReturnValueFromBuffer<int>(ret);
  LOG_ASSERT(ans == 5, "ans: %d, expected: 5", ans);

  LOG("Start calling Read");
  args = rpc::SerializeArgsToBuffer(static_cast<size_t>(4));
  success = int_array->call("Read", args, ret);
  LOG_ASSERT(success, "Call Read Failed");
  auto content = rpc::GetReturnValueFromBuffer<std::vector<uint8_t>>(ret);
  LOG_ASSERT(content.size() == sizeof(int), "size: %ld, expected: %ld", content.size(), sizeof(int));
  int parsed_content = *reinterpret_cast<int *>(content.data());
  LOG_ASSERT(parsed_content == 4, "parsed_content: %d, expected: 4", parsed_content);

  LOG("Finish");
}

void do_work(netaddr raddr) {
  auto manager = std::unique_ptr<FarMemManager>(FarMemManagerFactory::build(
      kCacheSize, kNumGCThreads,
      new TCPDevice(raddr, kNumConnections, kFarMemSize)));
  for (uint32_t i = 0; i < kNumUncompressedFiles; i++) {
    fm_array_ptrs[i].reset(
        manager->allocate_array_heap<snappy::FileBlock,
                                     kUncompressedFileNumBlocks>());
  }
  fm_compress_files_bench("/mnt/enwik9.uncompressed",
                          "/mnt/enwik9.compressed.tmp");
//  array_test();

  std::cout << "Force existing..." << std::endl;
  exit(0);
}

int argc;
void my_main(void *arg) {
  char **argv = (char **)arg;
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
