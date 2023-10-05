#pragma once
#include "MessageQueue.h"
#pragma warning( error : 4858)

namespace utils
{
	class Connection;
	namespace details
	{
		template <typename ThreadModel, typename Signature>
		class ConnectionImp;
	}
	template <class ThreadModel, class Signature, class AccessKey>
	struct SignalHolder;
}

namespace utils
{
	template <typename ThreadModel, typename Signature, typename AccessKey>
	class SignalImpl : private MessageQueueImpl<ThreadModel, Signature>, public ObjectLifeIndicator
	{
	public:
		SignalImpl() = default;
		SignalImpl(WorkerThread<Signature>* i_signalThread) : MessageQueueImpl<ThreadModel, Signature>(MAX_SIZE, i_signalThread)
		{
		}
		~SignalImpl()
		{
		}

		SignalImpl& operator=(SignalImpl&& other)
		{
			return static_cast<SignalImpl&>(MessageQueueImpl<ThreadModel, Signature>::operator=(std::move(other)));
		}

		using signature_t = Signature;
		using threadmodel_t = ThreadModel;
		template < typename _Ret, typename _Obj, typename... Args2>
		_NODISCARD_TRY_CHANGE_STATE inline Connection Connect(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj> ctx)
		{
			std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, ctx);
			return Connect(_Functor);
		}

		template < typename _Ret, typename _Obj, typename... Args2>
		_NODISCARD_TRY_CHANGE_STATE inline Connection Connect(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj*> ctx)
		{
			std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, *ctx);
			return Connect(_Functor);
		}

		template < typename _Ret, typename _Obj, typename... Args2>
		_NODISCARD_TRY_CHANGE_STATE inline Connection Connect(_Ret(_Obj::* callback)(Args2...), _Obj* ctx)
		{
			std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, ctx);
			return Connect(_Functor);
		}

		template < typename _Ret, typename... Args2>
		_NODISCARD_TRY_CHANGE_STATE inline Connection Connect(_Ret(*callback)(Args2...))
		{
			return Connect<_Ret(Args2...)>(callback);
		}

		template <typename _Call>
		_NODISCARD_TRY_CHANGE_STATE inline Connection Connect(_Call callback)
		{
			typename MessageQueueImpl<ThreadModel, Signature>::func_type _Functor (callback);
			return Connect<Signature>(_Functor);
		}

		template <typename T>
		_NODISCARD_TRY_CHANGE_STATE inline Connection Connect(std::function<T> callback)
		{
			Result<uint32_t, MessageQueueERR> connectResult = this->AddSlot(callback);
			if (connectResult.isOk())
			{
				return utils::Connection(std::make_unique<details::ConnectionImp<ThreadModel, Signature>>(static_cast<MessageQueueImpl<ThreadModel, Signature>*>(this), connectResult.unwrap()));
			}
			else
			{
				return utils::Connection();
			}
		}

	protected:
		template <class ThreadModel, class Signature, class AccessKey>
		friend struct SignalHolder;

		template <typename... Args>
		void Emit(Args&&... args)
		{
			if (this->IsObjectStillAlive())
			{
				this->Broadcast(std::forward<Args>(args)...);
			}
		}

		template <typename... Args>
		void EmitAsync(Args&&... args)
		{
			if (this->IsObjectStillAlive())
			{
				this->BroadcastAsync(std::forward<Args>(args)...);
			}
		}
	};

	template <class ThreadModel, class Signature, class AccessKey>
	struct SignalHolder
	{
	public:
		SignalHolder(SignalImpl<ThreadModel, Signature, AccessKey>& i_signal) : m_signal(i_signal)
		{
		}

		template <typename... Args>
		void Emit(Args&&... args)
		{
			m_signal.Emit(std::forward<Args>(args)...);
		}

		template <typename... Args>
		void EmitAsync(Args&&... args)
		{
			m_signal.EmitAsync(std::forward<Args>(args)...);
		}

	private:
		SignalImpl<ThreadModel, Signature, AccessKey>& m_signal;
	};

	template <class AccessKey, class Emitter>
	SignalHolder<typename Emitter::threadmodel_t, typename Emitter::signature_t, AccessKey> Access(Emitter& i_emitter)
	{
		return SignalHolder<typename Emitter::threadmodel_t, typename Emitter::signature_t, AccessKey>(i_emitter);
	}

	namespace details
	{
		class IConnection
		{
		public:
			virtual ~IConnection() = default;
			virtual void Disconnect() = 0;
			virtual void Lock() = 0;
			virtual void Unlock() = 0;
			virtual bool IsConnectionStillValid() const = 0;
			virtual bool HasConnectedSlots() const = 0;
		};

		template <typename ThreadModel, typename R, typename ...Args>
		class ConnectionImp<ThreadModel, R(Args...)> : public IConnection
		{
		public:
			ConnectionImp(MessageQueueImpl<ThreadModel, R(Args...)>* i_msgQueue, uint32_t i_slot) : m_msgQueue(i_msgQueue), m_slot(i_slot)
			{
				ConnectMessageQueueDestroyHandler();
			}

			ConnectionImp() = default;
			ConnectionImp(ConnectionImp<ThreadModel, R(Args...)> const& other) = delete;
			ConnectionImp(ConnectionImp<ThreadModel, R(Args...)>&& other) noexcept
			{
				if (other.IsConnectionStillValid())
				{
					Move(std::move(other));
					ConnectMessageQueueDestroyHandler();
				}
			}

			ConnectionImp<ThreadModel, R(Args...)>& operator=(ConnectionImp<ThreadModel, R(Args...)>&& other) noexcept
			{
				// Guard self assignment
				if (this == &other)
				{
					return *this;
				}
				Move(std::move(other));
				ConnectMessageQueueDestroyHandler();
				return *this;
			}
			ConnectionImp<ThreadModel, R(Args...)> operator=(ConnectionImp<ThreadModel, R(Args...)> const& other) = delete;

			~ConnectionImp()
			{
				Disconnect();
			}

			void Disconnect() override
			{
				if (IsConnectionStillValid())
				{
					m_msgQueue->DisconnectSlot(m_slot.value());
					if (!m_destroyHandle.has_value())
					{
						return;
					}
					m_msgQueue->CallSlot(m_destroyHandle.value(), ref_tuple(m_msgQueue->m_params));
				}
			}

			void Lock() override
			{
				if (IsConnectionStillValid())
				{
					m_msgQueue->LockSlot(m_slot.value(), true);
				}
			}

			void Unlock() override
			{
				if (IsConnectionStillValid())
				{
					m_msgQueue->LockSlot(m_slot.value(), false);
				}
			}

			bool IsConnectionStillValid() const override
			{
				return m_msgQueue != nullptr && m_slot.has_value();
			}

			bool HasConnectedSlots() const override
			{
				return m_msgQueue != nullptr && !m_msgQueue->m_queue.empty();
			}

		private:
			void Move(ConnectionImp<ThreadModel, R(Args...)>&& other)
			{
				m_msgQueue = std::exchange(other.m_msgQueue, nullptr);
				m_slot = std::exchange(other.m_slot, std::optional<uint32_t>());
				m_destroyHandle = std::exchange(other.m_destroyHandle, std::optional<uint32_t>());
			}
			void Copy(ConnectionImp<ThreadModel, R(Args...)> const& other)
			{
				m_msgQueue = other.m_msgQueue;
				m_slot = other.m_slot;
				m_destroyHandle = other.m_destroyHandle;
			}
			void ConnectMessageQueueDestroyHandler()
			{
				if (m_destroyHandle.has_value())
				{
					m_msgQueue->DisconnectSlot(m_destroyHandle.value());
				}
				std::function<R(Args...)> destroyHandle = std::bind_front(&ConnectionImp<ThreadModel, R(Args...)>::OnSignalDestroyHandle, this);
				Result<uint32_t, MessageQueueERR> enqueueResult = m_msgQueue->AddSlot(destroyHandle, true);
				if (enqueueResult.isOk())
				{
					m_destroyHandle = enqueueResult.unwrap();
				}
			}
			typename MessageQueueImpl<ThreadModel, R(Args...)>::ret_type OnSignalDestroyHandle(Args&&... args)
			{
				m_msgQueue = nullptr;
				m_slot.reset();
				m_destroyHandle.reset();
			}

			MessageQueueImpl<ThreadModel, R(Args...)>* m_msgQueue = nullptr;
			std::optional<uint32_t> m_slot;
			std::optional<uint32_t> m_destroyHandle;
		};
	}

	class Connection : public HolderImpl<details::IConnection>
	{
	public:
		Connection() = default;
		Connection(std::unique_ptr<details::IConnection> i_connectionImpl) : HolderImpl<details::IConnection>(std::move(i_connectionImpl))
		{
		}
		Connection(Connection&& other) noexcept : HolderImpl<details::IConnection>(std::move(other))
		{
		}
		Connection& operator=(Connection&& other) noexcept
		{
			return static_cast<Connection&>(HolderImpl<details::IConnection>::operator=(std::move(other)));
		}

		~Connection()
		{
			if (m_holderImpl)
			{
				m_holderImpl.reset();
			}
		}

		void Disconnect()
		{
			if (m_holderImpl)
			{
				m_holderImpl->Disconnect();
			}
		}
		void Lock()
		{
			if (m_holderImpl)
			{
				m_holderImpl->Lock();
			}
		}
		void Unlock()
		{
			if (m_holderImpl)
			{
				m_holderImpl->Unlock();
			}
		}
		bool IsConnectionStillValid() const
		{
			if (m_holderImpl)
			{
				m_holderImpl->IsConnectionStillValid();
			}
			return false;
		}
		bool HasConnectedSlots() const
		{
			if (m_holderImpl)
			{
				m_holderImpl->HasConnectedSlots();
			}
			return false;
		}
	};

	template<class Signature, class AccessKey> using Signal_st = SignalImpl<details::threading::threadmodel_st, Signature, AccessKey>;
	template<class Signature, class AccessKey> using Signal_st_da = SignalImpl<details::threading::threadmodel_st_da, Signature, AccessKey>;
	template<class Signature, class AccessKey> using Signal_mt = SignalImpl<details::threading::threadmodel_mt, Signature, AccessKey>;
	template<class Signature, class AccessKey> using Signal = Signal_st<Signature, AccessKey>;
}