#include "server_array.hpp"

namespace far_memory {

void ServerArray::read_object(uint8_t obj_id_len, const uint8_t *obj_id, uint16_t *data_len, uint8_t *data_buf) {
//  auto reader_lock = lock_.get_reader_lock();
  uint64_t index;
  assert(obj_id_len == sizeof(index));
  index = *reinterpret_cast<const uint64_t *>(obj_id);

  *data_len = item_size_; // 注意：这里由uint32_t缩小为uint16_t
  __builtin_memcpy(data_buf, vec_[index].data(), item_size_);
}

void ServerArray::write_object(uint8_t obj_id_len, const uint8_t *obj_id, uint16_t data_len, const uint8_t *data_buf) {
//  auto reader_lock = lock_.get_reader_lock();
  uint64_t index;
  assert(obj_id_len == sizeof(index));
  index = *reinterpret_cast<const uint64_t *>(obj_id);

  assert(data_len == item_size_);
  __builtin_memcpy(vec_[index].data(), data_buf, data_len);
}

bool ServerArray::remove_object(uint8_t obj_id_len, const uint8_t *obj_id) {
  BUG();
}

void ServerArray::compute(uint8_t opcode,
                          uint16_t input_len,
                          const uint8_t *input_buf,
                          uint16_t *output_len,
                          uint8_t *output_buf) {
  BUG();
}

void ServerArray::call(const std::string &method, const rpc::BufferPtr &args,
                       rpc::BufferPtr &ret) {
  auto reply = router_.Call(method, args);
  ret = std::make_shared<rpc::Buffer>(sizeof(rpc::RpcErrorCode) + reply.ret->ReadableBytes());
  rpc::Serializer ret_serializer(ret);
  ret_serializer << reply.error_code << reply.ret;
}

ServerDS *ServerArrayFactory::build(uint32_t param_len, uint8_t *params) {
  auto num_items = *reinterpret_cast<uint64_t *>(&params[0]);
  auto item_size = *reinterpret_cast<uint32_t *>(&params[sizeof(num_items)]);
  return new ServerArray(num_items, item_size);
}

}
