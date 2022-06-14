#include <thread>
#include <iostream>
#include <time.h>
#include <vector>

#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include <boost/fiber/all.hpp>

rocksdb::Options options;
static rocksdb::WriteOptions write_options;
rocksdb::TransactionDBOptions txn_db_options;
rocksdb::TransactionDB* txn_db;

std::string kDBPath = "/tmp/rocksdb_donna";
using namespace rocksdb;
void fiberRun() {
  TransactionOptions txn_options;

  Slice value = "DIMITROULA";
  Slice key = "RANDOM";
  ReadOptions read_options;

  fprintf(stdout, "experiment starts\n");
  std::string val;
  rocksdb::Transaction* txn;
  for (int i = 0; i < 1000; i++) {
    txn = txn_db->BeginTransaction(write_options);
    std::cout << boost::this_fiber::get_id() << " 1" << "\n";
    txn->GetForUpdate(read_options, key, &val);
    std::cout << boost::this_fiber::get_id() << " 2\n";
   //  boost::this_fiber::sleep_for(std::chrono::milliseconds(std::rand() % 2));
    txn->Get(read_options, key, &val);
    std::cout << boost::this_fiber::get_id() << " 3\n";
    // boost::this_fiber::yield();
    txn->Put(key, value);
    std::cout << boost::this_fiber::get_id() << " 4\n";
    txn->Commit();
    std::cout << boost::this_fiber::get_id() << " 5\n";
    delete txn;
    boost::this_fiber::yield();
  }
  fprintf(stdout, "exepriment finished\n");
}


void threadRun() {
  std::vector<boost::fibers::fiber> fibers;
  int nb_fibers = 1;
  for (int i = 0; i < nb_fibers; i++) {
    fibers.emplace_back(boost::fibers::launch::post, fiberRun);
  }

  std::cout << "fibers launched .. good luck!\n";

  for (auto& fiber : fibers)
    fiber.join();

  std::cout << "all fibers joined\n";


}


int main(int argc, char* argv[]) {
  options.create_if_missing = true;
  rocksdb::Status s = rocksdb::TransactionDB::Open(options, txn_db_options, kDBPath, &txn_db);
  std::vector<std::thread> vec;

  for (int i = 0; i < 2; i++) {
    vec.emplace_back(std::thread(threadRun));
  }

  for (auto& thread : vec) {
    thread.join();
  }
}
