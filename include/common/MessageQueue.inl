#pragma once

namespace utils
{
	template <typename ThreadModel, typename T>
	MessageQueueImpl<ThreadModel, T>::MessageQueueImpl(uint32_t i_maxQueue, WorkerThread<T>* i_signalThread) : m_maxQueue(i_maxQueue), m_internalLock(false), m_signalThread(i_signalThread), m_threadChecker()
	{
	}

	template <typename ThreadModel, typename T>
	template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
	Result<uint32_t, MessageQueueERR> MessageQueueImpl<ThreadModel, T>::PushCallback(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj> ctx, Args3&&... args)
	{
		if (callback != nullptr)
		{
			assert(callback, "NULL CALLBACK DEFINED HERE");
			std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, ctx);
			return PushCallback(_Functor, std::forward<Args3>(args)...);
		}
		return Err(MessageQueueERR::CALLBACK_NULL_ERR);
	}

	template <typename ThreadModel, typename T>
	template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
	Result<uint32_t, MessageQueueERR> MessageQueueImpl<ThreadModel, T>::PushCallback(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj*> ctx, Args3&&... args)
	{
		if (callback != nullptr)
		{
			assert(callback, "NULL CALLBACK DEFINED HERE");
			std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, *ctx);
			return PushCallback(_Functor, std::forward<Args3>(args)...);
		}
		return Err(MessageQueueERR::CALLBACK_NULL_ERR);
	}

	template <typename ThreadModel, typename T>
	template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
	Result<uint32_t, MessageQueueERR> MessageQueueImpl<ThreadModel, T>::PushCallback(_Ret(_Obj::* callback)(Args2...), _Obj* ctx, Args3&&... args)
	{
		if (callback != nullptr)
		{
			assert(callback, "NULL CALLBACK DEFINED HERE");
			std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, ctx);
			return PushCallback(_Functor, std::forward<Args3>(args)...);
		}
		return Err(MessageQueueERR::CALLBACK_NULL_ERR);
	}

	template <typename ThreadModel, typename T>
	template < typename _Ret, typename... Args2, typename... Args3>
	Result<uint32_t, MessageQueueERR> MessageQueueImpl<ThreadModel, T>::PushCallback(_Ret(*callback)(Args2...), Args3&&... args)
	{
		if (callback != nullptr)
		{
			assert(callback, "NULL CALLBACK DEFINED HERE");
			return PushCallback<_Ret(Args2...)>(callback, std::forward<Args3>(args)...);
		}
		return Err(MessageQueueERR::CALLBACK_NULL_ERR);
	}

	template <typename ThreadModel, typename T>
	template < typename _Call, typename... Args2>
	Result<uint32_t, MessageQueueERR> MessageQueueImpl<ThreadModel, T>::PushCallback(_Call callback, Args2&&... args)
	{
		func_type _Functor = std::bind(callback, std::forward<Args2>(args)...);
		return PushCallback(_Functor);
	}

	template <typename ThreadModel, typename T>
	template < typename _Ret, typename... Args2 >
	Result<uint32_t, MessageQueueERR> MessageQueueImpl<ThreadModel, T>::PushCallback(std::function<_Ret(Args2...)> callback, Args2&&... args)
	{
		func_type _Functor = std::bind(callback, std::forward<Args2>(args)...);
		return AddSlot(_Functor);
	}

	template <typename ThreadModel, typename T>
	Result<uint32_t, MessageQueueERR> MessageQueueImpl<ThreadModel, T>::AddSlot(std::function<T> callback, bool i_isDestroyHandler)
	{
		if (callback == nullptr)
		{
			assert(callback, "NULL CALLBACK DEFINED HERE");
			return Err(MessageQueueERR::CALLBACK_NULL_ERR);
		}
		m_threadChecker.check_thread_id();
		if (m_internalLock)
		{
			assert(m_internalLock);
			return Err(MessageQueueERR::BLOCKING_ERR);
		}
		std::lock_guard guard(this->m_mutex);
		if (m_queue.size() < this->m_maxQueue)
		{
			uint32_t slot = InternalCalcSlot();
			const bool lock = i_isDestroyHandler;
			m_queue.push_back(Message(i_isDestroyHandler, lock, slot, callback));
			return Ok(slot);
		}
		else
		{
			return Err(MessageQueueERR::PUSH_MQ_ERR_EXCEED);
		}
	}

	template <typename ThreadModel, typename T>
	template <typename... Args>
	Result<uint32_t, MessageQueueERR> MessageQueueImpl<ThreadModel, T>::Dispatch(Args&&... args)
	{
		return DispatchInternal(m_queue.size(), false, std::forward<Args>(args)...);
	}

	template <typename ThreadModel, typename T>
	template <typename... Args>
	Result<uint32_t, MessageQueueERR> MessageQueueImpl<ThreadModel, T>::Dispatch(uint32_t i_max, Args&&... args)
	{
		return DispatchInternal(i_max, false, std::forward<Args>(args)...);
	}

	template <typename ThreadModel, typename T>
	template <typename... Args>
	Result<uint32_t, MessageQueueERR> MessageQueueImpl<ThreadModel, T>::DispatchInternal(size_t i_max, bool i_isEmit, Args&&... args)
	{
		m_threadChecker.check_thread_id();
		if (m_internalLock)
		{
			assert(m_internalLock);
			return Err(MessageQueueERR::BLOCKING_ERR);
		}
		size_t dispatchedCount = 0;
		Message message;
		this->m_mutex.lock();
		while (!m_queue.empty() && IsGreaterThan(dispatchedCount, { i_max,  m_queue.size() }))
		{
			if (i_isEmit)
			{
				message = m_queue[dispatchedCount];
			}
			else
			{
				message = std::move(m_queue.front());
				m_queue.erase(m_queue.begin());
			}
			this->m_mutex.unlock();
			Epilogue lock([&]()
			{
				++dispatchedCount;
				this->m_mutex.lock();
			});
			if (message.msg == nullptr || message.lock)
			{
				continue;
			}
			message(std::forward<Args>(args)...);
		}
		this->m_mutex.unlock();
		return Ok((uint32_t) dispatchedCount);
	}

	template <typename ThreadModel, typename T>
	template <typename... Args>
	void MessageQueueImpl<ThreadModel, T>::Broadcast(Args&&... args)
	{
		DispatchInternal(m_queue.size(), true, std::forward<Args>(args)...);
	}

	template <typename ThreadModel, typename T>
	template <typename... Args>
	void MessageQueueImpl<ThreadModel, T>::BroadcastAsync(Args&&... args)
	{
		if (m_signalThread != nullptr)
		{
			m_threadChecker.check_thread_id();
			if (m_internalLock)
			{
				assert(m_internalLock);
				return;
			}
			uint32_t dispatchedCount = 0;
			Message message;
			std::lock_guard lk(m_mutex);
			while (dispatchedCount < m_queue.size())
			{
				message = m_queue[dispatchedCount];
				if (!message.lock && message.msg)
				{
					m_signalThread->PushCallback(message.msg, std::forward<Args>(args)...);
				}
				++dispatchedCount;
			}
		}
	}

	template <typename ThreadModel, typename T>
	typename std::deque<typename MessageQueueImpl<ThreadModel, T>::Message>::iterator MessageQueueImpl<ThreadModel, T>::GetSlotIt(uint32_t i_slot)
	{
		if (m_internalLock)
		{
			assert(m_internalLock);
			return m_queue.end();
		}
		auto slotToBeDisconnectedIt = std::find_if(m_queue.begin(), m_queue.end(), [i_slot](Message const& i_msg)
		{
			return i_msg.slot == i_slot;
		});
		return slotToBeDisconnectedIt;
	}

	template <typename ThreadModel, typename T>
	void MessageQueueImpl<ThreadModel, T>::DisconnectSlot(uint32_t i_slot)
	{
		m_threadChecker.check_thread_id();
		auto slotToBeDisconnectedIt = GetSlotIt(i_slot);
		std::lock_guard lk(m_mutex);
		if (slotToBeDisconnectedIt != m_queue.end())
		{
			m_queue.erase(slotToBeDisconnectedIt);
			m_reserveSlots.push(i_slot);
		}
	}

	template <typename ThreadModel, typename T>
	void MessageQueueImpl<ThreadModel, T>::LockSlot(uint32_t i_slot, bool i_isLock)
	{
		m_threadChecker.check_thread_id();
		auto slotIt = GetSlotIt(i_slot);
		std::lock_guard lk(m_mutex);
		if (slotIt != m_queue.end())
		{
			slotIt->lock = i_isLock;
		}
	}

	template <typename ThreadModel, typename T>
	template <typename... Args>
	void MessageQueueImpl<ThreadModel, T>::CallSlot(uint32_t i_slot, Args&&... args)
	{
		m_threadChecker.check_thread_id();
		auto slotIt = GetSlotIt(i_slot);
		std::lock_guard lk(m_mutex);
		if (slotIt != m_queue.end())
		{
			(*slotIt)(std::forward<Args>(args)...);
			if (slotIt->isDestroyHandler)
			{
				m_queue.erase(slotIt);
				m_reserveSlots.push(i_slot);
			}
		}
	}

	template <typename ThreadModel, typename T>
	void MessageQueueImpl<ThreadModel, T>::OnDestroy()
	{
		m_internalLock = true;
		while (!m_queue.empty())
		{
			Message message = std::move(m_queue.front());
			m_queue.erase(m_queue.begin());
			if (message.isDestroyHandler)
			{
				message(ref_tuple(m_params));
			}
		}
	}

	template <typename ThreadModel, typename T>
	void MessageQueueImpl<ThreadModel, T>::SetThreadId(const std::thread::id& i_threadId)
	{
		m_threadChecker.set_thread_id(i_threadId);
	}
}