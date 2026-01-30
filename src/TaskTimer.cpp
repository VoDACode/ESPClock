#include "TaskTimer.h"

TaskTimer::TaskTimer()
{
    this->tasks = std::vector<TimerTask>();
}

TaskTimer::~TaskTimer()
{
    this->tasks.clear();
}
void TaskTimer::add(void (*callback)(), unsigned long interval)
{
    this->add(callback, interval, false);
}
void TaskTimer::add(void (*callback)(), unsigned long interval, bool isRunning)
{
    TimerTask task;
    task.callback = callback;
    task.interval = interval;
    task.lastRun = 0;
    task.isRunning = isRunning;
    this->tasks.push_back(task);
}

void TaskTimer::remove(void (*callback)())
{
    for (int i = 0; i < this->tasks.size(); i++)
    {
        if (this->tasks[i].callback == callback)
        {
            this->tasks.erase(this->tasks.begin() + i);
            break;
        }
    }
}

void TaskTimer::tick()
{
    for (TimerTask &task : this->tasks)
    {
        if (millis() - task.lastRun > task.interval && task.isRunning)
        {
            task.callback();
            task.lastRun = millis();
        }
    }
}

void TaskTimer::setTaskRunning(void (*callback)(), bool isRunning)
{
    TimerTask &task = this->getTask(callback);
    if (&task == nullptr)
        return;
    task.isRunning = isRunning;
}

void TaskTimer::setTaskInterval(void (*callback)(), unsigned long interval)
{
    TimerTask &task = this->getTask(callback);
    if (&task == nullptr)
        return;
    task.interval = interval;
}

TimerTask& TaskTimer::getTask(void (*callback)())
{
    for (TimerTask &task : this->tasks)
    {
        if (task.callback == callback)
        {
            return task;
        }
    }
    return this->emptyTask;
}