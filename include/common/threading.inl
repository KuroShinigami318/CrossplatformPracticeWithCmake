#pragma once

namespace utils
{
namespace details
{
namespace threading
{
	inline bool null_mutex::try_lock() noexcept
	{
		return false;
	}

	inline void null_mutex::lock()
	{
	}

	inline void null_mutex::unlock() noexcept
	{
	}

	inline void null_thread_checker::check_thread_id() const
	{
	}

	inline thread_checker::thread_checker() : thread_id(utils::GetCurrentThreadID())
	{
	}

	template<class> void thread_checker::check_thread_id() const
	{
		const size_t currentThreadId = utils::GetCurrentThreadID();
#if defined(_DEBUG)
		assert(thread_id == currentThreadId);
#endif
		if (thread_id != currentThreadId)
		{
			ERROR_LOG("thread checker failure", "mismatch thread id expected: {} but actual: {}", thread_id, currentThreadId);
		}
	}

	inline void thread_checker::set_thread_id(const size_t&)
	{
	}

	inline void thread_checker_da::set_thread_id(const size_t& i_threadId)
	{
		thread_id = i_threadId;
	}
} // threading
} // details
} // utils