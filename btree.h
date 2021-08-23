#pragma once

#include <iostream>

using std::cout;
using std::endl;

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
  os << "[";
  for (auto& it : v) {
    os << it;
    // Compare pointer address rather than value when determining if
    // reaching the end Because a vector can contain duplicate values
    if (&it != &v.back())
      os << ", ";
  }
  os << "]";
  return os;
}

template <typename K, typename V>
class Entry {
 public:
  K m_Key;
  V m_Value;

  Entry(const K& key, const V& value) : m_Key(key), m_Value(value) {}

  Entry(const K&& key, const V&& value)
      : m_Key(std::move(key)), m_Value(std::move(value)) {}

  Entry(const Entry<K, V>& other)
      : m_Key(other.m_Key), m_Value(other.m_Value) {}

  Entry(Entry&& other) noexcept
      : m_Key(std::move(other.m_Key)), m_Value(std::move(other.m_Value)) {}

  Entry& operator=(const Entry<K, V>& other) {
    m_Key = other.m_Key;
    m_Value = other.m_Value;
    return *this;
  }

  Entry& operator=(Entry&& other) noexcept {
    m_Key = std::move(other.m_Key);
    m_Value = std::move(other.m_Value);
    return *this;
  }

  ~Entry() = default;

  void printInfo() {
    cout << "key: " << m_Key << ", value: " << m_Value << endl;
  }
};

// The tree has order of t, where each node can have [t, 2t] m_Children and
// [t-1, 2t-1] m_Keys (key-value pairs)
template <typename K, typename V>
class BTreeNode {
 private:
  int m_t;
  bool m_isLeaf;
  std::vector<Entry<K, V>> m_Entries;
  std::vector<std::shared_ptr<BTreeNode>> m_Children;

 public:
  BTreeNode(int t, bool isLeaf);

  int nEntries() const;

  int nChildren() const;

  void insertToNonFull(const K& key, const V& value);

  void splitChild(int fullChildIdx);

  V* getValuePtr(const K& key);

  int getIdxForKey(const K& key) const;

  void remove(const K& key);

  void removeFromLeaf(int idx);

  void removeFromNonLeaf(int idx);

  Entry<K, V> getPredEntry(int idx) const;

  Entry<K, V> getSuccEntry(int idx) const;

  void fillChild(int idx);

  void borrowEntryFromPrevChild(int idx);

  void borrowEntryFromNextChild(int idx);

  void mergeWithNextChild(int idx);

  const std::vector<Entry<K, V>> getAllEntries() const;

  void printNodeInfo() const;

  template <typename, typename>
  friend class BTree;
};

template <typename K, typename V>
class BTree {
 private:
  int m_t;
  std::shared_ptr<BTreeNode<K, V>> m_Root;

 public:
  explicit BTree(int t);

  ~BTree();

  void insert(const K& key, const V& value);

  V* getValuePtr(const K& key);

  void set(const K& key, const V& value);

  std::optional<V> get(const K& key);

  void remove(const K& key);

  std::vector<Entry<K, V>> getAllEntries() const;

  void printTreeInfo() const;

  void printAllEntries() const;
};

template <typename K, typename V>
BTreeNode<K, V>::BTreeNode(int t, bool isLeaf) : m_t(t), m_isLeaf(isLeaf) {
  m_Entries.reserve(2 * t - 1);
  m_Children.reserve(2 * t);
}

template <typename K, typename V>
int BTreeNode<K, V>::nEntries() const {
  return m_Entries.size();
}

template <typename K, typename V>
int BTreeNode<K, V>::nChildren() const {
  return m_Children.size();
}

template <typename K, typename V>
void BTreeNode<K, V>::insertToNonFull(const K& key, const V& value) {
  cout << "inserting entry to NODE, key:" << key << ", value:" << value << endl;
  int i = nEntries() - 1;

  if (m_isLeaf) {
    // The current node is leaf
    // At this moment, the node should have at least 1 empty spot for new
    // value, otherwise it would have been split by m_Parent. So we only
    // need to insert new value and key
    // 1. Find the location of new entry
    // 2. Insert new entry
    while (i >= 0 && m_Entries[i].m_Key > key)
      i--;
    m_Entries.emplace(m_Entries.begin() + i + 1, key, value);
  } else {
    // The current node is not leaf
    // Find the child which is going to have the new key
    // This child has index [i+1]
    while (i >= 0 && m_Entries[i].m_Key > key)
      i--;

    // Check if such child at (i+1) is 1 less than full (2t-1), split if so
    if (m_Children[i + 1]->nEntries() == 2 * m_t - 1) {
      splitChild(i + 1);

      // As the middle key of original child at [i+1] is now inserted to
      // this node's keys[i+1], check if key is bigger than that middle
      // key to decide if key should be inserted to current children[i+1]
      // (it now has less keys), or the new children[i+2].
      if (key > m_Entries[i + 1].m_Key)
        i++;
    }
    // Insert entry to proper child
    m_Children[i + 1]->insertToNonFull(key, value);
  }
}

template <typename K, typename V>
void BTreeNode<K, V>::splitChild(int fullChildIdx) {
  // fullChild will be split into 3 parts: t-1 keys, 1 key, t-1 keys
  // 1. The first t-1 keys remains in fullChild
  // 2. The last t-1 keys is put in a new split node
  // 3. The 1 key in the middle goes to parent (this node)
  cout << "node full, splitting..." << endl;

  // Get a reference of the full child's pointer
  std::shared_ptr<BTreeNode<K, V>>& fullChild = m_Children[fullChildIdx];

  // Create a new child to store last (t-1) m_Keys
  auto newChild =
      std::make_shared<BTreeNode<K, V>>(fullChild->m_t, fullChild->m_isLeaf);

  // Copy last (t-1) entries starting from index [t] from fullChild to
  // newChild Then delete those entries from fullChild
  for (int i = 0; i < m_t - 1; i++)
    newChild->m_Entries.push_back(fullChild->m_Entries[i + m_t]);
  fullChild->m_Entries.erase(fullChild->m_Entries.begin() + m_t,
                             fullChild->m_Entries.end());

  // Move the last t children of fullChild to newChild
  if (!fullChild->m_isLeaf) {
    for (int i = 0; i < m_t; i++)
      newChild->m_Children.push_back(fullChild->m_Children[i + m_t]);
    fullChild->m_Children.erase(fullChild->m_Children.begin() + m_t,
                                fullChild->m_Children.end());
  }

  // Create space and insert newChild
  m_Children.insert(m_Children.begin() + fullChildIdx + 1, newChild);

  // Create space and insert the middle key (and value) from fullChild
  m_Entries.insert(m_Entries.begin() + fullChildIdx,
                   fullChild->m_Entries[m_t - 1]);
  fullChild->m_Entries.erase(fullChild->m_Entries.begin() + m_t - 1);
}

template <typename K, typename V>
V* BTreeNode<K, V>::getValuePtr(const K& key) {
  // Find first key >= given key
  int i = 0;
  while (i < nEntries() && key > m_Entries[i].m_Key)
    i++;

  // Return value if found key. Need to check if i is still < m_nKeys
  if (i < nEntries() && m_Entries[i].m_Key == key) {
    cout << "found key: " << key << ", value: " << m_Entries[i].m_Value << endl;
    return &(m_Entries[i].m_Value);
  }

  // If key is not found and this is a leaf, return a nullptr
  if (m_isLeaf) {
    cout << "cannot find key: " << key << endl;
    return nullptr;
  }

  // If key is not found, search in child
  return m_Children[i]->getValuePtr(key);
}

template <typename K, typename V>
int BTreeNode<K, V>::getIdxForKey(const K& key) const {
  // Find first key >= given key
  int i = 0;
  while (i < nEntries() && key > m_Entries[i].m_Key)
    i++;
  return i;
}

template <typename K, typename V>
void BTreeNode<K, V>::remove(const K& key) {
  cout << "removing from NODE, key:" << key << endl;
  int keyIdx = getIdxForKey(key);

  // If the key is found right on this node
  if (keyIdx < nEntries() && m_Entries[keyIdx].m_Key == key) {
    if (m_isLeaf)
      // This node is a leaf node
      removeFromLeaf(keyIdx);
    else
      // This node is not a leaf node
      removeFromNonLeaf(keyIdx);
    return;
  }

  // If the key is not found on this node
  if (m_isLeaf) {
    cout << "cannot delete key: " << key << ", not found in tree" << endl;
    return;
  }

  // Need to check children nodes to remove given key

  // Flag indicating whether the key may be present in the last child node
  bool searchKeyInLastChild = (keyIdx == nEntries());

  // If the child where the key may exist has less that t keys, we fill that
  // child
  if (m_Children[keyIdx]->nEntries() < m_t)
    fillChild(keyIdx);

  // If the last child has been merged, it must have merged with the previous
  // child and so we recurse on the (idx-1)th child. Else, we recurse on the
  // (idx)th child which now has at least t keys
  if (searchKeyInLastChild && keyIdx > nEntries())
    m_Children[keyIdx - 1]->remove(key);
  else
    m_Children[keyIdx]->remove(key);
}

template <typename K, typename V>
void BTreeNode<K, V>::removeFromLeaf(int idx) {
  // Delete key and value pointer
  m_Entries.erase(m_Entries.begin() + idx);
}

template <typename K, typename V>
void BTreeNode<K, V>::removeFromNonLeaf(int idx) {
  if (m_Children[idx]->nEntries() >= m_t) {
    // If the child that holds key has at least t keys,
    // find the predecessor 'predKey' of key, replace key with predKey.
    // Recursively delete predKey from child idx
    Entry<K, V> predEntry = getPredEntry(idx);
    m_Entries[idx] = predEntry;
    m_Children[idx]->remove(predEntry.m_Key);
  } else if (m_Children[idx + 1]->nEntries() >= m_t) {
    // If the child at idx has less that t keys, examine child idx+1.
    // If child idx+1 has at least t keys, find the successor 'succKey' of
    // key in child idx+1, replace key by succKey. Recursively delete
    // succKey from child idx+1
    Entry<K, V> succEntry = getSuccEntry(idx);
    m_Entries[idx] = succEntry;
    m_Children[idx + 1]->remove(succEntry.m_Key);
  } else {
    // If both children[idx] and children[idx+1] have less that t keys,
    // merge everything on children[idx+1] into children[idx].
    // Now children[idx] contains 2t-1 keys
    // Free children[idx+1] and recursively delete key from children[idx]
    mergeWithNextChild(idx);
    m_Children[idx]->remove(m_Entries[idx].m_Key);
  }
}

template <typename K, typename V>
Entry<K, V> BTreeNode<K, V>::getPredEntry(int idx) const {
  // Keep moving to the right most node until we reach a leaf
  std::shared_ptr<BTreeNode<K, V>> cur = m_Children[idx];
  while (!cur->m_isLeaf)
    cur = cur->m_Children[cur->nEntries()];

  // Return the last key of the leaf
  return cur->m_Entries[cur->nEntries() - 1];
}

template <typename K, typename V>
Entry<K, V> BTreeNode<K, V>::getSuccEntry(int idx) const {
  // Keep moving the left most node starting from children[idx+1] until we
  // reach a leaf
  std::shared_ptr<BTreeNode<K, V>> cur = m_Children[idx + 1];
  while (!cur->m_isLeaf)
    cur = cur->m_Children[0];

  // Return the first key of the leaf
  return cur->m_Entries[0];
}

// Fill up the children[idx] if it has less than t-1 keys
template <typename K, typename V>
void BTreeNode<K, V>::fillChild(int idx) {
  if (idx != 0 && m_Children[idx - 1]->nEntries() >= m_t)
    // If the previous child children[idx-1] has more than t-1 keys, borrow
    // a key from that child
    borrowEntryFromPrevChild(idx);
  else if (idx != nEntries() && m_Children[idx + 1]->nEntries() >= m_t)
    // If the next child children[idx-1] has more than t-1 keys, borrow a
    // key from that child
    borrowEntryFromNextChild(idx);
  else {
    // If children[idx] is not the last child, merge it with its next child
    if (idx != nEntries())
      mergeWithNextChild(idx);
    else
      // If children[idx] is the last child, merge it with with its
      // previous child
      mergeWithNextChild(idx - 1);
  }
}

// Borrow an entry from the children[idx-1] node and put it in children[idx]
template <typename K, typename V>
void BTreeNode<K, V>::borrowEntryFromPrevChild(int idx) {
  std::shared_ptr<BTreeNode<K, V>> dest = m_Children[idx];
  std::shared_ptr<BTreeNode<K, V>> prev = m_Children[idx - 1];

  // The last key from children[idx-1] goes up to the parent and key[idx-1]
  // from parent is inserted as the first key in children[idx].
  dest->m_Entries.insert(dest->m_Entries.begin(), m_Entries[idx - 1]);

  // Moving prev child's last child as children[idx]'s first child
  if (!dest->m_isLeaf)
    dest->m_Children.insert(dest->m_Children.begin(),
                            prev->m_Children[prev->nEntries()]);

  // Moving the key from the prev child to the parent
  // This reduces the number of keys in the prev child
  m_Entries[idx - 1] = prev->m_Entries[prev->nEntries() - 1];
  prev->remove(prev->m_Entries[prev->nEntries() - 1].m_Key);
}

// A function to borrow an entry from the children[idx+1] and place it in
// children[idx]
template <typename K, typename V>
void BTreeNode<K, V>::borrowEntryFromNextChild(int idx) {
  std::shared_ptr<BTreeNode<K, V>> dest = m_Children[idx];
  std::shared_ptr<BTreeNode<K, V>> next = m_Children[idx + 1];

  // keys[idx] is inserted as the last key in children[idx]
  dest->m_Entries.push_back(m_Entries[idx]);

  // Sibling's first child is inserted as the last child
  // into children[idx]
  if (!(dest->m_isLeaf))
    dest->m_Children.push_back(next->m_Children[0]);

  // The first key from next child is inserted into children[idx]
  m_Entries[idx] = next->m_Entries[0];

  // Moving all keys in next child forward by 1
  next->m_Entries.erase(next->m_Entries.begin());

  // Moving children pointers on next child forward by 1
  if (!next->m_isLeaf) {
    next->m_Children.erase(next->m_Children.begin());
  }
}

// Merge children[idx] and children[idx+1]
template <typename K, typename V>
void BTreeNode<K, V>::mergeWithNextChild(int idx) {
  std::shared_ptr<BTreeNode<K, V>> child = m_Children[idx];
  std::shared_ptr<BTreeNode<K, V>> next = m_Children[idx + 1];

  // Pulling a key from the current node and inserting it into (t-1)th
  // position of children[idx]
  child->m_Entries.insert(child->m_Entries.begin() + m_t - 1, m_Entries[idx]);
  m_Entries.erase(m_Entries.begin() + idx);

  // Copying the keys from children[idx+1] to children[idx] at the end
  for (int i = 0; i < next->nEntries(); ++i) {
    child->m_Entries.push_back(next->m_Entries[i]);
  }

  // Copying the child pointers from children[idx+1] to children[idx]
  if (!child->m_isLeaf) {
    for (int i = 0; i <= next->nEntries(); ++i)
      child->m_Children.push_back(next->m_Children[i]);
  }

  // Moving the child pointers after (idx+1) in the current node forward by 1
  m_Children.erase(m_Children.begin() + idx + 1);
}

template <typename K, typename V>
const std::vector<Entry<K, V>> BTreeNode<K, V>::getAllEntries() const {
  if (m_isLeaf)
    return m_Entries;

  std::vector<Entry<K, V>> res;
  for (int i = 0; i < nEntries() + 1; i++) {
    // Merge children[i] result, nEntries+1 children
    std::vector<Entry<K, V>> childRes = m_Children[i]->getAllEntries();
    res.insert(res.end(), childRes.begin(), childRes.end());

    // Merge entries[i], nEntries entries
    if (i < nEntries())
      res.push_back(m_Entries[i]);
  }

  return res;
}

template <typename K, typename V>
void BTreeNode<K, V>::printNodeInfo() const {
  if (m_Entries.empty())
    return;

  // Print keys, values held on this node
  cout << "----" << (m_isLeaf ? "leaf " : "non-leaf") << "node with "
       << nEntries() << " key(s)----" << endl;
  for (int i = 0; i < nEntries(); i++) {
    cout << "key: " << m_Entries[i].m_Key << ", value: " << m_Entries[i].m_Value
         << endl;
  }

  // Print keys, values on children
  if (!m_isLeaf) {
    for (int i = 0; i < nEntries() + 1; i++) {
      m_Children[i]->printNodeInfo();
    }
  }
}

template <typename K, typename V>
BTree<K, V>::BTree(int t) : m_t(t), m_Root(nullptr) {}

template <typename K, typename V>
BTree<K, V>::~BTree() {
  cout << "deleting tree" << endl;
}

template <typename K, typename V>
void BTree<K, V>::insert(const K& key, const V& value) {
  cout << "inserting entry to TREE, key:" << key << ", value:" << value << endl;
  if (m_Root == nullptr) {
    // Empty tree, init root
    m_Root = std::make_shared<BTreeNode<K, V>>(m_t, true);
    m_Root->m_Entries.template emplace_back(key, value);
  } else {
    // Non empty tree
    if (m_Root->nEntries() == 2 * m_t - 1) {
      cout << "ROOT full, splitting..." << endl;

      // Grow height if root full
      auto newRoot = std::make_shared<BTreeNode<K, V>>(m_t, false);

      // Make old root as child or new root
      newRoot->m_Children.push_back(m_Root);

      // Split old root as two children of new root
      newRoot->splitChild(0);

      // New root now has middle key of old root and two children,
      // determine which child to insert new key, value
      int i = 0;
      if (key > newRoot->m_Entries[0].m_Key)
        i++;
      newRoot->m_Children[i]->insertToNonFull(key, value);

      // Take the new root
      m_Root = newRoot;
    } else {
      // Root not full, insert key, value to root
      m_Root->insertToNonFull(key, value);
    }
  }
}

template <typename K, typename V>
V* BTree<K, V>::getValuePtr(const K& key) {
  return (m_Root == nullptr) ? nullptr : m_Root->getValuePtr(key);
}

template <typename K, typename V>
const V getValue(const K& key) {}

template <typename K, typename V>
void BTree<K, V>::set(const K& key, const V& value) {
  V* pCrtValue = getValuePtr(key);
  if (pCrtValue != nullptr) {
    // Copy the new value to memory address
    // May have performance hit, but copy value can maintain continuous
    // memory allocation for **values array
    *pCrtValue = value;
  } else {
    insert(key, value);
  }
}

template <typename K, typename V>
std::optional<V> BTree<K, V>::get(const K& key) {
  V* valuePtr = getValuePtr(key);
  if (valuePtr == nullptr)
    return std::nullopt;
  return *(valuePtr);
}

template <typename K, typename V>
void BTree<K, V>::remove(const K& key) {
  cout << "removing from TREE, key:" << key << endl;
  if (m_Root == nullptr) {
    cout << "cannot remove key: " << key << ",the tree is empty";
    return;
  }

  // Call the remove function for root
  m_Root->remove(key);

  // If the root node has 0 keys, make its first child as the new root if it
  // has a child, otherwise set root as NULL
  if (m_Root->nEntries() == 0) {
    if (m_Root->m_isLeaf)
      m_Root = nullptr;
    else
      m_Root = m_Root->m_Children[0];
  }
}

template <typename K, typename V>
std::vector<Entry<K, V>> BTree<K, V>::getAllEntries() const {
  return m_Root->getAllEntries();
}

template <typename K, typename V>
void BTree<K, V>::printTreeInfo() const {
  cout << "\n----------tree info begins----------" << endl;
  m_Root->printNodeInfo();
  cout << "----------tree info ends----------\n" << endl;
}

template <typename K, typename V>
void BTree<K, V>::printAllEntries() const {
  std::vector<Entry<K, V>> entries = getAllEntries();
  cout << "\n----------all entries in tree begins----------" << endl;
  for (const Entry<K, V>& entry : entries) {
    cout << entry.m_Key << "->" << entry.m_Value << endl;
  }
  cout << "----------all entries in tree ends----------" << endl;
}
