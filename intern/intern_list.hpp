#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace scp {

class InternedStringView {
  private:
    InternedStringView(std::string_view str) : sv(str) {
    }
    InternedStringView(const char* data, size_t len) : sv(data, len) {
    }
    std::string_view sv;
    friend class InternList;
    friend class InternStringIterator;

  public:
    size_t size() const {
        return sv.size();
    }
    const char* data() const {
        return sv.data();
    }
    const char* c_str() const {
        return sv.data();
    }
    bool operator==(const InternedStringView& other) const {
        return this->data() == other.data();
    }
    std::string toString() const {
        return std::string(this->sv);
    }
    uint64_t hash() const {
        static_assert(sizeof(void*) == sizeof(uint64_t), "non 64-bit platforms unsupported");
        uint64_t x = reinterpret_cast<uint64_t>(this->sv.data());
        // #https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
        // x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        // x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
        // x = x ^ (x >> 31);
        return x;
    }
};

} // namespace scp

namespace std {
template <> struct hash<scp::InternedStringView> {
    std::size_t operator()(const scp::InternedStringView& sv) const {
        return sv.hash();
    }
};
} // namespace std

namespace scp {

class InternStringIterator {
  private:
    using DelegateIterator = std::unordered_set<std::string_view>::const_iterator;
    DelegateIterator iter;
    mutable InternedStringView buffer;

    explicit InternStringIterator(DelegateIterator it) : iter(it), buffer(nullptr, 0) {
    }
    friend class InternList;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = InternedStringView;
    using difference_type = size_t;
    using pointer = InternedStringView*;
    using reference = InternedStringView&;

    InternStringIterator& operator++() {
        ++this->iter;
        return *this;
    }
    InternStringIterator operator++(int) {
        InternStringIterator retval = *this;
        ++(*this);
        return retval;
    }
    bool operator==(InternStringIterator other) const {
        return this->iter == other.iter;
    }
    bool operator!=(InternStringIterator other) const {
        return !(*this == other);
    }
    reference operator*() const {
        this->buffer = *this->iter;
        return this->buffer;
    }
};

class InternList {
  private:
    size_t bufferSize;
    char* buffer;
    std::unordered_set<std::string_view> views;
    size_t endOfList = 0;
    bool wantAligment;

  public:
    static size_t pageSize();

    using const_iterator = InternStringIterator;

    InternList(const size_t nPages, bool aligned);
    InternList(const size_t nPages) : InternList(nPages, false) {
    }
    ~InternList();

    InternList(const InternList&) = delete;
    InternList(InternList&&) = delete;

    InternList& operator=(const InternList&) = delete;
    InternList& operator=(InternList&&) = delete;

    const_iterator begin() const noexcept {
        return InternStringIterator(this->views.begin());
    }

    const_iterator end() const noexcept {
        return InternStringIterator(this->views.end());
    }

    InternedStringView intern(std::string_view str);

    std::optional<InternedStringView> find(std::string_view str) const;

    std::vector<std::string_view> getContents() const {
        return {this->views.begin(), this->views.end()};
    }

    void freeze(size_t n);
    void unfreeze();

    size_t size() const {
        return this->views.size();
    }
};

} // namespace scp
