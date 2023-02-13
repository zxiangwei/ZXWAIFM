extern "C" {
#include <net/ip.h>
#include <runtime/runtime.h>
#include <runtime/tcp.h>
#include <runtime/thread.h>
}
#include "thread.h"

#include "device.hpp"
#include "helpers.hpp"
#include "object.hpp"
#include "server.hpp"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

using namespace far_memory;

std::vector<rt::Thread> slave_threads;
std::unique_ptr<uint8_t> far_mem;

std::atomic<bool> has_shutdown{true};
rt::Thread master_thread;
Server server;
FILE *log_file;

#define FLOG(fmt, ...) fprintf(log_file, fmt "\n", ##__VA_ARGS__); fflush(log_file);

// Request:
//     |OpCode = Init (1B)|Far Mem Size (8B)|
// Response:
//     |Ack (1B)|
void process_init(tcpconn_t *c) {
  uint64_t *far_mem_size;
  uint8_t req[sizeof(decltype(*far_mem_size))];
  helpers::tcp_read_until(c, req, sizeof(req));

  far_mem_size = reinterpret_cast<uint64_t *>(req);
  *far_mem_size = ((*far_mem_size - 1) / helpers::kHugepageSize + 1) *
                  helpers::kHugepageSize;
  auto far_mem_ptr =
      static_cast<uint8_t *>(helpers::allocate_hugepage(*far_mem_size));
  BUG_ON(far_mem_ptr == nullptr);
  far_mem.reset(far_mem_ptr);

  barrier();
  uint8_t ack;
  helpers::tcp_write_until(c, &ack, sizeof(ack));
}

// Request:
//     |Opcode = Shutdown (1B)|
// Response:
//     |Ack (1B)|
void process_shutdown(tcpconn_t *c) {
  far_mem.reset();

  uint8_t ack;
  helpers::tcp_write_until(c, &ack, sizeof(ack));

  for (auto &thread : slave_threads) {
    thread.Join();
  }
  slave_threads.clear();
}

// Request:
// |Opcode = KOpReadObject(1B) | ds_id(1B) | obj_id_len(1B) | obj_id |
// Response:
// |data_len(2B)|data_buf(data_len B)|
void process_read_object(tcpconn_t *c) {
  uint8_t
      req[Object::kDSIDSize + Object::kIDLenSize + Object::kMaxObjectIDSize];
  uint8_t resp[Object::kDataLenSize + Object::kMaxObjectDataSize];

  helpers::tcp_read_until(c, req, Object::kDSIDSize + Object::kIDLenSize);
  auto ds_id = *const_cast<uint8_t *>(&req[0]);
  auto object_id_len = *const_cast<uint8_t *>(&req[Object::kDSIDSize]);
  auto *object_id = &req[Object::kDSIDSize + Object::kIDLenSize];
  helpers::tcp_read_until(c, object_id, object_id_len);

  auto *data_len = reinterpret_cast<uint16_t *>(&resp);
  auto *data_buf = &resp[Object::kDataLenSize];
  server.read_object(ds_id, object_id_len, object_id, data_len, data_buf);

  helpers::tcp_write_until(c, resp, Object::kDataLenSize + *data_len);
}

// Request:
// |Opcode = KOpWriteObject (1B)|ds_id(1B)|obj_id_len(1B)|data_len(2B)|
// |obj_id(obj_id_len B)|data_buf(data_len)|
// Response:
// |Ack (1B)|
void process_write_object(tcpconn_t *c) {
  uint8_t req[Object::kDSIDSize + Object::kIDLenSize + Object::kDataLenSize +
              Object::kMaxObjectIDSize + Object::kMaxObjectDataSize];

  helpers::tcp_read_until(
      c, req, Object::kDSIDSize + Object::kIDLenSize + Object::kDataLenSize);

  auto ds_id = *const_cast<uint8_t *>(&req[0]);
  auto object_id_len = *const_cast<uint8_t *>(&req[Object::kDSIDSize]);
  auto data_len = *reinterpret_cast<uint16_t *>(
      &req[Object::kDSIDSize + Object::kIDLenSize]);

  helpers::tcp_read_until(
      c, &req[Object::kDSIDSize + Object::kIDLenSize + Object::kDataLenSize],
      object_id_len + data_len);

  auto *object_id = const_cast<uint8_t *>(
      &req[Object::kDSIDSize + Object::kIDLenSize + Object::kDataLenSize]);
  auto *data_buf =
      const_cast<uint8_t *>(&req[Object::kDSIDSize + Object::kIDLenSize +
                                 Object::kDataLenSize + object_id_len]);

  server.write_object(ds_id, object_id_len, object_id, data_len, data_buf);

  uint8_t ack;
  helpers::tcp_write_until(c, &ack, sizeof(ack));
}

// Request:
// |Opcode = kOpRemoveObject (1B)|ds_id(1B)|obj_id_len(1B)|obj_id(obj_id_len B)|
// Response:
// |exists (1B)|
void process_remove_object(tcpconn_t *c) {
  uint8_t
      req[Object::kDSIDSize + Object::kIDLenSize + Object::kMaxObjectIDSize];

  helpers::tcp_read_until(c, req, Object::kDSIDSize + Object::kIDLenSize);
  auto ds_id = *const_cast<uint8_t *>(&req[0]);
  auto obj_id_len = *const_cast<uint8_t *>(&req[Object::kDSIDSize]);

  helpers::tcp_read_until(c, &req[Object::kDSIDSize + Object::kIDLenSize],
                          obj_id_len);

  auto *obj_id =
      const_cast<uint8_t *>(&req[Object::kDSIDSize + Object::kIDLenSize]);
  bool exists = server.remove_object(ds_id, obj_id_len, obj_id);

  helpers::tcp_write_until(c, &exists, sizeof(exists));
}

// Request:
// |Opcode = kOpConstruct (1B)|ds_type(1B)|ds_id(1B)|
// |param_len(1B)|params(param_len B)|
// Response:
// |Ack (1B)|
void process_construct(tcpconn_t *c) {
  uint8_t ds_type;
  uint8_t ds_id;
  uint8_t param_len;
  uint8_t *params;
  uint8_t req[sizeof(ds_type) + Object::kDSIDSize + sizeof(param_len) +
              std::numeric_limits<decltype(param_len)>::max()];

  helpers::tcp_read_until(
      c, req, sizeof(ds_type) + Object::kDSIDSize + sizeof(param_len));
  ds_type = *const_cast<uint8_t *>(&req[0]);
  ds_id = *const_cast<uint8_t *>(&req[sizeof(ds_type)]);
  param_len = *const_cast<uint8_t *>(&req[sizeof(ds_type) + Object::kDSIDSize]);
  helpers::tcp_read_until(
      c, &req[sizeof(ds_type) + Object::kDSIDSize + sizeof(param_len)],
      param_len);
  params = const_cast<uint8_t *>(
      &req[sizeof(ds_type) + Object::kDSIDSize + sizeof(param_len)]);

  server.construct(ds_type, ds_id, param_len, params);

  uint8_t ack;
  helpers::tcp_write_until(c, &ack, sizeof(ack));
}

// Request:
// |Opcode = kOpDeconstruct (1B)|ds_id(1B)|
// Response:
// |Ack (1B)|
void process_destruct(tcpconn_t *c) {
  uint8_t ds_id;

  helpers::tcp_read_until(c, &ds_id, Object::kDSIDSize);

  server.destruct(ds_id);

  uint8_t ack;
  helpers::tcp_write_until(c, &ack, sizeof(ack));
}

// Request:
// |Opcode = kOpCompute(1B)|ds_id(1B)|opcode(1B)|input_len(2B)|
// |input_buf(input_len)|
// Response:
// |output_len(2B)|output_buf(output_len B)|
void process_compute(tcpconn_t *c) {
  uint8_t opcode;
  uint16_t input_len;
  uint8_t req[Object::kDSIDSize + sizeof(opcode) + sizeof(input_len) +
              TCPDevice::kMaxComputeDataLen];

  helpers::tcp_read_until(
      c, req, Object::kDSIDSize + sizeof(opcode) + sizeof(input_len));

  auto ds_id = *reinterpret_cast<uint8_t *>(&req[0]);
  opcode = *reinterpret_cast<uint8_t *>(&req[Object::kDSIDSize]);
  input_len =
      *reinterpret_cast<uint16_t *>(&req[Object::kDSIDSize + sizeof(opcode)]);
  assert(input_len <= TCPDevice::kMaxComputeDataLen);

  if (input_len) {
    helpers::tcp_read_until(
        c, &req[Object::kDSIDSize + sizeof(opcode) + sizeof(input_len)],
        input_len);
  }

  auto *input_buf = const_cast<uint8_t *>(
      &req[Object::kDSIDSize + sizeof(opcode) + sizeof(input_len)]);

  uint16_t *output_len;
  uint8_t resp[sizeof(*output_len) + TCPDevice::kMaxComputeDataLen];
  output_len = reinterpret_cast<uint16_t *>(&resp[0]);
  uint8_t *output_buf = &resp[sizeof(*output_len)];
  server.compute(ds_id, opcode, input_len, input_buf, output_len, output_buf);

  helpers::tcp_write_until(c, resp, sizeof(*output_len) + *output_len);
}

// Request:
// |Opcode = kOpCall(1B)|ds_id(1B)|body_len(2B)|body(method+args)|
// Response:
// |ret_len(2B)|ret|
void process_call(tcpconn_t *c) {
//  FLOG("Start process call");
  uint16_t body_len;
  uint8_t req_header[Object::kDSIDSize + sizeof(body_len)];

  helpers::tcp_read_until(
      c, req_header, Object::kDSIDSize + sizeof(body_len));

  auto ds_id = *reinterpret_cast<uint8_t *>(&req_header[0]);
  body_len = *reinterpret_cast<uint16_t *>(&req_header[Object::kDSIDSize]);
  assert(body_len <= TCPDevice::kMaxCallDataLen);
//  FLOG("Read Header Success(ds_id: %d, body_len: %d)", ds_id, body_len);

  auto body_buffer = std::make_shared<rpc::Buffer>(body_len);

  if (body_len) {
    helpers::tcp_read_until( c, body_buffer->GetWritePtr(), body_len);
    body_buffer->HasWritten(body_len);
  }

  rpc::Serializer body_serializer(body_buffer);
  auto method = rpc::Get<std::string>(body_serializer);
//  FLOG("Read Body Success(method: %s)", method.c_str());
//  FLOG("Start Call method: %s", method.c_str());
//  auto start = std::chrono::steady_clock::now();

  rpc::BufferPtr ret_buffer;
  server.call(ds_id, method, body_buffer, ret_buffer);
//  auto end = std::chrono::steady_clock::now();
//  FLOG("Call method %s cost %ld us", method.c_str(),
//       std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

  uint16_t ret_len = ret_buffer->ReadableBytes(); // 没有处理大端小端
//  FLOG("Write Response(ret_len: %d)", ret_len);

  helpers::tcp_write2_until(c, &ret_len, sizeof(ret_len),
                            ret_buffer->GetReadPtr(), ret_len);
  // 理论上来说还应该加一步：ret_buffer.HasRead(ret_len),但不是必要的
}

void slave_fn(tcpconn_t *c) {
  // Run event loop.
  uint8_t opcode;
  int ret;
  while ((ret = tcp_read(c, &opcode, TCPDevice::kOpcodeSize)) > 0) {
    BUG_ON(ret != TCPDevice::kOpcodeSize);
//    FLOG("opcode: %d", opcode);
    switch (opcode) {
    case TCPDevice::kOpReadObject:
      process_read_object(c);
      break;
    case TCPDevice::kOpWriteObject:
      process_write_object(c);
      break;
    case TCPDevice::kOpRemoveObject:
      process_remove_object(c);
      break;
    case TCPDevice::kOpConstruct:
      process_construct(c);
      break;
    case TCPDevice::kOpDeconstruct:
      process_destruct(c);
      break;
    case TCPDevice::kOpCompute:
      process_compute(c);
      break;
    case TCPDevice::kOpCall:
      process_call(c);
      break;
    default:
      BUG();
    }
  }
  tcp_close(c);
}

void master_fn(tcpconn_t *c) {
  uint8_t opcode;
  helpers::tcp_read_until(c, &opcode, TCPDevice::kOpcodeSize);
  BUG_ON(opcode != TCPDevice::kOpInit);
  process_init(c);

  helpers::tcp_read_until(c, &opcode, TCPDevice::kOpcodeSize);
  BUG_ON(opcode != TCPDevice::kOpShutdown);
  process_shutdown(c);
  tcp_close(c);
  has_shutdown = true;
}

void do_work(uint16_t port) {
  log_file = fopen("/users/zhj/tcp_device_server.log", "w+");
  tcpqueue_t *q;
  struct netaddr server_addr = {.ip = 0, .port = port};
  tcp_listen(server_addr, 1, &q);

  tcpconn_t *c;
  while (tcp_accept(q, &c) == 0) {
    if (has_shutdown) {
      master_thread = rt::Thread([c]() { master_fn(c); });
      has_shutdown = false;
    } else {
      slave_threads.emplace_back([c]() { slave_fn(c); });
    }
  }
}

int argc;

void my_main(void *arg) {
  char **argv = static_cast<char **>(arg);
  int port = atoi(argv[1]);
  do_work(port);
}

int main(int _argc, char *argv[]) {
  int ret;

  if (_argc < 3) {
    std::cerr << "usage: [cfg_file] [port]" << std::endl;
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
