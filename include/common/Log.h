#pragma once
#define __STDC_WANT_LIB_EXT1__ 1
#define __STDC_LIB_EXT1__ 1
#include "Utils.h"

#if !defined(_DEBUG)
#define ENABLE_WRITE_LOG
#endif
namespace utils
{
	std::string ltrim(const std::string& s);
	std::string rtrim(const std::string& s);
	std::string trim(const std::string& s);

	constexpr std::string ReduceFileName(const char* fileName)
	{
		std::string str_file(fileName);
		size_t startOff = str_file.find_last_of("\\");
		return str_file.substr(startOff + 1);
	}

	class Log
	{
	private:
		Log();
		struct SignalKey;
		inline static std::unique_ptr<WorkerThread<void()>> s_logThread;

		static WorkerThread<void()>& GetLogThread()
		{
			std::lock_guard<std::mutex> guard(s_logThreadMutex);
			if (s_logThread == nullptr)
			{
#if defined(ENABLE_WRITE_LOG)
				CheckAndCleanLog();
#endif
				s_logThread = std::make_unique<WorkerThread<void()>>(true, "Log Thread", s_logThreadMode);
			}

			return *s_logThread.get();
		}

	public:
		enum class LogChannel
		{
			Error,
			Debug,
			Verbose,
			Info,
			Warning
		};

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 28183 6387)
#endif
		static void LogAdd(const utils::Log::LogChannel channel, std::string tag, std::string msg, size_t threadID, const std::source_location location)
		{
			MessageHandle<void> addLogResult = GetLogThread().PushCallback([=]() {
				std::time_t time = std::time(nullptr);
#if defined(__STDC_LIB_EXT1__)
				std::string strTime{};
				struct tm temp_buff;
				char buff[50];
				localtime_s(&temp_buff, &time);
				if (std::strftime(buff, sizeof(buff), "%d-%m-%Y %H:%M:%S", &temp_buff))
				{
					strTime = buff;
				}
				else
				{
					asctime_s(buff, &temp_buff);
					strTime = buff;
					Access<SignalKey>(sig_errorThrow).Emit("strftime FAILED");
				}
#else
				std::string strTime = std::asctime(std::localtime(&time));
#endif
				std::string output;
				output = Format("[{}][ThreadID: {}][{}][{}][Line: {}][{}][{}]: {}\n", trim(strTime), threadID, ReduceFileName(location.file_name()), location.function_name(), location.line(), channel, tag, msg);
				std::lock_guard<std::mutex> guard(s_logThreadMutex);
				if (channel == LogChannel::Error)
				{
					std::cerr << output.c_str();
				}
				else
				{
					std::clog << output.c_str();
				}
#if defined(WINAPI_FAMILY) and defined(_DEBUG)
				OutputDebugStringA(output.c_str());
#endif
#if defined(ENABLE_WRITE_LOG)
				std::string logSaveLocation = GetSaveLogLocation();
				std::ofstream logFile;
				logFile.open(logSaveLocation, std::ios::out | std::ios::app);
				if (logFile)
				{
					logFile << output;
					logFile.close();
				}
#endif
			});
			if (addLogResult.IsError())
			{
				Access<SignalKey>(sig_errorThrow).Emit(Format("AddLog FAILED: {}", addLogResult.GetError()));
			}
			GetLogThread().Dispatch();
		}

	private:
		static std::string GetSaveLogLocation()
		{
			char* temp_buffer;
			size_t len;
			errno_t err = _dupenv_s(&temp_buffer, &len, "APPDATA");
			if (err)
			{
				Access<SignalKey>(sig_errorThrow).Emit(Format("Get environment FAILED: {}", err));
				return "";
			}
			std::string logSaveLocation = temp_buffer;
			free(temp_buffer);
			logSaveLocation += "\\BLog.txt";
			return std::move(logSaveLocation);
		}

		static bool CheckAndCleanLog()
		{
			std::string logSaveLocation = GetSaveLogLocation();
			std::filesystem::path path { logSaveLocation.c_str() };
			std::ofstream logFile;
			logFile.open(logSaveLocation, std::ios::out);
			if (logFile)
			{
				logFile.close();
			}
			uintmax_t size = std::filesystem::file_size(path);
			if (size > MAX_SIZE_LOG_CAN_STORE_IN_MB * 1024 * 1024)
			{
				logFile.open(logSaveLocation, std::ios::out);
				if (logFile)
				{
					logFile << "";
					logFile.close();
					return true;
				}
			}
			return false;
		}

		static const size_t MAX_SIZE_LOG_CAN_STORE_IN_MB = 5;

		inline static std::mutex s_logThreadMutex;

	public:
		static void v(std::string tag, std::string msg, const std::source_location location = std::source_location::current())
		{
			LogAdd(LogChannel::Verbose, tag, msg, utils::GetCurrentThreadID(), location);
		}

		static void e(std::string tag, std::string msg, const std::source_location location = std::source_location::current())
		{
			LogAdd(LogChannel::Error, tag, msg, utils::GetCurrentThreadID(), location);
		}

		static void d(std::string tag, std::string msg, const std::source_location location = std::source_location::current())
		{
			LogAdd(LogChannel::Debug, tag, msg, utils::GetCurrentThreadID(), location);
		}

		static void i(std::string tag, std::string msg, const std::source_location location = std::source_location::current())
		{
			LogAdd(LogChannel::Info, tag, msg, utils::GetCurrentThreadID(), location);
		}

		static void w(std::string tag, std::string msg, const std::source_location location = std::source_location::current())
		{
			LogAdd(LogChannel::Warning, tag, msg, utils::GetCurrentThreadID(), location);
		}

		static void Wait()
		{
			GetLogThread().Wait();
		}

		inline static Signal<void(std::string), SignalKey> sig_errorThrow;

		inline static MODE s_logThreadMode = MODE::MESSAGE_QUEUE_MT;
	};
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif