## KTL

A lightweight set of utility headers written in C++17.

### Usage

**Requirements**

- CMake
- C++17 compiler (and stdlib)

**Steps**

1. Clone repo to appropriate subdirectory, say `ktl`
1. Add library to project via: `add_subdirectory(ktl)` and `target_link_libraries(foo ktl::ktl)`
1. Use desired headers via `#include <ktl/header.hpp>`

### Headers

#### `enum_flags/enum_flags.hpp`

Wrapper around an integral type used as bit flags.

#### `enum_flags/uint_flags.hpp`

Trivial wrapper for unsigned int as bit flags (union friendly).

#### `async_queue.hpp`

FIFO queue with thread safe "sleepy" API.

Features:

- Policy customization
- Multiple queues
- Thread-safe push-and-notify (to any desired queue)
- Thread-safe wait-and-pop (from first of any desired queues)
- Clear all queues and return residue
- Deactivate all queues (as secondary wait condition)

#### `fixed_any.hpp`

Fixed-size type erased storage.

#### `fixed_vector.hpp`

Fixed-size vector-like container using bytearray as storage.

#### `future.hpp`

Async operation / shared state wrappers, with `.then()` support.

#### `kthread.hpp`

`std::thread` wrapper that joins on destruction / move, and supports `stop_t` tokens.

#### `monotonic_map.hpp`

Wrapper over an (un)ordered map that associates each T with a unique RAII handle which can be used to unregister the instance.

#### `move_only_function.hpp`

Callable wrapper that cannot be copied, only moved.

#### `n_tree.hpp`

Models a "forward" N-tree (no parent link) via `std::forward_list`.

#### `not_null.hpp`

Wrapper for raw / smart pointers that is restricted from being null.

#### `result.hpp`

Models a result (`T`) or an error (`E`) value;`T` cannot be void.

Specializations:

- `T, T` : homogeneous result and error types
- `T, void` : result type only (like `std::optional<T>`)
- `bool, void` : boolean result only (like `bool`)

#### `tmutex.hpp`

Basic and strict wrappers for a `T` and its (mutable) mutex, stdlib RAII lock types.

#### `shared_tmutex.hpp`

Aliases of `tmutex` with `std::shared_mutex`.

#### `str_format.hpp`

Format a `std::string` / `std::wstring` using provided interpolation token (`{}` by default).

### Contributing

Pull/merge requests are welcome.

**[Original Repository](https://github.com/karnkaul/ktl)**
