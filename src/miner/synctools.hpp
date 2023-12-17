#pragma once
#include <condition_variable>
#include <mutex>
struct SyncTools {
    std::mutex m;
    std::condition_variable cv;
    bool wakeup;
};
