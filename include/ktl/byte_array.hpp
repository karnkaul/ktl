// KTL header-only library
// Requirements: C++20

#pragma once
#include <cassert>
#include <cstddef>
#include <cstring>
#include <memory>
#include <span>

namespace ktl {
///
/// \brief Lightweight, resizeable wrapper over std::unique_ptr<std::byte[]>
///
class byte_array {
  public:
	byte_array() = default;

	explicit byte_array(std::size_t length) : m_data(std::make_unique_for_overwrite<std::byte[]>(length)), m_capacity(length), m_size(length) {}

	byte_array& resize_for_overwrite(std::size_t length) {
		if (m_capacity < length) { *this = byte_array(length); }
		return *this;
	}

	byte_array& overwrite(void const* data, std::size_t length) {
		resize_for_overwrite(length);
		assert(m_data.get() && data && length > 0);
		std::memcpy(m_data.get(), data, m_size = length);
		return *this;
	}

	std::byte const* data() const noexcept { return m_data.get(); }
	std::byte* data() noexcept { return m_data.get(); }
	std::size_t size() const noexcept { return m_size; }

	std::span<std::byte const> span() const { return {data(), size()}; }
	operator std::span<std::byte const>() const { return span(); }

	void swap(byte_array& rhs) { std::swap(m_data, rhs.m_data); }

  private:
	std::unique_ptr<std::byte[]> m_data{};
	std::size_t m_capacity{};
	std::size_t m_size{};
};
} // namespace ktl
