#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <vector>

struct TimerTask
{
    void (*callback)();
    unsigned long interval;
    unsigned long lastRun;
    bool isRunning = false;
};

class TaskTimer
{
private:
    static TimerTask emptyTask;
    std::vector<TimerTask> tasks;
    TimerTask& getTask(void (*callback)());

public:
    TaskTimer();
    ~TaskTimer();

    void add(void (*callback)(), unsigned long interval);
    void add(void (*callback)(), unsigned long interval, bool isRunning);
    void remove(void (*callback)());
    void tick();
    void setTaskRunning(void (*callback)(), bool isRunning);
    void setTaskInterval(void (*callback)(), unsigned long interval);
};