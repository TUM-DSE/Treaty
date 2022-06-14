#include <iostream>
#include <thread>
#include <vector>

#include <boost/fiber/all.hpp>

using namespace std;

boost::fibers::mutex m;
boost::fibers::condition_variable cv;
uint64_t counter = 0;

void helloFiber(std::vector<int> nums) {
  int i = 0;
  cout << "Hello, boost::fiber" <<    endl;
  while (i < 1000000) {
    std::unique_lock<boost::fibers::mutex> lk(m);
    counter++;
    boost::this_fiber::yield();
    i++;
  }

  for (auto i : nums) 
    std::cout << i << "\n";
}

void helloFiber1(std::vector<int>* nums, std::vector<int>* num1) {
  int i = 0;
  cout << "Hello, boost::fiber" <<    endl;
  while (i < 1000000) {
    std::unique_lock<boost::fibers::mutex> lk(m);
    boost::this_fiber::yield();
    // counter++;
    i++;
  }

  for (auto i : *nums) 
    std::cout << i << "\n";

  if (num1)
    for (auto i : *num1) 
      std::cout << i << "\n";
}


void waiting() {
  cout << "Hello, boost::fiber waiting" <<    endl;
  while (counter == 0) {
    std::unique_lock<boost::fibers::mutex> lk(m);
    cv.wait(lk);
  }
}

void increasing() {
  cout << "Hello, boost::fiber increasing" <<    endl;

  boost::this_fiber::sleep_for(std::chrono::milliseconds(10));
  for (int i = 0; i < 1000000; i++ ) {
    std::unique_lock<boost::fibers::mutex> lk(m);
    counter = 555;
  }
  cv.notify_all();
}

static void threadRun() {
  std::vector<int> vec = {1, 2, 3};
  std::vector<boost::fibers::fiber> fibers;
  fibers.emplace_back(boost::fibers::launch::post, helloFiber1, &vec, &vec);
  fibers.emplace_back(boost::fibers::launch::post, waiting);
  for (int i = 0; i < 20; i ++ )
    fibers.emplace_back(boost::fibers::launch::post, increasing);

  boost::fibers::fiber f(boost::fibers::launch::post, helloFiber1, &vec, nullptr);
  std::cout << f.joinable() << "\n";
  std::cout << "there\n";
  std::cout << counter << "\n";


  for (auto & fib : fibers)
    fib.join();

  f.join();


  std::cout << " ending " << counter << "\n";
}

int main() {
  std::vector<std::thread> threads_vec;
  for (int i = 0; i < 5; i++) 
    threads_vec.emplace_back(std::thread(threadRun));

  for (auto& thread : threads_vec)
    thread.join();

  return 0;
}
