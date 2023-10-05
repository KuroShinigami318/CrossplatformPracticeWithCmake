#pragma once
#include "Utils.h"
#include "threading.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4002 26110 26117 4244 26800)
#endif

namespace utils
{
	template <typename Signature> class WorkerThread;
	namespace details
	{
		template <typename ThreadModel, typename T>
		class ConnectionImp;
	}
}

namespace utils
{
	template <typename ThreadModel, typename T>
	class MessageQueueImpl
	{
	public:
		static_assert(std::is_function_v<T>, "Please provide a valid function!");
		using func_type = std::function<T>;
		using ret_type = typename func_type::result_type;

		MessageQueueImpl(uint32_t i_maxQueue = MAX_SIZE, WorkerThread<T>* i_signalThread = nullptr);
		~MessageQueueImpl()
		{
			OnDestroy();
		}

		MessageQueueImpl& operator=(MessageQueueImpl&& other)
		{
			if (this == &other)
			{
				return *this;
			}
			Move(std::move(other));
			return *this;
		}

		template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
		Result<uint32_t, MessageQueueERR> PushCallback(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj> ctx, Args3&&... args);

		template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
		Result<uint32_t, MessageQueueERR> PushCallback(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj*> ctx, Args3&&... args);

		template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
		Result<uint32_t, MessageQueueERR> PushCallback(_Ret(_Obj::* callback)(Args2...), _Obj* ctx, Args3&&... args);

		template < typename _Ret, typename... Args2, typename... Args3>
		Result<uint32_t, MessageQueueERR> PushCallback(_Ret(*callback)(Args2...), Args3&&... args);

		template < typename _Call, typename... Args2>
		Result<uint32_t, MessageQueueERR> PushCallback(_Call callback, Args2&&... args);

		template < typename _Ret, typename... Args2 >
		Result<uint32_t, MessageQueueERR> PushCallback(std::function<_Ret(Args2...)> callback, Args2&&... args);

		Result<uint32_t, MessageQueueERR> AddSlot(std::function<T> callback, bool i_isDestroyHandler = false);

		template <typename... Args>
		Result<uint32_t, MessageQueueERR> Dispatch(Args&&... args);

		// return dispatched message
		template <typename... Args>
		Result<uint32_t, MessageQueueERR> Dispatch(uint32_t i_max, Args&&... args);

		void SetThreadId(const std::thread::id& i_threadId);

	protected:
		template <typename... Args>
		void Broadcast(Args&&... args);
		template <typename... Args>
		void BroadcastAsync(Args&&... args);
		template <typename... Args>
		Result<uint32_t, MessageQueueERR> DispatchInternal(size_t i_max, bool i_isEmit, Args&&... args);

	private:
		struct Message;
		typename std::deque<Message>::iterator GetSlotIt(uint32_t i_slot);
		void DisconnectSlot(uint32_t i_slot);
		void LockSlot(uint32_t i_slot, bool i_isLock);
		template <typename... Args>
		void CallSlot(uint32_t i_slot, Args&&... args);
		void OnDestroy();

		uint32_t InternalCalcSlot()
		{
			if (m_reserveSlots.empty())
			{
				return static_cast<uint32_t>(m_queue.size());
			}
			else
			{
				uint32_t slot = m_reserveSlots.front();
				m_reserveSlots.pop();
				return slot;
			}
		}

	private:
		template <typename ThreadModel, typename T>
		friend class details::ConnectionImp;

	private:
		struct Message
		{
		public:
			using msg_type = func_type;

			Message() = default;

			Message(bool i_lock, uint32_t i_slot, msg_type i_msg) : Message(false, i_lock, i_slot, i_msg)
			{
			}

			Message(bool i_isDestroyHandler, bool i_lock, uint32_t i_slot, msg_type i_msg) : isDestroyHandler(i_isDestroyHandler), lock(i_lock), slot(i_slot), msg(i_msg)
			{
			}

			bool isDestroyHandler = false;
			bool lock = true;
			uint32_t slot = 0;
			msg_type msg = nullptr;

		public:
			template <typename... Args>
			ret_type operator()(Args&&... args)
			{
				return call(msg, std::forward<Args>(args)...);
			}
		};

		void Move(MessageQueueImpl&& other)
		{
			m_queue = other.m_queue;
			m_reserveSlots = std::move(other.m_reserveSlots);
			m_internalLock = std::move(other.m_internalLock);
			m_maxQueue = std::move(other.m_maxQueue);
			m_signalThread = std::exchange(other.m_signalThread, nullptr);
			other.OnDestroy();
			m_params = std::move(other.m_params);
		}

		bool m_internalLock;
		uint32_t m_maxQueue;
		std::deque<Message> m_queue;
		std::queue<uint32_t> m_reserveSlots;
		typename ThreadModel::recursive_mutex_t m_mutex;
		typename function_traits<func_type>::decay_tuple_params m_params;
		typename ThreadModel::threadchecker_t m_threadChecker;
		WorkerThread<T>* m_signalThread;
	};

	template <typename Signature> using MessageQueue_st = MessageQueueImpl<details::threading::threadmodel_st, Signature>;
	template <typename Signature> using MessageQueue_st_da = MessageQueueImpl<details::threading::threadmodel_st_da, Signature>;
	template <typename Signature> using MessageQueue_mt = MessageQueueImpl<details::threading::threadmodel_mt, Signature>;
	using MessageQueue = MessageQueue_mt<void()>;
}

#include "MessageQueue.inl"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif