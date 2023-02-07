#pragma once

#include <vector>

#include "rpc_router.hpp"
#include "reader_writer_lock.hpp"
#include "server.hpp"

namespace far_memory {

class ServerArray : public ServerDS {
 public:
  explicit ServerArray(uint64_t num_items, uint32_t item_size)
      : vec_(num_items, std::vector<uint8_t>(item_size)),
        item_size_(item_size) {
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
  void RegisterRPCFuncs() {
    router_.Register("Add", Add);
    router_.Register("Read", [this](size_t index) { return Read(index); });
  }

  static int Add(int a, int b) { return a + b; }
  std::vector<uint8_t> Read(size_t index) {
    if (index >= vec_.size()) return {};
    return vec_[index];
  }

  std::vector<std::vector<uint8_t>> vec_;
  uint32_t item_size_;
  rpc::RpcRouter router_;
//  ReaderWriterLock lock_; // 暂时不使用锁
};

class ServerArrayFactory : public ServerDSFactory {
 public:
  ServerDS *build(uint32_t param_len, uint8_t *params) override;
};

} // namespace far_memory
