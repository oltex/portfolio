#pragma once
#include <Windows.h>

namespace system_component::lock {
	class slim_read_write final {
	public:
		inline explicit slim_read_write(void) noexcept {
			InitializeSRWLock(&_srwlock);
		};
		inline explicit slim_read_write(slim_read_write const& rhs) noexcept = delete;
		inline explicit slim_read_write(slim_read_write&& rhs) noexcept = delete;
		inline auto operator=(slim_read_write const& rhs) noexcept -> slim_read_write & = delete;
		inline auto operator=(slim_read_write&& rhs) noexcept -> slim_read_write & = delete;
		inline ~slim_read_write(void) noexcept = default;
	public:
		inline void acquire_exclusive(void) noexcept {
			AcquireSRWLockExclusive(&_srwlock);
		}
		inline bool try_acquire_exclusive(void) noexcept {
			return TryAcquireSRWLockExclusive(&_srwlock);
		}
		inline void acquire_shared(void) noexcept {
			AcquireSRWLockShared(&_srwlock);
		}
		inline bool try_acquire_shared(void) noexcept {
			return TryAcquireSRWLockShared(&_srwlock);
		}
		inline void release_exclusive(void) noexcept {
#pragma warning(suppress: 26110)
			ReleaseSRWLockExclusive(&_srwlock);
		}
		inline void release_shared(void) noexcept {
			ReleaseSRWLockShared(&_srwlock);
		}
	public:
		inline auto data(void) noexcept -> SRWLOCK& {
			return _srwlock;
		}
	private:
		SRWLOCK _srwlock;
	};
}