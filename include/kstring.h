#include <string>
#include <iostream>
#include <algorithm>
#include <type_traits>

template<typename T>
struct kchar_traits { };

template<>
struct kchar_traits<char> 
{
  using char_type = char;
  using int_type = int;
  using off_type = std::size_t;
  using pos_type = std::size_t;

  static constexpr std::size_t length(const char_type* str)
  {
    return std::strlen(str);
  }

  static constexpr void copy(char_type* dest, const char_type* src, std::size_t count)
  {
    std::copy(src, src + count, dest);
  }

  static constexpr void move(char_type* dest, const char_type* src, std::size_t count)
  {
    std::move(src, src + count, dest);
  }
};

template<class, class = void>
struct is_iterator : std::false_type { };

template<class T>
struct is_iterator<T, decltype(std::iterator_traits<T>::iterator_category(), void())> : std::true_type { };

template<typename T>
constexpr bool is_iterator_v = is_iterator<T>::value;

template<typename Elem, typename Traits = std::char_traits<Elem>>
class kbasic_string
{
public:
  static const std::size_t npos = -1;
  using iterator = Elem*;
  using const_iterator = const Elem*;
  using pointer = Elem*;
  using reference = Elem&;
  using const_reference = const Elem&;

  friend kbasic_string operator+(const kbasic_string& lhs, const kbasic_string& rhs)
  {
    return kbasic_string{lhs}.append(rhs);
  }

  friend kbasic_string operator+(const kbasic_string& lhs, const Elem* rhs)
  {
    return kbasic_string{lhs}.append(rhs);
  }

  friend kbasic_string operator+(const kbasic_string& lhs, Elem rhs)
  {
    return kbasic_string{lhs}.append(rhs);
  }

  friend std::ostream& operator<<(std::ostream& lhs, const kbasic_string& rhs)
  {
    lhs.write(rhs.data(), rhs.size());
    return lhs;
  }

  friend std::wostream& operator<<(std::wostream& lhs, const kbasic_string& rhs)
  {
    lhs.write(rhs.data(), rhs.size());
    return lhs;
  }

  kbasic_string()
  {
    set_size(0);
  }

  kbasic_string(const kbasic_string& other)
  {
    std::size_t size_curr = other.size();
    reserve(other.capacity());
    if (size_curr)
      Traits::copy(begin(), other.begin(), size_curr + 1);
    set_size(size_curr);
  }

  kbasic_string(kbasic_string&& other)
  {
    if (other.on_heap())
    {
      set_heap_ptr(other.heap_ptr());
      other.set_heap_ptr(nullptr);
    }
    else
    {
      Traits::copy(begin(), other.begin(), other.size() + 1);
    }
    set_on_heap(other.on_heap());
    set_size(other.size());
    set_capacity(other.capacity());
    other.set_on_heap(false);
  }

  kbasic_string& operator=(const kbasic_string& other)
  {
    std::size_t size = other.size();
    reserve(other.capacity());
    if (size)
      Traits::copy(begin(), other.begin(), size + 1);
    set_size(size);
    return *this;
  }

  kbasic_string& operator=(kbasic_string&& other)
  {
    if (other.on_heap())
    {
      set_heap_ptr(other.heap_ptr());
      other.set_heap_ptr(nullptr);
      if (!on_heap())
        set_on_heap(true);
    }
    else
    {
      Traits::copy(begin(), other.begin(), other.size() + 1);
    }
    set_size(other.size());
    set_capacity(other.capacity());
    other.set_on_heap(false);
    return *this;
  }

  kbasic_string(const Elem* str)
  {
    std::size_t size = Traits::length(str);
    reserve(size);
    Traits::copy(begin(), str, size + 1);
    set_size(size);
  }

  kbasic_string(const_iterator first, const_iterator last)
  {
    std::size_t size_curr = last - first;
    reserve(size_curr);
    Traits::copy(begin(), first, size_curr + 1);
    data()[size_curr] = 0;
    set_size(size_curr);
  }

  kbasic_string& operator=(const Elem* str)
  {
    std::size_t size = Traits::length(str);
    reserve(size);
    Traits::copy(begin(), str, size + 1);
    set_size(size);
    return *this;
  }

  void reserve(std::size_t cap)
  {
    if (cap <= capacity() || cap <= 23)
      return;
    std::size_t size_curr = size();
    Elem* mem = new Elem[cap + 1];
    if (size_curr)
      Traits::copy(mem, begin(), size_curr + 1);
    if (on_heap())
    {
      delete[] data();
    }
    else
    {
      set_on_heap(true);
      set_size(size_curr);
    }
    set_heap_ptr(mem);
    set_capacity(cap);
  }

  void resize(std::size_t sze, Elem fill)
  {
    std::size_t size_curr = size();
    if (sze > size_curr)
    {
      reserve(sze);
      iterator end_curr = end();
      std::fill(end_curr, end_curr + (sze - size_curr), fill);
      data()[sze] = 0;
    }
    else if (sze < size_curr)
    {
      erase(begin() + sze, end());
    }
    else
    {
      return;
    }
    set_size(sze);
  }

  void resize(std::size_t size)
  {
    resize(size, Elem{});
  }

  kbasic_string& erase(std::size_t pos, std::size_t count)
  {
    std::size_t size_curr = size();
    if (pos > size_curr)
      throw std::out_of_range("pos is bigger than size");
    iterator begin_curr = begin();
    if (count < size_curr)
      Traits::copy(begin_curr + pos, begin_curr + pos + count, (size_curr + 1) - (pos + count));
    else
      *begin_curr = 0;
    set_size(size_curr - count);
    return *this;
  }

  kbasic_string& erase(const_iterator it)
  {
    return erase(it - begin(), 1);
  }

  kbasic_string& erase(const_iterator first, const_iterator last)
  {
    return erase(first - begin(), last - first);
  }

  void pop_back()
  {
    erase(end() - 1);
  }

  void clear()
  {
    erase(begin(), end());
  }

  kbasic_string& append(const Elem* str, std::size_t len)
  {
    std::size_t size_curr = size();
    iterator begin_curr = begin();
    iterator end_curr = end();
    bool inside = std::any_of(begin_curr, end_curr, [&str](const Elem& val) { return &val == str; });
    std::size_t offset = 0;
    if (inside)
      offset = str - begin_curr;
    reserve(size_curr + len);
    if (inside)
      Traits::copy(begin() + size_curr, begin() + offset, len);
    else
      Traits::copy(begin() + size_curr, str, len);
    set_size(size_curr + len);
    data()[size_curr + len] = Elem();
    return *this;
  }

  kbasic_string& append(const Elem* str)
  {
    return append(str, Traits::length(str));
  }

  kbasic_string& append(const kbasic_string& str)
  {
    return append(str.data(), str.size());
  }

  kbasic_string& append(Elem ch)
  {
    return append(&ch, 1);
  }

  template<typename Iter, typename = std::enable_if_t<is_iterator_v<Iter>>>
  kbasic_string& append(const Iter first, const Iter last)
  {
    return append(first, last - first);
  }

  void push_back(Elem ch)
  {
    append(ch);
  }

  kbasic_string& insert(std::size_t pos, const Elem* str, std::size_t len)
  {
    std::size_t size_curr = size();
    if (pos > size_curr)
      throw std::out_of_range("pos is bigger than size");
    iterator begin_curr = begin();
    iterator end_curr = end();
    bool inside = std::any_of(begin_curr, end_curr, [&str](const Elem& val) { return &val == str; });
    std::size_t offset = 0;
    if (inside)
      offset = str - begin_curr;
    reserve(size_curr + len);
    begin_curr = begin();
    Traits::copy(begin_curr + pos + len, begin_curr + pos, end() - (begin_curr + pos));
    if (inside)
      Traits::copy(begin_curr + pos, begin_curr + offset, len);
    else
      Traits::copy(begin_curr + pos, str, len);
    set_size(size_curr + len);
    return *this;
  }

  kbasic_string& insert(std::size_t pos, const Elem* str)
  {
    return insert(pos, str, Traits::length(str));
  }

  kbasic_string& insert(std::size_t pos, const kbasic_string& str)
  {
    return insert(pos, str.data(), str.size());
  }

  kbasic_string& insert(std::size_t pos, Elem ch)
  {
    return insert(pos, &ch, 1);
  }

  template<typename Iter, typename = std::enable_if_t<is_iterator_v<Iter>>>
  kbasic_string& insert(std::size_t pos, const Iter first, const Iter last)
  {
    return insert(pos, first, last - first);
  }

  kbasic_string& assign(std::size_t sze, Elem c)
  {
    clear();
    resize(sze, c);
    return *this;
  }

  kbasic_string& assign(const kbasic_string& str)
  {
    return *this = str;
  }

  kbasic_string& assign(kbasic_string&& str)
  {
    return *this = std::move(str);
  }

  kbasic_string& assign(const Elem* str, std::size_t len)
  {
    return *this = str;
  }

  kbasic_string& assign(const Elem* str)
  {
    return *this = str;
  }

  std::size_t find(const Elem* str, std::size_t offset = 0) const noexcept
  {
    std::size_t other_size = Traits::length(str);
    std::size_t this_size = size();
    if (offset > this_size)
      throw std::out_of_range("pos is bigger than size");
    for (int i = offset; i < (this_size + 1) - other_size; ++i)
      if (std::equal(begin() + i, begin() + i + other_size, str, str + other_size))
        return i;
    return npos;
  }

  std::size_t find(const kbasic_string& str, std::size_t offset = 0) const noexcept
  {
    return find(str.data(), offset);
  }

  kbasic_string substr(std::size_t pos = 0, std::size_t sze = 0) const
  {
    return kbasic_string(begin() + pos, begin() + sze);
  }

  reference operator[](std::size_t n)
  {
    return at(n);
  }

  const_reference operator[](std::size_t n) const
  {
    return at(n);
  }

  kbasic_string& operator+=(const kbasic_string& other)
  {
    return append(other);
  }

  kbasic_string& operator+=(const Elem* str)
  {
    return append(str);
  }

  kbasic_string& operator+=(Elem ch)
  {
    return append(ch);
  }

  bool operator==(const kbasic_string& other) const noexcept
  {
    return std::equal(begin(), end(), other.begin(), other.end());
  }

  bool operator==(const Elem* str) const noexcept
  {
    return std::equal(begin(), end(), str, str + Traits::length(str));
  }

  bool operator==(Elem ch) const noexcept
  {
    return size() == 1 && *begin() == ch;
  }

  bool operator!=(const kbasic_string& other) const noexcept
  {
    return !(*this == other);
  }

  bool operator!=(const Elem* str) const noexcept
  {
    return !(*this == str);
  }

  bool operator!=(Elem ch) const noexcept
  {
    return !(*this == ch);
  }

  Elem* data() noexcept
  {
    return on_heap() ? heap_ptr() : reinterpret_cast<Elem*>(&data_);
  }

  const Elem* data() const noexcept
  {
    return on_heap() ? heap_ptr() : reinterpret_cast<const Elem*>(&data_);
  }

  const Elem* c_str() const noexcept
  {
    return data();
  }

  std::size_t size() const noexcept
  {
    if (on_heap())
#ifdef _WIN64
      return ((data_.long_string.size << 8) >> 8);
#else
      return data_.long_string.size;
#endif
    else
      return 23 - (data_.short_string.size >> 1);
  }

  std::size_t length() const noexcept
  {
    return size();
  }

  std::size_t capacity() const noexcept
  {
    if (on_heap())
      return data_.long_string.capacity;
    else
      return 23;
  }

  iterator begin() noexcept
  {
    return data();
  }

  const_iterator begin() const noexcept
  {
    return data();
  }

  iterator end() noexcept
  {
    return data() + size();
  }

  const_iterator end() const noexcept
  {
    return data() + size();
  }

  bool empty() const noexcept
  {
    return !size();
  }

  reference at(std::size_t n)
  {
    return data()[n];
  }

  const_reference at(std::size_t n) const
  {
    return data()[n];
  }

  ~kbasic_string()
  {
    if (on_heap())
      delete[] data();
  }
private:
  void set_size(std::size_t value)
  {
    if (on_heap())
#ifdef _WIN64
      data_.long_string.size = value | (1ull << (sizeof(std::size_t) * 7));
#else
      data_.long_string.size = value;
#endif
    else
      data_.short_string.size = ((23 - value) << 1);
  }

  void set_capacity(std::size_t value)
  {
    if (on_heap())
      data_.long_string.capacity = value;
  }

  void set_on_heap(bool value) noexcept
  {
    if (value == on_heap())
      return;
    data_.short_string.size ^= 1;
  }

  void set_heap_ptr(Elem* ptr)
  {
    data_.long_string.ptr = ptr;
  }

  bool on_heap() const noexcept
  {
    return data_.short_string.size & 1;
  }

  const Elem* heap_ptr() const noexcept
  {
    return data_.long_string.ptr;
  }

  Elem* heap_ptr() noexcept
  {
    return data_.long_string.ptr;
  }

  union
  {
    struct
    {
      Elem buffer[23];
      Elem size;
    } 
    short_string;
    struct
    {
      Elem* ptr;
      std::size_t capacity;
      std::size_t size;
    } 
    long_string;
  } 
  data_ = {};
};

using kstring = kbasic_string<char>;