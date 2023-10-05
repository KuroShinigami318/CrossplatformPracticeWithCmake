#pragma once

namespace utils
{
class Log;
}

#define ThrowIfNullptr(rawPtr) \
	if(rawPtr == nullptr) \
	{ \
		throw std::invalid_argument("received null ptr"); \
	}

namespace utils
{
	enum class ObjectLifeState
	{
		Alive,
		Dead
	};

	struct ObjectLifeIndicator
	{
	public:
		ObjectLifeIndicator() : objectLifeState(ObjectLifeState::Alive) {}
		~ObjectLifeIndicator()
		{
			objectLifeState = ObjectLifeState::Dead;
		}

		bool IsObjectStillAlive()
		{
			return objectLifeState == ObjectLifeState::Alive;
		}

	private:
		ObjectLifeState objectLifeState;
	};

	template <class T, class D = std::default_delete<T>>
	class HolderImpl
	{
	public:
		HolderImpl() = default;
		virtual ~HolderImpl() = default;
		HolderImpl(std::unique_ptr<T, D> i_holderImpl)
		{
			if (i_holderImpl)
			{
				Move(std::move(i_holderImpl));
			}
		}
		HolderImpl(HolderImpl&& other) noexcept
		{
			if (other.m_holderImpl)
			{
				Move(std::move(other.m_holderImpl));
			}
		}
		HolderImpl(HolderImpl const&) = delete;
		HolderImpl& operator=(HolderImpl&& other) noexcept
		{
			// Guard self assignment
			if (this == &other)
			{
				return *this;
			}
			if (other.m_holderImpl)
			{
				Move(std::move(other.m_holderImpl));
			}
			return *this;
		}
		HolderImpl operator=(HolderImpl const&) = delete;

	protected:
		void Move(std::unique_ptr<T, D> i_holderImpl)
		{
			m_holderImpl = std::unique_ptr<T, D>(i_holderImpl.release(), i_holderImpl.get_deleter());
		}
		std::unique_ptr<T, D> m_holderImpl;
	};

	template <class T, class D = std::default_delete<T>>
	class unique_ref : public HolderImpl<T, D>
	{
	public:
		unique_ref() = delete;
		unique_ref(std::nullptr_t) = delete;
		template <typename... Args>
		unique_ref(Args&&... args)
		{
			this->m_holderImpl = std::make_unique<T>(std::forward<Args>(args)...);
		}
		unique_ref(unique_ref<T, D>&& other) noexcept : HolderImpl<T, D>(std::move(other))
		{
		}
		unique_ref(T* rawPtr)
		{
			ThrowIfNullptr(rawPtr);
			this->m_holderImpl = std::unique_ptr<T, D>(rawPtr, D());
		}
		unique_ref<T>& operator=(unique_ref<T>&& other) noexcept
		{
			return static_cast<unique_ref<T>&>(operator=(std::move(other)));
		}
		unique_ref<T>& operator=(std::nullptr_t) = delete;
		~unique_ref()
		{
			reset();
		}

		void reset()
		{
			this->m_holderImpl.reset();
		}

		T* get() const
		{
			return this->m_holderImpl.get();
		}

		T* operator->() const
		{
			return get();
		}

		T& operator*() const
		{
			return *get();
		}

		// unique_ref comparision
		template <class T1, class T2> friend bool operator==(unique_ref<T1> const& left, unique_ref<T2> const& right);
		template <class T1, class T2> friend bool operator!=(unique_ref<T1> const& left, unique_ref<T2> const& right);

		// raw pointer comparision
		template <class T1, class T2> friend bool operator==(unique_ref<T1> const& left, const T2* right);
		template <class T1, class T2> friend bool operator!=(unique_ref<T1> const& left, const T2* right);
		template <class T1, class T2> friend bool operator==(const T1* left, unique_ref<T2> const& right);
		template <class T1, class T2> friend bool operator!=(const T1* left, unique_ref<T2> const& right);

		// unique_ptr comparision
		template <class T1, class T2> friend bool operator==(unique_ref<T1> const& left, std::unique_ptr<T2> const& right);
		template <class T1, class T2> friend bool operator!=(unique_ref<T1> const& left, std::unique_ptr<T2> const& right);
		template <class T1, class T2> friend bool operator==(std::unique_ptr<T1> const& left, unique_ref<T2> const& right);
		template <class T1, class T2> friend bool operator!=(std::unique_ptr<T1> const& left, unique_ref<T2> const& right);
	};

	// unique_ptr comparision
	template <class T1, class T2> inline bool operator==(unique_ref<T1> const& left, std::unique_ptr<T2> const& right)
	{
		return left.get() == right.get();
	}
	template <class T1, class T2> inline bool operator!=(unique_ref<T1> const& left, std::unique_ptr<T2> const& right)
	{
		return left.get() != right.get();
	}
	template <class T1, class T2> inline bool operator==(std::unique_ptr<T1> const& left, unique_ref<T2> const& right)
	{
		return left.get() == right.get();
	}
	template <class T1, class T2> inline bool operator!=(std::unique_ptr<T1> const& left, unique_ref<T2> const& right)
	{
		return left.get() != right.get();
	}

	// raw pointer comparision
	template <class T1, class T2> inline bool operator==(unique_ref<T1> const& left, const T2* right)
	{
		return left.get() == right;
	}
	template <class T1, class T2> inline bool operator!=(unique_ref<T1> const& left, const T2* right)
	{
		return left.get() != right;
	}
	template <class T1, class T2> inline bool operator==(const T1* left, unique_ref<T2> const& right)
	{
		return left == right.get();
	}
	template <class T1, class T2> inline bool operator!=(const T1* left, unique_ref<T2> const& right)
	{
		return left != right.get();
	}

	// unique_ref comparision
	template <class T1, class T2> inline bool operator==(unique_ref<T1> const& left, unique_ref<T2> const& right)
	{
		return left.get() == right.get();
	}
	template <class T1, class T2> inline bool operator!=(unique_ref<T1> const& left, unique_ref<T2> const& right)
	{
		return left.get() != right.get();
	}

	inline size_t GetCurrentThreadID()
	{
#if defined(WINAPI_FAMILY)
		DWORD threadID = GetCurrentThreadId();
#else
		size_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
#endif
		return threadID;
	}

	template<size_t...> struct index_sequence {};
	template<size_t N, size_t... S> struct make_index_sequence_internal : make_index_sequence_internal<N-1, N-1, S...>
	{
	};
	template<size_t... S> struct make_index_sequence_internal<0, S...>
	{
		using type = index_sequence<S...>;
	};
	template<size_t N> using make_index_sequence = make_index_sequence_internal<N>::type;

	template <typename T = void()>
	struct general_message {};

	template <typename R, typename ...Args>
	struct general_message<R(Args...)>
	{
	public:
		using ret_type = R;
		virtual ~general_message() = default;

		virtual ret_type operator()(Args&&...) = 0;

	protected:
		general_message() {};
	};

	struct void_message_no_argument : virtual public general_message<void()>
	{
	public:
		void_message_no_argument(std::function<void()> i_msg) : m_msg(i_msg)
		{
		}
		~void_message_no_argument() = default;
		void operator()() override
		{
			m_msg();
		}

	private:
		std::function<void()> m_msg;
	};

	template <typename T>
	struct normal_message_full_type {};

	template <typename R, typename ...Args>
	struct normal_message_full_type<R(Args...)> : virtual public general_message<R(Args...)>
	{
	public:
		normal_message_full_type(std::function<R(Args...)> i_msg) : m_msg(i_msg)
		{
		}
		~normal_message_full_type() = default;
		general_message<R(Args...)>::ret_type operator()(Args&&... args) override
		{
			return m_msg(std::forward<Args>(args)...);
		}

	private:
		std::function<R(Args...)> m_msg;
	};

	template<typename T>
	struct function_traits {};

	template<typename R, typename ...Args>
	struct function_traits<std::function<R(Args...)>>
	{
		static const size_t nargs = sizeof...(Args);

		typedef R result_type;

		using tuple_params = std::tuple<Args...>;
		using decay_tuple_params = std::tuple<typename std::decay<Args>::type...>;

		template <size_t i>
		struct arg
		{
			typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
		};
	};

	template<typename Function, typename Tuple, size_t ... I>
	auto call(Function f, Tuple t, index_sequence<I ...>)
	{
		return f(std::get<I>(t) ...);
	}

	template<typename Function, typename ...Args>
	auto call(Function f, std::tuple<Args...> t)
	{
		static constexpr auto size = std::tuple_size<std::tuple<Args...>>::value;
		return call(f, t, make_index_sequence<size>{});
	}

	template<typename Function, typename ...Args>
	auto call(Function f, Args&&... args)
	{
		return f(std::forward<Args>(args)...);
	}

	template <size_t ... I, typename Tuple>
	auto ref_tuple_impl(index_sequence<I ...>, Tuple& tup)
	{
		return std::make_tuple(std::ref(std::get<I>(tup))...);
	}

	template <typename Tuple>
	auto ref_tuple(Tuple& tup)
	{
		return ref_tuple_impl(make_index_sequence<std::tuple_size<Tuple>::value>{}, tup);
	}

	template < typename T >
	inline bool is_aligned(T* p) {
		return !(reinterpret_cast<uintptr_t>(p) % std::alignment_of<T>::value);
	}

	inline const std::string WHITESPACE = " \n\r\t\f\v";

	inline std::string ltrim(const std::string& s)
	{
		size_t start = s.find_first_not_of(WHITESPACE);
		return (start == std::string::npos) ? "" : s.substr(start);
	}

	inline std::string rtrim(const std::string& s)
	{
		size_t end = s.find_last_not_of(WHITESPACE);
		return (end == std::string::npos) ? "" : s.substr(0, end + 1);
	}

	inline std::string trim(const std::string& s)
	{
		return rtrim(ltrim(s));
	}

	constexpr unsigned int MAX_SIZE = SHRT_MAX;

	template <typename T>
	constexpr std::string ConvertToString(T value, std::false_type)
	{
		return std::format("{}", value);
	}

	template <typename T>
	constexpr std::string ConvertToString(T value, std::true_type)
	{
		return std::format("{}", magic_enum::enum_name(value));
	}

	template <typename T>
	constexpr std::string ConvertToString(T value)
	{
		return ConvertToString(value, std::bool_constant<std::is_enum_v<T>>());
	}

	template <size_t ... N, typename Tuple>
	constexpr std::string Format(const char* format, Tuple tuple, index_sequence<N...>)
	{
		return std::vformat(format, std::make_format_args(ConvertToString(std::get<N>(tuple))...));
	}

	template <typename ...Args>
	constexpr std::string Format(const char* format, Args&&... args)
	{
		return Format(format, std::make_tuple(std::forward<Args>(args)...), make_index_sequence<sizeof...(Args)>());
	}

	// default mode is UPDATE_CALLBACK
	enum class MODE
	{
		UPDATE_CALLBACK,
		MESSAGE_QUEUE,
		MESSAGE_QUEUE_MT,
		MESSAGE_LOOP,
		RUN_ONE_TIME,	// default for the thread is join when destroyed
	};

	enum class THREAD_EXECUTION_MODE
	{
		JOIN,
		DETACH,
		THREAD_POOL,
		NONE
	};

	enum class WorkerThreadERR
	{
		SUCCESSS,
		UPDATE_CALLBACK_ERR,
		PUSH_MQ_ERR_EXCEED,
		CHANGE_MODE_UPDATE_CB_ERR,
		CALLBACK_NULL_ERR,
		WORKER_RUNNABLE_CREATE_FAILED,
		CREATE_THREAD_FAILED,
		SAME_PREVIOUS_MODE,
		IS_ALREADY_CREATED,
		THIS_MODE_CAN_NOT_BE_USED,
		ENQUEUE_MSG_WHILE_STOPPING_THREAD,
		MESSAGE_CANCELED
	};

	enum class MessageQueueERR
	{
		UPDATE_CALLBACK_ERR,
		PUSH_MQ_ERR_EXCEED,
		CALLBACK_NULL_ERR,
		BLOCKING_ERR,
	};

	constexpr unsigned short k_updateIntervalMilliseconds = 1;

	using timepoint = std::chrono::system_clock::time_point;
	using milisecs = std::chrono::milliseconds;
	using secs = std::chrono::seconds;
	using nanosecs = std::chrono::nanoseconds;
	using minutes = std::chrono::minutes;
	using hours = std::chrono::hours;
	using days = std::chrono::days;
	using months = std::chrono::months;
	using years = std::chrono::years;
	template <class Rep, class Period>
	using duration = std::chrono::duration<Rep, Period>;
	template <class Clock, class Duration>
	using clock_timepoint = std::chrono::time_point<Clock, Duration>;

	template <bool>
	struct wrapper {};

	template <typename T>
	constexpr bool Contains(T value, std::vector<T> list)
	{
		std::vector<T> tempList = list;
		return std::find(std::begin(tempList), std::end(tempList), value) != std::end(tempList);
	}

	template <typename T>
	constexpr bool IsLesserThan(T value, std::vector<T> list)
	{
		std::vector<T> tempList = list;
		return !(std::find_if(std::begin(tempList), std::end(tempList), [=, &value](const T& comparedValue)
		{
			return comparedValue >= value;
		}) != std::end(tempList));
	}

	template <typename T>
	constexpr bool IsGreaterThan(T value, std::vector<T> list)
	{
		std::vector<T> tempList = list;
		return !(std::find_if(std::begin(tempList), std::end(tempList), [=, &value](const T& comparedValue)
		{
			return comparedValue <= value;
		}) != std::end(tempList));
	}

	template <typename T = void()>
	struct Epilogue
	{
		static_assert(std::is_function_v<T>, "Please provide a valid function!");
		using func_type = std::function<T>;
		using ret_type = std::invoke_result<func_type>::type;

		Epilogue(const std::function<T>& i_cleanup) : m_cleanup(i_cleanup)
		{
		}
		Epilogue(std::function<T>&& i_cleanup) : m_cleanup(std::move(i_cleanup))
		{
		}

		template <typename F>
		Epilogue(const F& i_cleanup) : Epilogue(func_type(i_cleanup))
		{
		}
		template <typename F>
		Epilogue(F&& i_cleanup) : Epilogue(std::move(func_type(i_cleanup)))
		{
		}

		~Epilogue()
		{
			m_cleanup();
		}

	private:
		func_type m_cleanup;
	};

	enum class MessagePriority
	{
		Immediately,
		ASAP, // As soon as possible
		NormalAsync
	};

	enum class MessageStatus
	{
		BeingDispatched,
		Dispatching,
		Canceled,
		Dispatched
	};

	enum class MessageHandleERR
	{
		SUCCESS,
		InvalidHandler,
		Void,
		Cancelled,
	};

	template <typename T>
	struct HandlerWrapper
	{
	public:
		HandlerWrapper(const size_t i_id) : m_handler(std::move(std::promise<T>()))
		{
			m_msgId = i_id;
			m_future = m_handler->get_future();
			m_status = MessageStatus::BeingDispatched;
			m_priority = MessagePriority::NormalAsync;
		}
		~HandlerWrapper()
		{
			OnCancel();
		}

		std::promise<T>& GetHandler()
		{
			return *m_handler;
		}

		std::future<T>& GetFuture()
		{
			return m_future;
		}

		MessageStatus OnCancel()
		{
			MessageStatus expected = MessageStatus::BeingDispatched;
			if (m_status.compare_exchange_strong(expected, MessageStatus::Canceled))
			{
				m_onPriorityChangedCallback = nullptr;
			}
			return expected;
		}

		void Cancel(std::true_type)
		{
			m_handler->set_value();
		}

		void Cancel(std::false_type)
		{
			m_handler->set_value(T());
		}

		MessageHandleERR Cancel()
		{
			MessageStatus expected = OnCancel();
			if (expected == MessageStatus::BeingDispatched)
			{
				Cancel(std::is_same<T, void>());
			}
			else if (expected == MessageStatus::Canceled)
			{
				return MessageHandleERR::Cancelled;
			}
			return MessageHandleERR::SUCCESS;
		}

		void SetStatus(MessageStatus& o_expected, const MessageStatus i_desired)
		{
			m_status.compare_exchange_strong(o_expected, i_desired);
		}

		bool IsCancel() const
		{
			return m_status == MessageStatus::Canceled;
		}

		// We don't guarantee set for multithread
		void SetPriority(const MessagePriority i_priority)
		{
			if (m_status != MessageStatus::BeingDispatched)
			{
				return;
			}
			if (m_handlerThreadId.has_value())
			{
				assert(m_handlerThreadId == std::this_thread::get_id(), "Only use this for single thread!");
				std::terminate();
			}
			else
			{
				m_handlerThreadId = std::this_thread::get_id();
			}
			if (m_onPriorityChangedCallback && i_priority != MessagePriority::NormalAsync)
			{
				m_priority = i_priority;
				m_onPriorityChangedCallback();
			}
		}

		MessagePriority GetPriority() const
		{
			return m_priority;
		}

		size_t GetMessageId()
		{
			return m_msgId;
		}

		void RegisterPriorityChangedCallback(std::function<void()> callback)
		{
			m_onPriorityChangedCallback = callback;
		}

		void RegisterMessageCancelCallback(std::function<void()> callback)
		{
			m_onCanceledMessageCallback = callback;
		}

		void OnMessageCanceled()
		{
			if (m_onCanceledMessageCallback)
			{
				m_onCanceledMessageCallback();
				m_onCanceledMessageCallback = nullptr;
			}
		}

	private:
		size_t m_msgId;
		std::optional<std::thread::id> m_handlerThreadId;
		unique_ref<std::promise<T>> m_handler;
		std::future<T> m_future;
		std::atomic<MessageStatus> m_status;
		MessagePriority m_priority;
		std::function<void()> m_onPriorityChangedCallback;
		std::function<void()> m_onCanceledMessageCallback;
	};

	template <typename T = void>
	struct MessageHandle
	{
	public:
		using handler_type = HandlerWrapper<T>*;
		using result_type = Result<T, MessageHandleERR>;
		using error_type = std::optional<WorkerThreadERR>;
		using status = std::future_status;
		using cleanupCallback = std::function<void(size_t)>;

		MessageHandle() : m_handler(nullptr)
		{
		}

		MessageHandle(WorkerThreadERR i_error) : m_error(i_error)
		{
		}

		MessageHandle(handler_type i_handler, error_type i_error) : MessageHandle(i_handler)
		{
			if (i_error.has_value() && i_error.value() != WorkerThreadERR::SUCCESSS)
			{
				m_error = i_error;
			}
		}

		MessageHandle(MessageHandle&& i_other) noexcept : MessageHandle(std::move(i_other.m_handler), std::move(i_other.m_error))
		{
			m_cleanup = std::move(i_other.m_cleanup);
			i_other.m_handler = nullptr;
			i_other.m_error.reset();
		}

		MessageHandle(handler_type i_handler) : m_handler(i_handler)
		{
			if (m_handler)
			{
				m_handler->RegisterMessageCancelCallback(std::bind(&MessageHandle::OnDestroyHandler, this));
			}
		}

		MessageHandle& operator=(MessageHandle&& other) noexcept
		{
			if (this == &other)
			{
				return *this;
			}
			Move(std::move(other));
			if (m_handler)
			{
				m_handler->RegisterMessageCancelCallback(std::bind(&MessageHandle::OnDestroyHandler, this));
			}
			return *this;
		}
		MessageHandle& operator=(MessageHandle const&) = delete;
		MessageHandle(MessageHandle const&) = delete;

		~MessageHandle()
		{
			if (m_handler && m_cleanup)
			{
				m_cleanup(m_handler->GetMessageId());
			}
		}

		MessageHandleERR Wait()
		{
			MessageHandleERR checkResult = CheckFatalError();
			if (checkResult != MessageHandleERR::SUCCESS)
			{
				return checkResult;
			}
			m_handler->GetFuture().wait();
			return MessageHandleERR::SUCCESS;
		}

		template <class Rep, class Period>
		Result<status, MessageHandleERR> WaitFor(const utils::duration<Rep, Period>& rel_time)
		{
			MessageHandleERR checkResult = CheckFatalError();
			if (checkResult != MessageHandleERR::SUCCESS)
			{
				return Err(checkResult);
			}
			return Ok(m_handler->GetFuture().wait_for(rel_time));
		}

		template <class Clock, class Duration>
		Result<status, MessageHandleERR> WaitUntil(const utils::clock_timepoint<Clock, Duration>& abs_time)
		{
			MessageHandleERR checkResult = CheckFatalError();
			if (checkResult != MessageHandleERR::SUCCESS)
			{
				return Err(checkResult);
			}
			return Ok(m_handler->GetFuture().wait_until(abs_time));
		}

		result_type GetResult(std::false_type)
		{
			MessageHandleERR checkResult = CheckFatalError();
			if (checkResult != MessageHandleERR::SUCCESS)
			{
				return Err(checkResult);
			}
			return Ok(m_handler->GetFuture().get());
		}

		result_type GetResult(std::true_type)
		{
			return Err(MessageHandleERR::Void);
		}

		result_type GetResult()
		{
			return GetResult(std::is_same<T, void>());
		}

		MessageHandleERR Cancel(std::true_type)
		{
			MessageHandleERR checkResult = CheckFatalError();
			if (checkResult != MessageHandleERR::SUCCESS)
			{
				return checkResult;
			}
			m_handler->GetHandler().set_value();
			m_handler->OnCancel();
			return MessageHandleERR::SUCCESS;
		}

		MessageHandleERR Cancel(std::false_type)
		{
			MessageHandleERR checkResult = CheckFatalError();
			if (checkResult != MessageHandleERR::SUCCESS)
			{
				return checkResult;
			}
			m_handler->GetHandler().set_value(T());
			m_handler->OnCancel();
			return MessageHandleERR::SUCCESS;
		}

		MessageHandleERR Cancel()
		{
			return Cancel(std::is_same<T, void>());
		}

		void SetPriority(const MessagePriority i_priority)
		{
			m_handler->SetPriority(i_priority);
		}

		MessageHandleERR CheckFatalError()
		{
			if (!IsValidHandler())
			{
				if (m_error.has_value() && m_error == WorkerThreadERR::MESSAGE_CANCELED)
				{
					return MessageHandleERR::Cancelled;
				}
				return MessageHandleERR::InvalidHandler;
			}
			if (m_handler->IsCancel())
			{
				return MessageHandleERR::Cancelled;
			}
			return MessageHandleERR::SUCCESS;
		}

		bool IsError()
		{
			return m_error.has_value() && m_error.value() != WorkerThreadERR::SUCCESSS;
		}

		bool IsSuccess()
		{
			return !IsError();
		}

		WorkerThreadERR GetError()
		{
			return IsSuccess() ? WorkerThreadERR::SUCCESSS : m_error.value();
		}

		void RegisterCleanupCallback(cleanupCallback i_callback)
		{
			m_cleanup = i_callback;
		}

	private:
		bool IsValidHandler()
		{
			return IsSuccess() && m_handler && m_handler->GetFuture().valid();
		}

		void OnDestroyHandler()
		{
			m_handler = nullptr;
			m_error = WorkerThreadERR::MESSAGE_CANCELED;
		}

		void Move(MessageHandle&& other)
		{
			m_handler = std::exchange(other.m_handler, nullptr);
			m_error = std::exchange(other.m_error, error_type());
			m_cleanup = std::exchange(other.m_cleanup, nullptr);
		}

		handler_type m_handler = nullptr;
		error_type m_error;
		cleanupCallback m_cleanup = nullptr;
	};
}

#define DEBUG_LOG(tag, format, ...) \
	utils::Log::LogAdd(utils::Log::LogChannel::Debug, tag, utils::Format(format, __VA_ARGS__), utils::GetCurrentThreadID(), std::source_location::current());

#define ERROR_LOG(tag, format, ...) \
	utils::Log::LogAdd(utils::Log::LogChannel::Error, tag, utils::Format(format, __VA_ARGS__), utils::GetCurrentThreadID(), std::source_location::current());

#define WARNING_LOG(tag, format, ...) \
	utils::Log::LogAdd(utils::Log::LogChannel::Warning, tag, utils::Format(format, __VA_ARGS__), utils::GetCurrentThreadID(), std::source_location::current());

#define INFO_LOG(tag, format, ...) \
	utils::Log::LogAdd(utils::Log::LogChannel::Info, tag, utils::Format(format, __VA_ARGS__), utils::GetCurrentThreadID(), std::source_location::current());

#define VERBOSE_LOG(tag, format, ...) \
	utils::Log::LogAdd(utils::Log::LogChannel::Verbose, tag, utils::Format(format, __VA_ARGS__), utils::GetCurrentThreadID(), std::source_location::current());