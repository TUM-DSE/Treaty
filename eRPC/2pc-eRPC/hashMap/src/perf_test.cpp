#include <string>
#include <sys/time.h>
#include <thread>
#include <vector>
#include "HashMap.h"


CTSL::HashMap<std::string, std::string> txn_map;

uint64_t Now()  {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

static void thread_run(void* ptr) {
  int* index = reinterpret_cast<int*>(ptr);
  std::cout << *index << "\n";
  auto start = Now();
  for (int i = 0; i < 1000000; i ++) {
    txn_map.insert(std::to_string(i* (*index)), std::to_string(i));
  }
  auto end = Now();

  std::cout << "Time elapsed " << (end-start) << "us\n";

  start = Now();
  for (int i = 0; i < 1000000; i ++) {
    txn_map.erase(std::to_string(i* (*index)));
  }
  end = Now();

  std::cout << "Time elapsed " << (end-start) << "us\n";
}


int main() {
  std::vector<std::thread> vec;
  std::vector<int> threads_args;

  for (size_t i = 1; i < 9; i++) {
    threads_args.push_back(i);
  }

  for (size_t i = 1; i < 9; i++) {
    vec.emplace_back(std::thread(thread_run, &threads_args[i-1]));
  }

  for (auto& thread : vec)
    thread.join();
  return 0;

}

