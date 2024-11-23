#include "intern_list.hpp"

#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace {

static const size_t PAGE_SIZE = sysconf(_SC_PAGE_SIZE); // this is always 4096 bytes on linux

} // namespace

namespace scp {

size_t InternList::pageSize() {
    return PAGE_SIZE;
}

InternList::InternList(const size_t nPages, bool align)
    : bufferSize(nPages * PAGE_SIZE), wantAligment(align) {
    this->buffer = static_cast<char*>(mmap(nullptr, this->bufferSize, PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (MAP_FAILED == this->buffer) {
        char msg[256];
        snprintf(msg, 256, "mmap failed to allocate buffer: %s", strerrorname_np(errno));
        throw std::runtime_error(msg);
    }
}

InternList::~InternList() {
    munmap(this->buffer, this->bufferSize);
}

void InternList::freeze(size_t n) {
    if (mprotect(this->buffer, n ? n : this->bufferSize, PROT_READ) != 0) {
        char msg[256];
        snprintf(msg, 256, "mprotect failed to set permissions on buffer: %s",
                 strerrorname_np(errno));
        throw std::runtime_error(msg);
    }
}

InternedStringView InternList::intern(std::string_view str) {
    const auto existing = this->find(str);
    if (existing.has_value()) {
        return existing.value();
    }

    if (this->endOfList + str.length() > this->bufferSize) {
        throw std::runtime_error("attempt to allocate past end of buffer");
    }
    char* const start = this->buffer + this->endOfList;
    std::memcpy(start, str.data(), str.length());
    this->views.emplace(start, str.length());
    this->endOfList += str.length();
    // add a null terminator for compatibility with C string APIs
    this->buffer[this->endOfList] = '\0';
    ++this->endOfList;

    if (this->wantAligment) {
        size_t misalign = this->endOfList % 64;
        if (misalign > 0) {
            this->endOfList += 64 - misalign;
        }
    }
    return InternedStringView(start, str.size());
}

std::optional<InternedStringView> InternList::find(const std::string_view str) const {

    const auto found = this->views.find(str);
    if (this->views.end() != found) {
        InternedStringView ret(*found);
        return {ret};
    }
    return {};
}

} // namespace scp
