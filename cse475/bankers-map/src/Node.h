template<class K, class V>
class Node {
public:
  K key;
  volatile V val;
  Node<K, V>* next;
  Node(K key, V val): key(key), val(val), next(NULL) {}
};
