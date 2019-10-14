template<class K, class V>
class Node {
public:
  K key;
  volatile V val;
  Node<K, V>* next;
  char padding[48]; //assumes key = 4bytes, val = 4 bytes, next = 8bytes;
  Node(K key, V val): key(key), val(val), next(NULL) {}
};
