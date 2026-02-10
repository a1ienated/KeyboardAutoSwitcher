#include "stdafx.h"
#include "StateManager.h"

Timer::Timer(HWND hWnd, UINT_PTR timerId, TimerType tt)
{
	m_hWnd = hWnd;
	m_id = timerId;
	m_type = tt;
	m_running = false;
}

Timer::~Timer() { Stop(); }

StateManager& StateManager::GetInstance()
{
	static StateManager s_instance;
	return s_instance;
}

void Timer::Start(uint32_t periodMs)
{
	if (m_running) return;

	UINT_PTR ok = SetTimer(
		m_hWnd,
		m_id,
		periodMs,
		TimerProc);

	m_running = (ok == m_id);
}

void Timer::Reschedule(uint32_t periodMs)
{
	Stop();
	Start(periodMs);
}

void Timer::Stop()
{
	if (!m_running) return;

	m_running = false;
	KillTimer(m_hWnd, m_id);
}

bool Timer::IsRunning() { return m_running; }

void StateManager::SetCallback(Callback cb)
{
	m_callback = std::move(cb);
}

StateEvent MakeEvent(HWND hWnd, UINT_PTR idTimer, DWORD dwTime)
{
	StateEvent se;
	se.hWnd = hWnd;
	se.timerType = StateManager::GetInstance().GetType(idTimer);

	return se;
}

VOID CALLBACK TimerProc(
	HWND hWnd,        // handle to window for timer messages
	UINT message,     // WM_TIMER message
	UINT_PTR idTimer, // timer identifier
	DWORD dwTime)     // current system time
{
	const StateEvent se = MakeEvent(hWnd, idTimer, dwTime);
	
	StateManager::GetInstance().callback(se);
}

void StateManager::RegisterTimer(std::unique_ptr<Timer> p_timer)
{
	timers.emplace(p_timer->m_type, std::move(p_timer));
}

Timer* StateManager::onTimer(TimerType tt)
{
	std::unordered_map<TimerType, std::unique_ptr<Timer>>::const_iterator t = timers.find(tt);
	if (t != timers.end())
		return t->second.get();

	return nullptr;
}

TimerType StateManager::GetType(UINT_PTR id)
{
	for (auto& [key, value] : timers)
	{
		if (value.get()->m_id == id)
			return value.get()->m_type;
	}
	return TimerType::none;
}

void StateManager::callback(StateEvent se)
{
	if (m_callback) m_callback(se);
}

