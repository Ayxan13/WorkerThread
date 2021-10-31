#pragma once

//
// Created by Ayxan Haqverdili on 2021-05-04
//

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <queue>

class WorkerThread final
{
public:
    using Task = std::function<void(void)>;

private:
    std::mutex mutex_;
    std::queue<Task> toDo_;
    std::condition_variable signal_;
    bool stop_ = false;
    std::thread thread_;

private:
    void ThreadMain() noexcept
    {
        for (Task task;;) {
            {
                std::unique_lock lock{ mutex_ };
                signal_.wait(lock, [this] { return (stop_ || !toDo_.empty()); });

                if (stop_ && toDo_.empty())
                    break;

                task = std::move(toDo_.front());
                toDo_.pop();
            }

            try {
                task();
            }
            catch (...) {
#if !defined(NDEBUG) && defined(_MSC_VER)
                __debugbreak();
#endif
            }
        }
    }

public:
    template<class... Func>
    void Schedule(Func&&... func)
    {
        {
            std::scoped_lock lock{ mutex_ };

            (toDo_.push(std::forward<Func>(func)), ...);
        }

        signal_.notify_one();
    }

    WorkerThread() : thread_(&WorkerThread::ThreadMain, this) {}

    ~WorkerThread()
    {
        {
            std::scoped_lock lock{ mutex_ };
            stop_ = true;
        }

        signal_.notify_one();
        thread_.join();
    }

    WorkerThread(WorkerThread const&) = delete;
    WorkerThread& operator=(WorkerThread const&) = delete;
    WorkerThread(WorkerThread&&) = delete;
    WorkerThread& operator=(WorkerThread&&) = delete;
};
