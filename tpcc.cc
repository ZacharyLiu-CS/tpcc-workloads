//
// tpcc.cc
//
// Created by Zacharyliu-CS on 07/12/2023.
// Copyright (c) 2023 liuzhenm@mail.ustc.edu.cn.
//

#include <gflags/gflags.h>
#include <atomic>
#include <ctime>
#include <iostream>
#include <thread>
#include "tpcc/config.h"
#include "tpcc/tpcc_tables.h"
#include "tpcc/tpcc_txn.h"

namespace TPCC {

DEFINE_bool(DEBUG, true, "Set if output log message.");
DEFINE_int32(NUM_WAREHOUSE, 2, "Set the num of warehouse.");
DEFINE_int32(FREQUENCY_NEW_ORDER, 45, "Default percentage of new-order txn.");
DEFINE_int32(FREQUENCY_PAYMENT, 43, "Default percentage of payment txn.");
DEFINE_int32(FREQUENCY_ORDER_STATUS, 4,
             "Default percentage of order-status txn.");
DEFINE_int32(FREQUENCY_DELIVERY, 4, "Default percentage of delivery txn.");
DEFINE_int32(FREQUENCY_STOCK_LEVEL, 4,
             "Default percentage of stock-level txn.");
DEFINE_string(DB_PATH, "/mnt/pmem0/tpccdb", "PATH of DB files stored");

}  // namespace TPCC

double RunTPCC(uint32_t txn_count,
               std::vector<TPCC::TPCCTxType>& tpcc_workgen_arr,
               TPCC::TPCCTable& tpcc_client) {
  // Guarantee that each coroutine has a different seed
  thread_local uint64_t seed =
      0xdeadbeef + std::hash<std::thread::id>()(std::this_thread::get_id());
  thread_local FastRandom random_generator(seed);

  struct timespec bench_start_time, bench_end_time;
  TPCC::TPCCTxn txn;

  clock_gettime(CLOCK_REALTIME, &bench_start_time);

  bool tx_committed = false;
  // Running transactions
  for (auto i = 0; i < txn_count; i++) {
    TPCC::TPCCTxType tx_type = tpcc_workgen_arr[Utils::FastRand(&seed) % 100];

    switch (tx_type) {
      case TPCC::TPCCTxType::kDelivery: {
        // printf("type: delivery   ");
        tx_committed = txn.Delivery(&tpcc_client, random_generator);
        break;
      }
      case TPCC::TPCCTxType::kNewOrder: {
        // printf("type: new order   ");
        tx_committed = txn.NewOrder(&tpcc_client, random_generator);
        break;
      }
      case TPCC::TPCCTxType::kOrderStatus: {
        // printf("type: order status   ");
        tx_committed = txn.OrderStatus(&tpcc_client, random_generator);
        break;
      }
      case TPCC::TPCCTxType::kPayment: {
        // printf("type: payment   ");
        tx_committed = txn.Payment(&tpcc_client, random_generator);
        break;
      }
      case TPCC::TPCCTxType::kStockLevel: {
        // printf("type: stock level   ");
        tx_committed = txn.StockLevel(&tpcc_client, random_generator);
        break;
      }
      default:
        printf("Unexpected transaction type %d\n", static_cast<int>(tx_type));
        abort();
    }
    // printf("\t transaction count: %d", i);
    // printf(", tpcc get record count: %lu, put record count: %lu \n", tpcc_client.GetReadRecordCount(), tpcc_client.GetLoadRecordCount());

  }
  clock_gettime(CLOCK_REALTIME, &bench_end_time);
  double bench_sec =
      (bench_end_time.tv_sec - bench_start_time.tv_sec) +
      (double)(bench_end_time.tv_nsec - bench_start_time.tv_nsec) / 1000000000;
  return bench_sec;
}
int main(int argc, char** argv) {
  google::SetUsageMessage("Usage message of pmem operation test:");
  google::ParseCommandLineFlags(&argc, &argv, true);
  TPCC::TestConfig();

  TPCC::TPCCTable tpcc_client(TPCC::DBType::rocksdb);
  std::vector<TPCC::TPCCTxType> tpcc_workgen_arr =
      tpcc_client.CreateWorkgenArray();
  tpcc_client.LoadTables();
  uint64_t txn_count = 100000;
  double benchsec = RunTPCC(100000, tpcc_workgen_arr, tpcc_client);
  printf("transaction count = %ld, sec = %.2lf, tmpC = %.2lf\n", txn_count, benchsec, txn_count*60/benchsec);
}
