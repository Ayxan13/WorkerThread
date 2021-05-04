#pragma once


//
// Created by Ayxan Haqverdili on 2021-05-04
//

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

class WorkerThread final {
public:
    using Task = std::function<void(void)>;

private:
    /* this mutex must be locked before
     * modifying state of this class */
    std::mutex _mutex;

    /* list of tasks to be executed */
    std::queue<Task> _toDo;

    /* The thread waits for this signal when
     * there are no tasks to be executed.
     * `notify_one` should be called to
     * wake up the thread and have it go
     * through the tasks. */
    std::condition_variable _signal;

    /* This flag is checked by the thread
     * before going to sleep. If it's set,
     * thread exits the event loop and terminates. */
    bool _stop = false;

    /* the thread is constructed at the
     * end so everything is ready by
     * the time it executes. */
    std::thread _thread;

private:
    void ThreadMain() noexcept {
        for (Task task;;) {
            {
                std::unique_lock lock{ _mutex };
                _signal.wait(lock, [this] { return (_stop || !_toDo.empty()); });

                if (_stop && _toDo.empty())
                    break;

                task = std::move(_toDo.front());
                _toDo.pop();
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
    template <class Func>
    void Schedule(Func&& func) {
        {
            std::scoped_lock lock{ _mutex };

            _toDo.push(std::forward<Func>(func));
        }

        _signal.notify_one();
    }

    WorkerThread() : _thread(&WorkerThread::ThreadMain, this) {}

    ~WorkerThread() {
        {
            std::scoped_lock lock{ _mutex };
            _stop = true;
        }

        _signal.notify_one();
        _thread.join();
    }

    WorkerThread(WorkerThread const&) = delete;
    WorkerThread& operator=(WorkerThread const&) = delete;
    WorkerThread(WorkerThread&&) = delete;
    WorkerThread& operator=(WorkerThread&&) = delete;
};
