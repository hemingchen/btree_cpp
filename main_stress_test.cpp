#include <algorithm>  // for std::generate_n
#include <chrono>
#include <iostream>
#include <memory>

#include "btree.h"

#define TEST_COUNT 10000
#define RDM_STR_LEN 3

using std::cout;
using std::endl;
using std::ostream;
using std::shared_ptr;
using std::string;
using std::vector;

using K = std::string;
// using V = int;
using V = std::vector<int>;
// BTree degree t
const int t = 5;

// Generate random string of given length from given char set
std::string genRandomStr(size_t length) {
  auto randchar = []() -> char {
    const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    const size_t max_index = (sizeof(charset) - 1);
    return charset[std::rand() % max_index];
  };
  std::string str(length, 0);
  std::generate_n(str.begin(), length, randchar);
  return str;
}

void writeTest(BTree<K, V>* btree) {
  for (int count = 0; count < TEST_COUNT; count++) {
    // 1. Use int value
    // bt->set(genRandomStr(RDM_STR_LEN), std::rand());

    // 2. Use vector<int> value
    std::string key(genRandomStr(RDM_STR_LEN));
    std::vector<int> value(100, 0);
    generate(value.begin(), value.end(), rand);
    btree->set(key, value);
  }
}

void readTest(BTree<K, V>* btree) {
  for (int count = 0; count < TEST_COUNT; count++) {
    btree->get(genRandomStr(RDM_STR_LEN));
  }
}

int main() {
  srand(time(nullptr));

  auto* btree = new BTree<K, V>(t);

  // Start clock
  auto start = std::chrono::high_resolution_clock::now();

  // Write test
  { writeTest(btree); }

  // Get write to b-tree time
  auto writeEnd = std::chrono::high_resolution_clock::now();
  auto writeEllapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(writeEnd - start);

  // Read test
  { readTest(btree); }

  // Stop clock and get read from b-tree time
  auto readEnd = std::chrono::high_resolution_clock::now();
  auto readEllapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(readEnd - writeEnd);

  cout << endl
       << TEST_COUNT << " KVs, execution time:" << endl
       << "  write: " << writeEllapsed.count() * 1e-9 << " seconds" << endl
       << "  read:  " << readEllapsed.count() * 1e-9 << " seconds" << endl;

  delete btree;

  return 0;
}
