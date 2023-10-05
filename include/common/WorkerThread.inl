#pragma once

namespace utils
{
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 26495 4002 4267 4244)
#endif
	template < typename _R, typename... Args >
	WorkerThread<_R(Args...)>::WorkerThread(const char* thread_name) : m_thread_name(thread_name), update(true), alive(true), m_mode(MODE::RUN_ONE_TIME),
		isTerminated(false), isProcesDone(true), oneTimeRun(OneTimeRunType::None), threads_count(0), didThreadStart(false), customRunnable(false)
	{

	}

	template < typename _R, typename... Args >
	template < typename T>
	WorkerThread<_R(Args...)>::WorkerThread(bool i_update, T* i_customRunnable, const char* thread_name) : WorkerThread(i_update, thread_name, MODE::UPDATE_CALLBACK, MAX_SIZE, false, std::thread::hardware_concurrency(), true)
	{
		static_assert(std::is_base_of_v<WorkerRunnable<_R()>, T>, "custom worker runnable must be derived from base with same signature of WorkerThread");
		if (i_customRunnable != nullptr)
		{
			this->customRunnable = true;
			this->workerRunnable = std::static_pointer_cast<WorkerRunnable<_R()>>(std::shared_ptr<T>(i_customRunnable, null_deleter()));
			FakeCreateWorkerThread();
		}
	}

	template < typename _R, typename... Args >
	WorkerThread<_R(Args...)>::WorkerThread(bool i_update, const char* thread_name, utils::MODE i_mode, unsigned int max, bool isThreadPool, unsigned int poolSize, bool i_customRunnable) : m_thread_name(thread_name), update(i_update), alive(true), m_mode(i_mode),
		isTerminated(false), isProcesDone(true), oneTimeRun(OneTimeRunType::None), threads_count(0), max_queue(max), m_poolSize(poolSize), didThreadStart(false), customRunnable(i_customRunnable)
	{
		if ((!update || m_mode != MODE::UPDATE_CALLBACK) && !i_customRunnable)
		{
			FakeCreateWorkerThread();
		}

		if (m_mode == MODE::MESSAGE_QUEUE_MT && isThreadPool && this->workerRunnable)
		{
			this->workerRunnable->thread_mode = THREAD_EXECUTION_MODE::THREAD_POOL;
		}
	}

	template < typename _R, typename... Args >
	template < typename _Ret, typename _Obj, typename... Args2, typename... Args3 >
	Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> WorkerThread<_R(Args...)>::CreateWorkerThread(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj> ctx, Args3&&... args)
	{
		std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, ctx);
		return CreateWorkerThread<_Ret(Args2...)>(_Functor, std::forward<Args3>(args)...);
	}

	template < typename _R, typename... Args >
	template < typename _Ret, typename _Obj, typename... Args2, typename... Args3 >
	Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> WorkerThread<_R(Args...)>::CreateWorkerThread(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj*> ctx, Args3&&... args)
	{
		std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, *ctx);
		return CreateWorkerThread<_Ret(Args2...)>(_Functor, std::forward<Args3>(args)...);
	}

	template < typename _R, typename... Args >
	template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
	Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> WorkerThread<_R(Args...)>::CreateWorkerThread(_Ret(_Obj::* callback)(Args2...), _Obj* ctx, Args3&&... args)
	{
		std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, ctx);
		return CreateWorkerThread<_Ret(Args2...)>(_Functor, std::forward<Args3>(args)...);
	}

	template < typename _R, typename... Args >
	template < typename _Ret, typename... Args2, typename... Args3>
	Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> WorkerThread<_R(Args...)>::CreateWorkerThread(_Ret(*callback)(Args2...), Args3&&... args)
	{
		return CreateWorkerThread<_Ret(Args2...)>(callback, std::forward<Args3>(args)...);
	}

	template<typename _R, typename ...Args>
	template<typename _Ret, typename ...Args2>
	Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> WorkerThread<_R(Args...)>::CreateWorkerThread(_Ret callback, Args2&& ...args)
	{
		return CreateWorkerThread(std::function(callback), std::forward<Args2>(args)...);
	}

	template<typename _R, typename ...Args>
	template<typename _Ret, typename... Args2>
	Result<utils::WorkerThread<_R(Args...)>*, WorkerThreadERR> WorkerThread<_R(Args...)>::CreateWorkerThread(std::function<_Ret(Args2...)> callback, Args2&&... args)
	{
		if (this->workerRunnable && !this->customRunnable)
		{
			return Err(WorkerThreadERR::IS_ALREADY_CREATED);
		}
		if (!callback)
		{
			assert(callback, "NULL CALLBACK DEFINED HERE");
			this->alive = false;
			return Err(WorkerThreadERR::CALLBACK_NULL_ERR);
		}
		this->m_callback = std::bind(callback, std::forward<Args2>(args)...);
		if (!this->customRunnable)
		{
			this->workerRunnable = std::make_shared<WorkerRunnable<_R()>>();
		}
		this->workerRunnable->thread_mode = THREAD_EXECUTION_MODE::NONE;
		if (!this->workerRunnable)
		{
			assert(this->workerRunnable, "Can not create worker runnable!");
			this->alive = false;
			utils::Log::e("Worker Thread", "Can not create worker runnable!");
			return Err(WorkerThreadERR::WORKER_RUNNABLE_CREATE_FAILED);
		}
		if (!this->workerRunnable->internalThread.has_value())
		{
			this->workerRunnable->internalThread = std::thread(&WorkerRunnable<_R()>::Run, this);
		}
		if (!this->workerRunnable->internalThread.has_value())
		{
			assert(this->workerRunnable->internalThread.has_value(), (std::to_string(error) + "Error in creating thread").c_str());
			this->workerRunnable.reset();
			return Err(WorkerThreadERR::CREATE_THREAD_FAILED);
		}
		return Ok(this);
	}

	template < typename _R, typename... Args >
	template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
	WorkerThread<_R(Args...)>::template message_handle WorkerThread<_R(Args...)>::PushCallback(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj> ctx, Args3&&... args)
	{
		if (callback != nullptr)
		{
			assert(callback, "NULL CALLBACK DEFINED HERE");
			std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, ctx);
			return PushCallback(_Functor, std::forward<Args3>(args)...);
		}
		return message_handle(WorkerThreadERR::CALLBACK_NULL_ERR);
	}

	template < typename _R, typename... Args >
	template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
	WorkerThread<_R(Args...)>::template message_handle WorkerThread<_R(Args...)>::PushCallback(_Ret(_Obj::* callback)(Args2...), std::shared_ptr<_Obj*> ctx, Args3&&... args)
	{
		if (callback != nullptr)
		{
			assert(callback, "NULL CALLBACK DEFINED HERE");
			std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, *ctx);
			return PushCallback(_Functor, std::forward<Args3>(args)...);
		}
		return message_handle(WorkerThreadERR::CALLBACK_NULL_ERR);
	}

	template < typename _R, typename... Args >
	template < typename _Ret, typename _Obj, typename... Args2, typename... Args3>
	WorkerThread<_R(Args...)>::template message_handle WorkerThread<_R(Args...)>::PushCallback(_Ret(_Obj::* callback)(Args2...), _Obj *ctx, Args3&&... args)
	{
		if (callback != nullptr)
		{
			assert(callback, "NULL CALLBACK DEFINED HERE");
			std::function<_Ret(Args2...)> _Functor = std::bind_front(callback, ctx);
			return PushCallback(_Functor, std::forward<Args3>(args)...);
		}
		return message_handle(WorkerThreadERR::CALLBACK_NULL_ERR);
	}

	template < typename _R, typename... Args >
	template < typename _Ret, typename... Args2, typename... Args3>
	WorkerThread<_R(Args...)>::template message_handle WorkerThread<_R(Args...)>::PushCallback(_Ret(*callback)(Args2...), Args3&&... args)
	{
		if (callback != nullptr)
		{
			assert(callback, "NULL CALLBACK DEFINED HERE");
			return PushCallback<_Ret(Args2...)>(callback, std::forward<Args3>(args)...);
		}
		return message_handle(WorkerThreadERR::CALLBACK_NULL_ERR);
	}

	template < typename _R, typename... Args >
	template < typename _Call, typename... Args2>
	WorkerThread<_R(Args...)>::template message_handle WorkerThread<_R(Args...)>::PushCallback(_Call callback, Args2&&... args)
	{
		func_type _Functor = std::bind(callback, std::forward<Args2>(args)...);
		return PushCallback(_Functor);
	}

	template < typename _R, typename... Args >
	template < typename _Ret, typename... Args2 >
	WorkerThread<_R(Args...)>::template message_handle WorkerThread<_R(Args...)>::PushCallback(std::function<_Ret(Args2...)> callback, Args2&&... args)
	{
		if (callback == nullptr)
		{
			assert(callback, "NULL CALLBACK DEFINED HERE");
			return message_handle(WorkerThreadERR::CALLBACK_NULL_ERR);
		}
		if (!alive)
		{
			return message_handle(WorkerThreadERR::ENQUEUE_MSG_WHILE_STOPPING_THREAD);
		}
		WorkerThreadERR returnType = WorkerThreadERR::SUCCESSS;
		switch (m_mode)
		{
			case MODE::MESSAGE_LOOP:
			case MODE::MESSAGE_QUEUE:
			case MODE::MESSAGE_QUEUE_MT:
			{
				std::lock_guard<std::recursive_mutex> guard(this->mutex);
				if (m_queue.size() < max_queue)
				{
					m_queue.push_back(std::move(packaged_message(std::move(std::make_unique<message_wrapper>(GenerateMessageId())), std::bind(callback, std::forward<Args2>(args)...))));
					m_queue.back().first->RegisterPriorityChangedCallback(std::bind_front(&WorkerThread<_R(Args...)>::OnMessagePriorityChanged, this));
					if (m_mode == MODE::MESSAGE_LOOP)
					{
						this->m_sizeToDispatch = m_queue.size();
						this->update = true;
					}
					message_handle msgHandle = message_handle(m_queue.back().first.get());
					msgHandle.RegisterCleanupCallback(std::bind_front(&WorkerThread<_R(Args...)>::OnClearCacheDispatchedMessages, this));
					return std::move(msgHandle);
				}
				else
				{
					returnType = WorkerThreadERR::PUSH_MQ_ERR_EXCEED;
				}
			}
			break;
			case MODE::UPDATE_CALLBACK:
			{
				this->mutex.lock();
				bool updateCached = this->update;
				bool l_isProcessDone = this->isProcesDone;
				if (!this->m_pauseReason.has_value() && updateCached)
				{
					this->m_pauseReason = PauseReason::Automatic;
				}
				else if (this->m_pauseReason == PauseReason::Automatic)
				{
					updateCached = true;
				}
				this->mutex.unlock();
				Pause(true);
				if (!l_isProcessDone)
				{
					returnType = WorkerThreadERR::UPDATE_CALLBACK_ERR;
					if (GetCurThreadId() != this->workerRunnable->internalThread->get_id())
					{
						utils::Log::d("WorkerThread::PushCallback", "Update callback before the process done! Before Wait");
						InterWait();
						utils::Log::d("WorkerThread::PushCallback", "Update callback before the process done! After Wait");
					}
				}
				this->m_callback = std::bind(callback, std::forward<Args2>(args)...);
				Pause(!updateCached);
			}
			break;
			default:
			{
				assert(false, "CAN'T USE PUSH CALLBACK IN THIS MODE");
				return message_handle(WorkerThreadERR::THIS_MODE_CAN_NOT_BE_USED);
			}
			break;
		}
		return message_handle(returnType);
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::Dispatch()
	{
		if (IsAny(this->m_mode.load(std::memory_order_acquire), {MODE::MESSAGE_QUEUE, MODE::MESSAGE_QUEUE_MT}))
		{
			std::unique_lock<std::recursive_mutex> guard(this->mutex);
			if (m_mode == MODE::MESSAGE_QUEUE)
			{
				this->m_sizeToDispatch = m_queue.size();
			}
			else
			{
				this->m_sizeToDispatch = this->m_poolSize;
			}
			this->update.store(true, std::memory_order_release);
		}
	}

	template < typename _R, typename... Args >
	Result<typename WorkerThread<_R(Args...)>::template packaged_message, WorkerThreadERR> WorkerThread<_R(Args...)>::Pop()
	{
		std::lock_guard<std::recursive_mutex> guard(this->mutex);
		if (m_sizeToDispatch > 0 && !m_queue.empty())
		{
			packaged_message msg = std::move(m_queue.front());
			m_queue.erase(m_queue.begin());
			m_sizeToDispatch--;
			return Ok(std::move(msg));
		}
		if (this->m_mode == utils::MODE::MESSAGE_LOOP)
		{
			return Err(WorkerThreadERR::SUCCESSS);
		}
		else if (m_sizeToDispatch <= 0)
		{
			return Err(WorkerThreadERR::PUSH_MQ_ERR_EXCEED);
		}
		else
		{
			return Err(WorkerThreadERR::CALLBACK_NULL_ERR);
		}
	}

	template < typename _R, typename... Args >
	Result<typename WorkerThread<_R(Args...)>::template packaged_message, WorkerThreadERR> WorkerThread<_R(Args...)>::Blocking_Pop(bool i_blocking)
	{
		std::unique_lock<std::recursive_mutex> guard(this->mutex);
		if (m_queue.empty())
		{
			if (i_blocking)
			{
				cv.wait(guard, [&] {return !m_queue.empty() || oneTimeRun == OneTimeRunType::Automatic_Trigger || !alive; });
				if (oneTimeRun == OneTimeRunType::Automatic_Trigger || !alive)
				{
					return Err(WorkerThreadERR::CALLBACK_NULL_ERR);
				}
			}
			else
			{
				return Err(WorkerThreadERR::CALLBACK_NULL_ERR);
			}
		}
		packaged_message msg = std::move(m_queue.front());
		m_queue.erase(m_queue.begin());
		return Ok(std::move(msg));
	}

	template < typename _R, typename... Args >
	size_t WorkerThread<_R(Args...)>::QueueSize() const
	{
		std::lock_guard<std::recursive_mutex> guard(const_cast<std::recursive_mutex&>(this->mutex));
		return this->m_queue.size();
	}

	template < typename _R, typename... Args >
	bool WorkerThread<_R(Args...)>::IsEmptyQueue() const
	{
		std::lock_guard<std::recursive_mutex> guard(const_cast<std::recursive_mutex&>(this->mutex));
		return this->m_queue.empty();
	}

	template < typename _R, typename... Args >
	bool WorkerThread<_R(Args...)>::IsProcesDone()
	{
		std::lock_guard<std::recursive_mutex> guard(this->mutex);
		return this->isProcesDone;
	}

	template < typename _R, typename... Args >
	bool WorkerThread<_R(Args...)>::HasAllMTProcessDone()
	{
		bool isEmptyQueue;
		uint32_t threadsCount;
		if (this->workerRunnable == nullptr)
		{
			return true;
		}
		{
			std::lock_guard<std::recursive_mutex> guard(this->mutex);
			isEmptyQueue = m_queue.empty();
			if (isEmptyQueue && this->workerRunnable->thread_mode == THREAD_EXECUTION_MODE::THREAD_POOL && this->oneTimeRun == OneTimeRunType::Waitable_Trigger)
			{
				this->oneTimeRun = OneTimeRunType::Automatic_Trigger;
				cv.notify_all();
			}
		}
		{
			std::lock_guard<std::mutex> guard(this->sync_mutex);
			threadsCount = threads_count;
		}
		return threadsCount == 0 && isEmptyQueue;
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::Clear()
	{
		std::lock_guard<std::recursive_mutex> guard(this->mutex);
		std::for_each(m_queue.begin(), m_queue.end(), [](packaged_message& message)
		{
			MessageHandleERR cancelResult = message.first->Cancel();
			if (cancelResult != MessageHandleERR::SUCCESS)
			{
				utils::Log::e("CancelError", Format("Cancel Error with code: {}", cancelResult));
			}
			else
			{
				message.first->OnMessageCanceled();
			}
		});
		std::for_each(m_cacheDispatchedMessages.begin(), m_cacheDispatchedMessages.end(), [](auto& message)
		{
			message.second.first->OnMessageCanceled();
		});
		m_queue.clear();
		m_cacheDispatchedMessages.clear();
	}

	template < typename _R, typename... Args >
	bool WorkerThread<_R(Args...)>::Joinable()
	{
		if (!this->workerRunnable)
		{
			return false;
		}
		return this->workerRunnable->internalThread->joinable() && this->workerRunnable->thread_mode != THREAD_EXECUTION_MODE::DETACH;
	}

	template < typename _R, typename... Args >
	WorkerThread<_R(Args...)>::~WorkerThread()
	{
		if (this->workerRunnable == nullptr || !this->workerRunnable->IsObjectStillAlive())
		{
			return;
		}
		if (this->IsObjectStillAlive())
		{
			this->Stop();
		}
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::Sleep(uint32_t sleepTimeMs)
	{
		std::this_thread::sleep_for(utils::milisecs(sleepTimeMs));
	}

	template < typename _R, typename... Args >
	std::thread::id WorkerThread<_R(Args...)>::GetCurThreadId()
	{
		return std::this_thread::get_id();
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::Pause(bool i_paused)
	{
		std::lock_guard<std::recursive_mutex> guard(this->mutex);
		this->update.store(!i_paused, std::memory_order_release);
		if (!this->m_pauseReason.has_value())
		{
			this->m_pauseReason = PauseReason::Manual;
		}
	}

	template < typename _R, typename... Args >
	bool WorkerThread<_R(Args...)>::IsPaused()
	{
		return !this->update;
	}

	template < typename _R, typename... Args >
	bool WorkerThread<_R(Args...)>::IsTerminated()
	{
		return this->isTerminated;
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::StopAsync()
	{
		std::unique_lock<std::recursive_mutex> guard(this->mutex);
		this->update = false;
		this->alive = false;
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::Stop()
	{
		if (Joinable())
		{
			{
				std::lock_guard<std::recursive_mutex> guard(this->mutex);
				this->update = false;
				this->alive = false;
				this->max_queue = 0;
			}
			assert(!this->isTerminated, "thread has been already terminated");
			assert(this->workerRunnable->thread_mode != THREAD_EXECUTION_MODE::DETACH, "trying to join detached thread");
			this->workerRunnable->internalThread->join();
		}
		else if (this->m_mode == MODE::RUN_ONE_TIME && this->update)
		{
			std::unique_lock<std::recursive_mutex> lk_guard(mutex);
			cv.wait(lk_guard, [&] {return !update; });
		}
		this->workerRunnable->OnCancel();
		this->workerRunnable.reset();
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::Suspend()
	{
		SuspendThread(this->workerRunnable->internalThread->native_handle());
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::Resume()
	{
		ResumeThread(this->workerRunnable->internalThread->native_handle());
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::Join()
	{
		if (Joinable())
		{
			this->workerRunnable->thread_mode = THREAD_EXECUTION_MODE::JOIN;
			this->workerRunnable->internalThread->join();
			this->workerRunnable.reset();
		}
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::Detach()
	{
		std::unique_lock<std::recursive_mutex> lk_guard(mutex);
		cv.wait(lk_guard, [&] {return didThreadStart; });
		this->workerRunnable->thread_mode = THREAD_EXECUTION_MODE::DETACH;
		this->workerRunnable->internalThread->detach();
	}

	template < typename _R, typename... Args >
	bool WorkerThread<_R(Args...)>::Wait()
	{
		if (DoesModeSupportWaitable())
		{
			MODE mode = this->m_mode;
			if (mode == MODE::MESSAGE_QUEUE_MT)
			{
				this->mutex.lock();
				oneTimeRun = OneTimeRunType::Waitable_Trigger;
				this->mutex.unlock();
				while (!this->HasAllMTProcessDone());
			}
			if (mode == MODE::MESSAGE_QUEUE)
			{
				this->mutex.lock();
				unsigned int sizeToDispatch = this->m_sizeToDispatch;
				bool i_isProcesDone = this->isProcesDone;
				while (sizeToDispatch > 0 || !i_isProcesDone)
				{
					this->mutex.unlock();
					sizeToDispatch = this->m_sizeToDispatch;
					i_isProcesDone = this->isProcesDone;
					this->mutex.lock();
				}
				this->mutex.unlock();
			}
			else
			{
				while (!(this->IsEmptyQueue() && this->IsProcesDone()));
			}
			return true;
		}
		return false;
	}

	template < typename _R, typename... Args >
	utils::MODE WorkerThread<_R(Args...)>::GetCurrentMode()
	{
		return m_mode;
	}

	template < typename _R, typename... Args >
	WorkerThreadERR WorkerThread<_R(Args...)>::ChangeMode(MODE mode, unsigned int max)
	{
		if (m_mode.load(std::memory_order_relaxed) != mode)
		{
			MODE old_mode = m_mode;
			switch (mode)
			{
				case MODE::UPDATE_CALLBACK:
				{
					std::lock_guard<std::mutex> guard(this->sync_mutex);
					if (old_mode == MODE::MESSAGE_QUEUE_MT)
					{
						if (threads_count > 0)
							return WorkerThreadERR::CHANGE_MODE_UPDATE_CB_ERR;
					}
					m_subThreads.clear();
					Clear();
					threads_count = 0;
				}
				break;
				case MODE::MESSAGE_QUEUE:
				case MODE::MESSAGE_QUEUE_MT:
				{
					max_queue = max;
					if (mode == MODE::MESSAGE_QUEUE)
					{
						if (this->workerRunnable->thread_mode == THREAD_EXECUTION_MODE::THREAD_POOL)
						{
							Wait();
							JoinThreadsIntoPool();
						}
						std::lock_guard<std::mutex> guard(this->sync_mutex);
						CleanSubThreads(true);
						assert(this->threads_count == 0);
					}
				}
				break;
				case MODE::MESSAGE_LOOP:
				{
					max_queue = max;
					update = true;
					if (this->workerRunnable->thread_mode == THREAD_EXECUTION_MODE::THREAD_POOL)
					{
						Wait();
						JoinThreadsIntoPool();
					}
					std::lock_guard<std::mutex> guard(this->sync_mutex);
					CleanSubThreads(true);
					assert(this->threads_count == 0);
				}
				break;
				default:
				{
					assert(false, "YOU CAN'T CHANGE TO THIS MODE");
					return WorkerThreadERR::THIS_MODE_CAN_NOT_BE_USED;
				}
				break;
			}
			m_mode.store(mode, std::memory_order_release);
			return WorkerThreadERR::SUCCESSS;
		}
		return WorkerThreadERR::SAME_PREVIOUS_MODE;
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::Update()
	{
		this->mutex.lock();
		this->didThreadStart = true;
		this->cv.notify_one();
		bool isAlive = this->alive && this->workerRunnable && this->workerRunnable->IsObjectStillAlive();
		this->mutex.unlock();
		while (isAlive)
		{
			if (update.load(std::memory_order_acquire))
			{
				switch (m_mode.load(std::memory_order_relaxed))
				{
					case MODE::MESSAGE_LOOP:
					case MODE::MESSAGE_QUEUE:
					{
						Result<packaged_message, WorkerThreadERR> popResult = Pop();
						if (popResult.isOk())
						{
							Epilogue<void()> epilogue = FlagSwitcher(this->mutex, this->isProcesDone, false);
							Call(std::move(popResult.expect("Unhandled exception")));
						}
						else if (popResult.unwrapErr() != WorkerThreadERR::SUCCESSS)
						{
							std::lock_guard<std::recursive_mutex> guard(this->mutex);
							this->m_sizeToDispatch = 0;
							this->update = false;
						}
					}
					break;
					case MODE::MESSAGE_QUEUE_MT:
					{
						bool empty = true;
						{
							std::lock_guard<std::recursive_mutex> guard(this->mutex);
							empty = m_queue.empty();
						}
						if (!empty && IsLesser(m_sizeToDispatch, { threads_count, m_subThreads.size() }))
						{
							{
								std::lock_guard<std::mutex> guard(this->sync_mutex);
								threads_count++;
							}
							std::future<uint32_t> subThread;
							if (this->workerRunnable->thread_mode == utils::THREAD_EXECUTION_MODE::THREAD_POOL)
							{
								subThread = std::async(std::launch::async, &WorkerThread<_R(Args...)>::MTPopQueue, this);
							}
							else
							{
								Result<packaged_message, WorkerThreadERR> popedMessageResult = this->Blocking_Pop(false);
								if (popedMessageResult.isErr())
								{
									Access<AccessKey>(sig_errorThrow).Emit(Format("MESSAGE_QUEUE_MT Pop failed: {}", popedMessageResult.unwrapErr()));
									std::lock_guard<std::mutex> guard(this->sync_mutex);
									threads_count--;
									continue;
								}
								subThread = std::async(std::launch::async, &WorkerThread<_R(Args...)>::CallAsync, this, std::move(popedMessageResult.expect("This should have never happened!")));
							}
							m_subThreads.push_back(std::move(subThread));
						}
						else
						{
							std::lock_guard<std::recursive_mutex> guard(this->mutex);
							this->m_sizeToDispatch = 0;
							this->update = false;
						}
					}
					break;
					case MODE::UPDATE_CALLBACK:
					{
						const bool& didWorkerRunnableOverride = this->workerRunnable->didWorkerRunnableOverride;
						this->mutex.lock();
						this->m_pauseReason.reset();
						this->isProcesDone = false;
						this->mutex.unlock();
						this->workerRunnable->OnRun();
						if (!didWorkerRunnableOverride)
						{
							assert(!didWorkerRunnableOverride, "runnable has been overridden!");
							if (!RunWithExHandle(this))
							{
								assert(true, "pausing update due to invalid callback!");
								std::lock_guard<std::recursive_mutex> guard(this->mutex);
								this->update = false;
								this->m_pauseReason = PauseReason::Automatic;
							}
						}
						std::lock_guard<std::recursive_mutex> guard(this->mutex);
						if (oneTimeRun == OneTimeRunType::Manual_Trigger)
						{
							this->update = false;
						}
						this->isProcesDone = true;
					}
					break;
					case MODE::RUN_ONE_TIME:
					{
						RunWithExHandle(this);
						if (IsObjectStillAlive())
						{
							utils::Access<AccessKey>(sig_onRunFinished).Emit();
						}
						this->alive = false;
						this->isTerminated = true;
						return;
					}
					break;
					default:
					{
						assert(false, "WorkerThread was destroyed!!!");
						utils::Log::e("WorkerThread", "WorkerThread was destroyed!!!");
						return;
					}
					break;
				}
				StopWaiting();
			}
			if (m_mode.load(std::memory_order_relaxed) == MODE::MESSAGE_QUEUE_MT)
			{
				if (!this->m_subThreads.empty() && threads_count == 0)
				{
					std::lock_guard<std::mutex> guard(this->sync_mutex);
					CleanSubThreads(false);
				}
				if (!IsEmptyQueue())
				{
					Dispatch();
				}
			}
			if (!update.load(std::memory_order_relaxed))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(k_updateIntervalMilliseconds));
			}
			this->mutex.lock();
			isAlive = this->alive && this->workerRunnable && this->workerRunnable->IsObjectStillAlive();
			this->mutex.unlock();
		}
		Clear();
		if (this->workerRunnable != nullptr)
		{
			if (this->workerRunnable->thread_mode == THREAD_EXECUTION_MODE::THREAD_POOL)
			{
				JoinThreadsIntoPool();
			}
			std::lock_guard<std::mutex> guard(this->sync_mutex);
			CleanSubThreads(true);
		}
		assert(this->threads_count == 0);
		this->isTerminated = true;
	}

	template<typename _R, typename ...Args>
	inline uint32_t WorkerThread<_R(Args...)>::MTPopQueue(void* value)
	{
		WorkerThread<_R(Args...)>* context = (WorkerThread<_R(Args...)>*) value;
		if (!context->workerRunnable)
		{
			return 0;
		}
		Epilogue cleanup(std::move([&]()
		{
			std::lock_guard<std::mutex> guard(context->sync_mutex);
			assert(context->threads_count > 0);
			context->threads_count--;
		}));
		do
		{
			Result<packaged_message, WorkerThreadERR> popedMessageResult = context->Blocking_Pop(true);
			if (!(popedMessageResult.isOk() && context->alive))
			{
				return 0;
			}
			context->Call(std::move(popedMessageResult.expect("Unhandled exception")));
		} while (context->workerRunnable && context->workerRunnable->thread_mode == utils::THREAD_EXECUTION_MODE::THREAD_POOL && context->alive);
		return 0;
	}

	template < typename _R, typename... Args >
	bool WorkerThread<_R(Args...)>::RunWithExHandle(void* value)
	{
		try
		{
			WorkerThread<_R(Args...)>* context = (WorkerThread<_R(Args...)>*) value;
			assert(context->m_callback != nullptr, "call empty callback");
			assert(context->workerRunnable, "Dead Thread!!!");
			if (context->m_callback == nullptr || context->isTerminated || !context->alive)
			{
				return false;
			}
			if (context->m_mode == MODE::RUN_ONE_TIME)
			{
				this->update = false;
				cv.notify_one();
			}
			context->m_callback();
			return true;
		}
		catch (const std::bad_exception& e)
		{
			assert(false, Format("Unknown Exception Catched when function called!! - {}", e.what()).c_str());
			utils::Log::e("WorkerThread::RunWithExHandle", Format("Unknown Exception Catched when function called!! - {}", e.what()));
			Access<AccessKey>(sig_errorThrow).Emit(std::string(e.what()));
			return false;
		}
		catch (const std::exception& e)
		{
			assert(false, Format("I don't know why this was occured!!! - {}", e.what()).c_str());
			utils::Log::e("WorkerThread::RunWithExHandle", Format("I don't know why this was occured!!! - {}", e.what()));
			Access<AccessKey>(sig_errorThrow).Emit(std::string(e.what()));
			return false;
		}
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::OnMessagePriorityChanged()
	{
		std::lock_guard<std::recursive_mutex> guard(mutex);
		std::sort(m_queue.begin(), m_queue.end(), [](packaged_message const& message1, packaged_message const& message2)
		{
			return message1.first->GetPriority() < message2.first->GetPriority();
		});
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::OnClearCacheDispatchedMessages(const size_t i_key)
	{
		if (this->IsObjectStillAlive())
		{
			std::lock_guard<std::recursive_mutex> guard(mutex);
			m_reserveSlots.push(i_key);
			m_cacheDispatchedMessages.erase(i_key);
		}
	}

	template < typename _R, typename... Args >
	void WorkerThread<_R(Args...)>::RunOneTime(bool i_isOneTimeRun)
	{
		std::lock_guard<std::recursive_mutex> guard(this->mutex);
		this->update = true;
		this->oneTimeRun = i_isOneTimeRun ? OneTimeRunType::Manual_Trigger : OneTimeRunType::None;
	}

	template < typename _R, typename... Args >
	bool WorkerThread<_R(Args...)>::IsOneTimeRun()
	{
		std::lock_guard<std::recursive_mutex> guard(this->mutex);
		return this->oneTimeRun == OneTimeRunType::Manual_Trigger;
	}

	template < typename _R>
	uint32_t WorkerRunnable<_R>::Run(void* value)
	{
		WorkerThread<_R>* thread_ctx = reinterpret_cast<WorkerThread<_R>*>(value);
#if defined(WINAPI_FAMILY)
		set_thread_name(GetCurrentThreadId(), thread_ctx->m_thread_name.c_str());
#endif
		thread_ctx->Update();
		return 0;
	}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}