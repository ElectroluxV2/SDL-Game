template <class T>
struct Vector {
  int allocatedSize;
  int count;
  T* root;
  typedef T* iterator;
  typedef T* const_iterator;

  Vector() {
    allocatedSize = 1;
    count = 0;
    root = new T[allocatedSize];
  }

  iterator begin() {
    return root;
  }
  const_iterator begin() const {
    return root;
  }

  iterator end() {
    return root + count;
  }
  const_iterator end() const {
    return root + count;
  }

  void Reallocate(int reallocateSize) {
    allocatedSize = reallocateSize;
    T* newRoot = new T[allocatedSize];
    memcpy(newRoot, root, count*sizeof(T));
    delete[] root;
    root = newRoot;
  }

  void push_back(T newElement) {
    if (count == allocatedSize) Reallocate(2 * allocatedSize);
    root[count] = newElement;
    count++;
  }

  T pop_back() {
    T toReturn = root[count];
    count--;
    if (count * 4 < allocatedSize) Reallocate(allocatedSize / 2);
    return toReturn;
  }

  T Get(int index) {
    if (index < 0 || index > count) return NULL;
    return root[index];
  }

  T* Next() {
    if (count == allocatedSize) Reallocate(2 * allocatedSize);
    return &root[++count];
  }
};