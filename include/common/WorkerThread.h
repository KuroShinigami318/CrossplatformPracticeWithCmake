#pragma once
#include "Utils.h"

/*
How to use this lib:
- You can manual create worker thread by yourself when you define update is true and the mode is UPDATE_CALLBACK,
this is only case you must manually create worker thread.
- when you use MESSAGE_QUEUE_MT mode this is multi thread mode with several worker thread
+ you can get the result from the callback.
But you need to declare the result type except void at template specialization for WorkerThread.
Ex: WorkerThread<int()> example; or WorkerThread<bool()> example; or WorkerThread<double()> example, etc... WorkerThread<ResultType()>
+ then you can use GetResult() to get the result from the handle when PushCallback return.
*/
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4002)
#endif
namespace utils
{
	template < typename Signature = void() >
	class WorkerThread;
}

namespace utils 
{
	template < typename _R = void() >
	class WorkerRunnable : public ObjectLifeIndicator
	{
	static_assert(std::is_function_v<_R>, "Please provide a valid function!");
	public:
		WorkerRunnable() : didWorkerRunnableOverride(true), thread_mode(THREAD_EXECUTION_MODE::NONE) {}
		~WorkerRunnable() = default;

		template < typename Signature >
		friend class WorkerThread;

		virtual void OnRun() { didWorkerRunnableOverride = false; }
		virtual void OnCancel() {}

	private:
		THREAD_EXECUTION_MODE thread_mode;
		std::optional<std::thread> internalThread;
		bool didWorkerRunnableOverride;

		static uint32_t Run(void* value);

#if defined(WINAPI_FAMILY)
		static void set_thread_name(DWORD thread_id, const char* name)
		{
			struct ThreadNameInfo
			{
				DWORD Type;	  // must be 0x1000
				LPCSTR Name;
				DWORD ThreadID;
			};

			const ThreadNameInfo info{ 0x1000, name, thread_id };

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 6320 6322 6312)
#endif
			__try
			{
				RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (const ULONG_PTR*)&info);
			}
			__except (EXCEPTION_CONTINUE_EXECUTION)
			{
			}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
		}
#endif
	};

	template < typename _R, typename... Args >
	class WorkerThread<_R(Args...)> : public std::enable_shared_from_this <WorkerThread<_R(Args...)>>, public ObjectLifeIndicator
	{
	private:
		struct AccessKey;

	public:
		static_assert(std::is_function_v<_R(Args...)>, "Please provide a valid function!");
		using shared_ptr = std::shared_ptr<WorkerThread<_R(Args...)>>;
		using func_type = std::function<_R(Args...)>;
		using ret_type = typename func_type::result_type;
		using message_handle = MessageHandle<ret_type>;
		using message_wrapper = HandlerWrapper<ret_type>;
		using packaged_message = std::pair<std::unique_ptr<message_wrapper>, func_type>;

		std::shared_ptr<WorkerThread<_R(Args...)>> getptr()
		{
			return this->shared_from_this();
		}

		struct null_deleter
		{
			// only use this in case you know what you are doing
			void operator()(utils::WorkerThread<_R(Args...)>* thread)
			{
				assert(thread->workerRunnable->thread_mode == THREAD_EXECUTION_MODE::DETACH, "Please don't use this with other mode from detach");
				if (thread->workerRunnable->thread_mode != THREAD_EXECUTION_MODE::DETACH)
				{
					delete thread;
				}
			}

			template <class T>
			void operator()(T* object)
			{
			}
		};

		static void default_deleter(utils::WorkerThread<_R(Args...)>* thread)
		{
			if (thread != nullptr && thread->IsObjectStillAlive())
			{
				delete thread;
			}
		}

		// only use this for creating detach thread (in case you know what you are doing)
		static std::shared_ptr<WorkerThread<_R(Args...)>> make_shared_with_null_deleter(Args&&... args)
		{
			return std::shared_ptr<WorkerThread<_R(Args...)>>(new utils::WorkerThread<_R(Args...)>(std::forward<Args>(args)...), null_deleter());
		}

		static std::shared_ptr<WorkerThread<_R(Args...)>> make_shared(Args&&... args)
		{
			return std::shared_ptr<WorkerThread<_R(Args...)>>(new utils::WorkerThread<_R(Args...)>(std::forward<Args>(args)...), &default_deleter);
		}

		WorkerThread(const char* thread_name = "Worker Thread");	// Default mode is RUN_ONE_TIME

		template <typename T>
		WorkerThread(bool, T* i_customRunnable, const char* thread_name = "Worker Thread");

		WorkerThread(bool, const char* thread_name = "Worker Thread", utils::MODE i_mode = utils::MODE::UPDATE_CALLBACK, unsigned int max = MAX_SIZE, bool isThreadPool = false, unsigned int poolSize = std::thread::hardware_concurrency(), bool i_customRunnable = false);

		template < typename _Ret, typename _Obj, typename... Args2, typename... Args3 >
		Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> CreateWorkerThread(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj> ctx, Args3&&... args);

		template < typename _Ret, typename _Obj, typename... Args2, typename... Args3 >
		Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> CreateWorkerThread(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj*> ctx, Args3&&... args);

		template < typename _Ret, typename _Obj, typename... Args2, typename... Args3 >
		Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> CreateWorkerThread(_Ret(_Obj::* callback)(Args2...), _Obj* ctx, Args3&&... args);

		template < typename _Ret, typename... Args2, typename... Args3>
		Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> CreateWorkerThread(_Ret(*callback)(Args2...), Args3&&... args);

		template < typename _Ret, typename... Args2>
		Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> CreateWorkerThread(_Ret callback, Args2&&... args);

		template < typename _Ret, typename... Args2>
		Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> CreateWorkerThread(std::function<_Ret(Args2...)> callback, Args2&&... args);

		template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
		message_handle PushCallback(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj> ctx, Args3&&... args);

		template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
		message_handle PushCallback(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj*> ctx, Args3&&... args);

		template < typename _Ret, typename _Obj, typename... Args2 , typename... Args3>
		message_handle PushCallback(_Ret(_Obj::* callback)(Args2...), _Obj *ctx, Args3&&... args);

		template < typename _Ret, typename... Args2, typename... Args3>
		message_handle PushCallback(_Ret(* callback)(Args2...), Args3&&... args);

		template < typename _Call, typename... Args2>
		message_handle PushCallback(_Call callback, Args2&&... args);

		template < typename _Ret, typename... Args2 >
		message_handle PushCallback(std::function<_Ret(Args2...)> callback, Args2&&... args);

		void Dispatch();

		Result<packaged_message, WorkerThreadERR> Pop();

		Result<packaged_message, WorkerThreadERR> Blocking_Pop(bool);

		size_t QueueSize() const;

		bool IsEmptyQueue() const;

		bool IsProcesDone();

		bool HasAllMTProcessDone();

		void Clear();

		void RunOneTime(bool);

		bool IsOneTimeRun();

		bool Joinable();

		~WorkerThread();

		static void Sleep(uint32_t sleepTimeMs);

		static std::thread::id GetCurThreadId();

		void Pause(bool);

		bool IsPaused();

		bool IsTerminated();

		void StopAsync();

		void Suspend();

		void Resume();

		void Join();

		void Detach();

		bool Wait();

		utils::MODE GetCurrentMode();

		WorkerThreadERR ChangeMode(utils::MODE, unsigned int max = MAX_SIZE);

		Signal_mt<void(), AccessKey> sig_onRunFinished;
		Signal_mt<void(std::string), AccessKey> sig_errorThrow;

	private:
		template < typename _R >
		friend class WorkerRunnable;

		enum class PauseReason
		{
			Manual,
			Automatic,
		};

		enum class OneTimeRunType
		{
			Waitable_Trigger,
			Manual_Trigger,
			Automatic_Trigger,
			None,
		};

		void Stop();

		void Update();

		static uint32_t MTPopQueue(void* value);

		bool RunWithExHandle(void* value);

		void OnMessagePriorityChanged();

		void OnClearCacheDispatchedMessages(const size_t i_key);

		size_t GenerateMessageId()
		{
			if (m_reserveSlots.empty())
			{
				return m_msgIds++;
			}
			else
			{
				size_t slot = m_reserveSlots.front();
				m_reserveSlots.pop();
				return slot;
			}
		}

		template <typename _Mutex>
		constexpr void PrologueSwitchFlag(_Mutex& o_mutex, bool& o_flagToSwitch, bool i_desiredState)
		{
			o_mutex.lock();
			o_flagToSwitch = i_desiredState;
			o_mutex.unlock();
		}

		template <typename _Mutex>
		constexpr Epilogue<void()> EpilogueSwitchFlag(_Mutex& o_mutex, bool& o_flagToSwitch, bool i_desiredState)
		{
			std::function<void()> functor = [&o_mutex, &o_flagToSwitch, i_desiredState]()
			{
				o_mutex.lock();
				o_flagToSwitch = i_desiredState;
				o_mutex.unlock();
			};
			return Epilogue(std::move(functor));
		}

		template <typename _Mutex>
		constexpr Epilogue<void()> FlagSwitcher(_Mutex& o_mutex, bool& o_flagToSwitch, bool i_initialState)
		{
			PrologueSwitchFlag(o_mutex, o_flagToSwitch, i_initialState);
			return EpilogueSwitchFlag(o_mutex, o_flagToSwitch, !i_initialState);
		}

		constexpr bool IsAny(MODE i_mode, std::vector<MODE> i_list)
		{
			return Contains(i_mode, i_list);
		}

		constexpr bool IsLesser(size_t i_mode, std::vector<size_t> i_list)
		{
			return IsLesserThan(i_mode, i_list);
		}

		constexpr bool IsGreater(size_t i_mode, std::vector<size_t> i_list)
		{
			return IsGreaterThan(i_mode, i_list);
		}

		inline bool DoesModeSupportWaitable()
		{
			return !IsAny(this->m_mode, {MODE::RUN_ONE_TIME, MODE::UPDATE_CALLBACK});
		}

		inline void InterWait()
		{
			std::unique_lock<std::mutex> lk_guard(sync_mutex);
			cv.wait(lk_guard, [&] {return isProcesDone; });
		}

		inline void StopWaiting()
		{
			cv.notify_all();
		}

		inline uint32_t CallWithResult(packaged_message callback, std::false_type)
		{
			static_assert(!std::is_same_v<ret_type, void>, "WRONG CALL WITH VOID");
			if (!callback.first->IsCancel())
			{
				MessageStatus expected = MessageStatus::BeingDispatched;
				callback.first->SetStatus(expected, MessageStatus::Dispatching);
				assert(expected == MessageStatus::BeingDispatched);
				if (expected != MessageStatus::BeingDispatched)
				{
					Access<AccessKey>(sig_errorThrow).Emit(Format("Dispatching message failed due to: {}", expected));
					return -1;
				}
				ret_type result = callback.second();
				callback.first->GetHandler().set_value(result);
				callback.first->SetStatus(expected, MessageStatus::Dispatched);
				if (alive)
				{
					std::lock_guard<std::recursive_mutex> guard(mutex);
					m_cacheDispatchedMessages.emplace(callback.first->GetMessageId(), std::move(callback));
				}
				else
				{
					callback.first->OnMessageCanceled();
				}
			}
			return 0;
		}

		inline uint32_t CallWithResult(packaged_message callback, std::true_type)
		{
			static_assert(std::is_same_v<ret_type, void>, "WRONG CALL WITH NON VOID");
			if (!callback.first->IsCancel())
			{
				MessageStatus expected = MessageStatus::BeingDispatched;
				callback.first->SetStatus(expected, MessageStatus::Dispatching);
				assert(expected == MessageStatus::BeingDispatched);
				if (expected != MessageStatus::BeingDispatched)
				{
					Access<AccessKey>(sig_errorThrow).Emit(Format("Dispatching message failed due to: {}", expected));
					return -1;
				}
				callback.second();
				callback.first->GetHandler().set_value();
				callback.first->SetStatus(expected, MessageStatus::Dispatched);
				if (alive)
				{
					std::lock_guard<std::recursive_mutex> guard(mutex);
					m_cacheDispatchedMessages.emplace(callback.first->GetMessageId(), std::move(callback));
				}
				else
				{
					callback.first->OnMessageCanceled();
				}
			}
			return 0;
		}

		inline void Call(packaged_message callback)
		{
			this->CallWithResult(std::move(callback), std::is_same<ret_type, void>{});
		}

		inline uint32_t CallAsync(packaged_message callback)
		{
			Epilogue cleanup(std::move([&]()
			{
				std::lock_guard<std::mutex> guard(this->sync_mutex);
				assert(this->threads_count > 0);
				this->threads_count--;
			}));
			return this->CallWithResult(std::move(callback), std::is_same<ret_type, void>{});
		}

		inline void FakeCreateWorkerThread(std::false_type)
		{
			CreateWorkerThread([]() -> ret_type {
				ret_type fake_return{};
				return fake_return;
			});
		}

		inline void FakeCreateWorkerThread(std::true_type)
		{
			CreateWorkerThread([]() {});
		}

		inline void FakeCreateWorkerThread()
		{
			FakeCreateWorkerThread(std::is_same<ret_type, void>{});
		}

		inline void JoinThreadsIntoPool()
		{
			StopWaiting();
			while (threads_count > 0);
		}

		inline void CleanSubThreads(bool waitUntilCleanAll)
		{
			do
			{
				std::erase_if(m_subThreads, [](std::future<uint32_t>& i_subThread)
				{
					return i_subThread._Is_ready();
				});
			}
			while (!m_subThreads.empty() && waitUntilCleanAll);
		}

		std::recursive_mutex mutex;
		std::mutex sync_mutex;
		std::string m_thread_name;

		func_type m_callback;
		unsigned int threads_count;
		unsigned int m_sizeToDispatch;
		unsigned int max_queue = MAX_SIZE;
		unsigned int m_poolSize = std::thread::hardware_concurrency();
		OneTimeRunType oneTimeRun;
		std::atomic_bool update;
		bool alive;
		bool isTerminated;
		bool isProcesDone;
		bool didThreadStart;
		bool customRunnable;
		std::shared_ptr<WorkerRunnable<_R()>> workerRunnable;
		size_t m_msgIds = 0;
		std::queue<size_t> m_reserveSlots;
		std::vector<packaged_message> m_queue;
		std::vector<std::future<uint32_t>> m_subThreads;
		std::unordered_map<size_t, packaged_message> m_cacheDispatchedMessages;
		std::atomic<MODE> m_mode;
		std::optional<PauseReason> m_pauseReason;
		std::condition_variable_any cv;
	};

	template <typename T>
	struct ThreadGuard
	{
		void operator()(utils::WorkerThread<T>* thread) const
		{
			if (thread->Joinable())
				thread->Join(); // this is safe, but it blocks when scoped_thread goes out of scope
			delete thread;
		}
	};

	template <typename T = void()>
	using scoped_thread = std::unique_ptr<utils::WorkerThread<T>, ThreadGuard<T>>;

	template <typename T = void(), typename... Args>
	scoped_thread<T> inline CreateScopedThread(Args... args)
	{
		return std::move(scoped_thread<T>(new utils::WorkerThread<T>(std::forward<Args>(args)...), ThreadGuard<T>()));
	}
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#include "WorkerThread.inl"