#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <atomic>
#include <memory>
#include <vector>

namespace sparo {
// Helper to transfer ownership of a raw pointer to a std::unique_ptr<T>.
// Note that std::unique_ptr<T> has very different semantics from
// std::unique_ptr<T[]>: do not use this helper for array allocations.
template <typename T>
std::unique_ptr<T> WrapUnique(T* ptr) {
  return std::unique_ptr<T>(ptr);
}

// Round up |size| to a multiple of alignment, which must be a power of two.
template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr T AlignUp(T size, T alignment) {
  // DCHECK(IsPowerOfTwo(alignment));
  return (size + alignment - 1) & ~(alignment - 1);
}
// Advance |ptr| to the next multiple of alignment, which must be a power of
// two. Defined for types where sizeof(T) is one byte.
template <typename T, typename = std::enable_if_t<sizeof(T) == 1>>
inline T* AlignUp(T* ptr, uintptr_t alignment) {
  return reinterpret_cast<T*>(
      AlignUp(reinterpret_cast<uintptr_t>(ptr), alignment));
}

constexpr size_t RoundUpToWordBoundary(size_t size) {
  return AlignUp(size, sizeof(void*));
}

}  // namespace sparo

// SharedBuffer is an abstraction around a region of shared memory, it has
// methods to facilitate safely reading and writing into the shared region.
// SharedBuffer only handles the access to the shared memory any notifications
// must be performed separately.
class SharedBuffer {
 public:
  SharedBuffer(SharedBuffer&& other) = default;

  SharedBuffer(const SharedBuffer&) = delete;
  SharedBuffer& operator=(const SharedBuffer&) = delete;

  ~SharedBuffer() { reset(); }

  enum class Error { kSuccess = 0, kGeneralError = 1, kControlCorruption = 2 };

  static std::unique_ptr<SharedBuffer> Create(const int& memfd, size_t size) {
    if (memfd < 0) {
      return nullptr;
    }

    // Enforce the system shared memory security policy.
    // if
    // (!base::SharedMemorySecurityPolicy::AcquireReservationForMapping(size)) {

    uint8_t* ptr = reinterpret_cast<uint8_t*>(
        mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0));

    if (ptr == MAP_FAILED) {
      return nullptr;
    }

    return sparo::WrapUnique<SharedBuffer>(new SharedBuffer(ptr, size));
  }

  uint8_t* usable_region_ptr() const { return base_ptr_ + kReservedSpace; }
  size_t usable_len() const { return len_ - kReservedSpace; }
  bool is_valid() const { return base_ptr_ != nullptr && len_ > 0; }

  void reset() {
    if (is_valid()) {
      if (munmap(base_ptr_, len_) < 0) {
        // PLOG(ERROR) << "Unable to unmap shared buffer";
        return;
      }

      // base::SharedMemorySecurityPolicy::ReleaseReservationForMapping(len_);
      base_ptr_ = nullptr;
      len_ = 0;
    }
  }

  // Only one side should call Initialize, this will initialize the first
  // sizeof(ControlStructure) bytes as our control structure. This should be
  // done when offering fast comms.
  void Initialize() { new (static_cast<void*>(base_ptr_)) ControlStructure; }

  // TryWrite will attempt to append |data| of |len| to the shared buffer, this
  // call will only succeed if there is no one else trying to write AND there is
  // enough space currently in the buffer.
  Error TryWrite(const void* data, size_t len) {
    // // DCHECK(data);
    // // DCHECK(len);

    if (len > usable_len()) {
      return Error::kGeneralError;
    }

    if (!TryLockForWriting()) {
      return Error::kGeneralError;
    }

    // At this point we know that the space available can only grow because
    // we're the only writer we will write from write_pos -> end and 0 -> (len
    // - (end - write_pos)) where end is usable_len().
    uint32_t cur_read_pos = read_pos().load();
    uint32_t cur_write_pos = write_pos().load();

    if (!ValidateReadWritePositions(cur_read_pos, cur_write_pos)) {
      UnlockForWriting();
      return Error::kControlCorruption;
    }

    uint32_t space_available =
        usable_len() - NumBytesInUse(cur_read_pos, cur_write_pos);

    if (space_available <= len) {
      UnlockForWriting();

      return Error::kGeneralError;
    }

    // If we do not have enough space from the current write position to the end
    // then we will be forced to wrap around. If we do have enough space we can
    // just start writing at the write position, otherwise we start writing at
    // the write position up to the end of the usable area and then we write the
    // remainder of the payload starting at position 0.
    if ((usable_len() - cur_write_pos) > len) {
      memcpy(usable_region_ptr() + cur_write_pos, data, len);
    } else {
      size_t copy1_len = usable_len() - cur_write_pos;
      memcpy(usable_region_ptr() + cur_write_pos, data, copy1_len);
      memcpy(usable_region_ptr(),
             reinterpret_cast<const uint8_t*>(data) + copy1_len,
             len - copy1_len);
    }

    // Atomically update the write position.
    // We also verify that the write position did not advance, it SHOULD NEVER
    // advance since we were holding the write lock.
    if (write_pos().exchange((cur_write_pos + len) % usable_len()) !=
        cur_write_pos) {
      UnlockForWriting();
      return Error::kControlCorruption;
    }

    UnlockForWriting();

    return Error::kSuccess;
  }

  Error TryReadLocked(void* data, uint32_t len, uint32_t* bytes_read) {
    uint32_t cur_read_pos = read_pos().load();
    uint32_t cur_write_pos = write_pos().load();

    if (!ValidateReadWritePositions(cur_read_pos, cur_write_pos)) {
      return Error::kControlCorruption;
    }

    // The most we can read is the smaller of what's in use in the shared memory
    // usable area and the buffer size we've been passed.
    uint32_t bytes_available_to_read =
        NumBytesInUse(cur_read_pos, cur_write_pos);
    bytes_available_to_read = std::min(bytes_available_to_read, len);
    if (bytes_available_to_read == 0) {
      *bytes_read = 0;
      return Error::kSuccess;
    }

    // We have two cases when reading, the first is the read position is behind
    // the write position, in that case we can simply read all data between the
    // read and write position (up to our buffer size). The second case is when
    // the write position is behind the read position. In this situation we must
    // read from the read position to the end of the available area, and
    // continue reading from the 0 position up to the write position or the
    // maximum buffer size (bytes_available_to_read).
    if (cur_read_pos < cur_write_pos) {
      memcpy(data, usable_region_ptr() + cur_read_pos, bytes_available_to_read);
    } else {
      // We first start by reading to the end of the the usable area, if we
      // cannot read all the way (because our buffer is too small, we're done).
      uint32_t bytes_from_read_to_end = usable_len() - cur_read_pos;
      bytes_from_read_to_end =
          std::min(bytes_from_read_to_end, bytes_available_to_read);
      memcpy(data, usable_region_ptr() + cur_read_pos, bytes_from_read_to_end);

      if (bytes_from_read_to_end < bytes_available_to_read) {
        memcpy(reinterpret_cast<uint8_t*>(data) + bytes_from_read_to_end,
               usable_region_ptr(),
               bytes_available_to_read - bytes_from_read_to_end);
      }
    }

    // Atomically update the read position.
    // We also verify that the read position did not advance, it SHOULD NEVER
    // advance since we were holding the read lock.
    uint32_t new_read_pos =
        (cur_read_pos + bytes_available_to_read) % usable_len();
    if (read_pos().exchange(new_read_pos) != cur_read_pos) {
      *bytes_read = 0;
      return Error::kControlCorruption;
    }

    *bytes_read = bytes_available_to_read;
    return Error::kSuccess;
  }

  bool TryLockForReading() {
    // We return true if we set the flag (meaning it was false).
    return !read_flag().test_and_set(std::memory_order_acquire);
  }

  void UnlockForReading() { read_flag().clear(std::memory_order_release); }

 private:
  struct ControlStructure {
    std::atomic_flag write_flag{false};
    std::atomic_uint32_t write_pos{0};

    std::atomic_flag read_flag{false};
    std::atomic_uint32_t read_pos{0};

    // If we're using a notification mechanism that relies on futex, make the
    // space available for one, if not these 32bits are unused. The kernel
    // requires they be 32bit aligned.
    alignas(4) volatile uint32_t futex = 0;
  };

  // This function will only validate that the values provided for write and
  // read positions are valid based on usable size of the shared memory region.
  // This should ALWAYS be called before attempting a write or read using
  // atomically loaded values from the control structure.
  bool ValidateReadWritePositions(uint32_t read_pos, uint32_t write_pos) {
    // The only valid values for read and write positions are [0 - usable_len
    // - 1].
    if (write_pos >= usable_len()) {
      // LOG(ERROR) << "Write position of shared buffer is currently beyond the
      // "
      //               "usable length";
      return false;
    }

    if (read_pos >= usable_len()) {
      // LOG(ERROR) << "Read position of shared buffer is currently beyond the "
      //               "usable length";
      return false;
    }

    return true;
  }

  // NumBytesInUse will calculate how many bytes in the shared buffer are
  // currently in use.
  uint32_t NumBytesInUse(uint32_t read_pos, uint32_t write_pos) {
    uint32_t bytes_in_use = 0;
    if (read_pos <= write_pos) {
      bytes_in_use = write_pos - read_pos;
    } else {
      bytes_in_use = write_pos + (usable_len() - read_pos);
    }

    return bytes_in_use;
  }

  bool TryLockForWriting() {
    // We return true if we set the flag (meaning it was false).
    return !write_flag().test_and_set(std::memory_order_acquire);
  }

  void UnlockForWriting() { write_flag().clear(std::memory_order_release); }

  // This is the space we need to reserve in this shared buffer for our control
  // structure at the start.
  constexpr static size_t kReservedSpace =
      sparo::RoundUpToWordBoundary(sizeof(ControlStructure));

  std::atomic_flag& write_flag() {
    // DCHECK(is_valid());
    return reinterpret_cast<ControlStructure*>(base_ptr_)->write_flag;
  }

  std::atomic_flag& read_flag() {
    // DCHECK(is_valid());
    return reinterpret_cast<ControlStructure*>(base_ptr_)->read_flag;
  }

  std::atomic_uint32_t& read_pos() {
    // DCHECK(is_valid());
    return reinterpret_cast<ControlStructure*>(base_ptr_)->read_pos;
  }

  std::atomic_uint32_t& write_pos() {
    // DCHECK(is_valid());
    return reinterpret_cast<ControlStructure*>(base_ptr_)->write_pos;
  }

  SharedBuffer(uint8_t* ptr, size_t len) : base_ptr_(ptr), len_(len) {}

  uint8_t* base_ptr_ = nullptr;
  size_t len_ = 0;
};