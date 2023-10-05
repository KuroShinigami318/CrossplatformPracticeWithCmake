#pragma once

namespace utils
{
namespace details
{
namespace threading
{
	struct null_mutex
	{
		bool try_lock() noexcept;

		void lock();

		void unlock() noexcept;
	};

	struct null_thread_checker
	{
	public:
		void check_thread_id() const;
	};

	struct thread_checker
	{
	public:
		thread_checker();

		template <class _dummy = void> void check_thread_id() const;

	protected:
		virtual void set_thread_id(const size_t&);

		size_t thread_id;
	};

	struct thread_checker_da : thread_checker
	{
	public:
		void set_thread_id(const size_t& i_threadId) override;
	};

	struct threadmodel_mt
	{
		using recursive_mutex_t = std::recursive_mutex;
		using mutex_t = std::mutex;
		using threadchecker_t = null_thread_checker;
		static const bool is_da = std::is_same<threadchecker_t, thread_checker_da>::value;
	};

	struct threadmodel_st
	{
		using recursive_mutex_t = null_mutex;
		using mutex_t = null_mutex;
		using threadchecker_t = thread_checker;
		static const bool is_da = std::is_same<threadchecker_t, thread_checker_da>::value;
	};

	struct threadmodel_st_da
	{
		using recursive_mutex_t = null_mutex;
		using mutex_t = null_mutex;
		using threadchecker_t = thread_checker_da;
		static const bool is_da = std::is_same<threadchecker_t, thread_checker_da>::value;
	};
} // namespace threading
} // namespace details

using null_mutex = details::threading::null_mutex;
} // namespace utils

#include "threading.inl"