#pragma once
#include <unordered_map>
#include <wtypes.h>
#include <functional>
#include <memory>

enum class TimerType {
	LayoutSwitch,
	UI,
	none
};

struct StateEvent {
	HWND hWnd;
	TimerType timerType;
};

struct Timer {
public:
	Timer(HWND hWnd, UINT_PTR timerId, TimerType tt);
	Timer(const Timer&) = delete;
	Timer& operator=(const Timer&) = delete;
	~Timer();

	void Start(uint32_t periodMs);
	void Stop();
	void Reschedule(uint32_t periodMs);
	bool IsRunning();

	TimerType m_type;
	UINT_PTR m_id;
	HWND m_hWnd{};
	bool m_running{false};
};

class StateManager
{
private:
	StateManager() = default;
	StateManager(const StateManager&) = delete; // Delete copy constructor
	StateManager& operator=(const StateManager&) = delete; // Delete assignment operator
public:
	using Callback = std::function<bool(const StateEvent&)>;

	void SetCallback(Callback cb);
	static StateManager& GetInstance();
	void RegisterTimer(std::unique_ptr<Timer>);
	Timer* onTimer(TimerType tt);
	void Shutdown();

	TimerType GetType(UINT_PTR id);

	void callback(StateEvent se);
private:
	Callback m_callback{nullptr};
	std::unordered_map<TimerType, std::unique_ptr<Timer>> timers;
};

	VOID CALLBACK TimerProc(HWND hWnd, UINT message, UINT_PTR idTimer, DWORD dwTime);