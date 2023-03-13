#ifndef RPC_ARRAY_HPP_
#define RPC_ARRAY_HPP_

#include <vector>
#include <thread>

#include "rpc_router.hpp"
#include "reader_writer_lock.hpp"
#include "server.hpp"

namespace far_memory {

class ServerArray : public ServerDS {
 public:
  explicit ServerArray(uint64_t num_items, uint32_t item_size)
      : num_items_(num_items),
        item_size_(item_size),
        vec_(num_items * item_size){
    RegisterRPCFuncs();
  }
  ~ServerArray() override = default;

  void read_object(uint8_t obj_id_len, const uint8_t *obj_id,
                   uint16_t *data_len, uint8_t *data_buf);
  void write_object(uint8_t obj_id_len, const uint8_t *obj_id,
                    uint16_t data_len, const uint8_t *data_buf);
  bool remove_object(uint8_t obj_id_len, const uint8_t *obj_id);
  void compute(uint8_t opcode, uint16_t input_len, const uint8_t *input_buf,
               uint16_t *output_len, uint8_t *output_buf);
  // ret包括ErrorCode和实际的函数返回值
  void call(const std::string &method, const rpc::BufferPtr &args,
            rpc::BufferPtr &ret);

 private:
  uint8_t *ItemData(size_t index) {
    return vec_.data() + index * item_size_;
  }

  void RegisterRPCFuncs() {
    router_.Register("Add", Add);
    router_.Register("Read", [this](size_t index) { return Read(index); });
    router_.Register("SnappyCompress", [this]() { SnappyCompress(); });
    RegisterDecisionTestFuncs();
  }

  void RegisterDecisionTestFuncs() {
    constexpr int kBaseTime = 2500;
    constexpr int kTimeStep = 1000;
    constexpr int kFuncNum = 16;
    for (int i = 0; i < kFuncNum; ++i) {
      router_.Register("Func" + std::to_string(i), [i]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(kBaseTime + kTimeStep * i));
      });
    }
  }

  static int Add(int a, int b) { return a + b; }
  std::vector<uint8_t> Read(size_t index) {
    if (index >= vec_.size()) return {};
    std::vector<uint8_t> ret(item_size_);
    std::memcpy(ret.data(), ItemData(index), item_size_);
    return ret;
  }

  void SnappyCompress();

  uint64_t num_items_;
  uint32_t item_size_;
  std::vector<uint8_t> vec_;
  rpc::RpcRouter router_;
//  ReaderWriterLock lock_; // 暂时不使用锁
};

class ServerArrayFactory : public ServerDSFactory {
 public:
  ServerDS *build(uint32_t param_len, uint8_t *params) override;
};

} // namespace far_memory

#endif
