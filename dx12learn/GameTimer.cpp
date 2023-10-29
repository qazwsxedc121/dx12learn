#include "stdafx.h"
#include "GameTimer.h"

GameTimer::GameTimer()
: SecondsPerCount(0.0), DeltaTime(-1.0), BaseTime(0),
PausedTime(0), StopTime(0), PrevTime(0), CurrTime(0), Stopped(false)
{
    __int64 countsPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    SecondsPerCount = 1.0 / (double)countsPerSec;
}

float GameTimer::GetTotalTime() const
{
    if(Stopped)
    {
        return (float)(((StopTime - PausedTime)-BaseTime)*SecondsPerCount);
    }
    else
    {
        return (float)(((CurrTime - PausedTime)-BaseTime)*SecondsPerCount);
    }
}

float GameTimer::GetDeltaTime() const
{
    return (float)DeltaTime;
}

void GameTimer::Reset()
{
    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    BaseTime = currTime;
    PrevTime = currTime;
    StopTime = 0;
    Stopped  = false;
}

void GameTimer::Start()
{
    __int64 startTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
    
    if(Stopped)
    {
        PausedTime += (startTime - StopTime);

        PrevTime = startTime;
        StopTime = 0;
        Stopped  = false;
    }
}

void GameTimer::Stop()
{
    if(!Stopped)
    {
        __int64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

        StopTime = currTime;
        Stopped  = true;
    }
}

void GameTimer::Tick()
{
    if(Stopped)
    {
        DeltaTime = 0.0;
        return;
    }

    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
    CurrTime = currTime;

    DeltaTime = (CurrTime - PrevTime)*SecondsPerCount;

    PrevTime = CurrTime;

    if(DeltaTime < 0.0)
    {
        DeltaTime = 0.0;
    }
}
