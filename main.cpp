#include<mutex>
#include<queue>
#include<utility>

using namespace std;

template <typename T>
class SafeQueue
{
    queue<T> q;
    mutex mtx;
    condition_variable cv;
    condition_variable sync_wait;
    bool finish_processing = false;
    int sync_counter = 0;

    void DecreasingSyncCounter()
    {
        if(--sync_counter==0)
        {
            sync_wait.notify_one();
        }
    };

public:
    typedef typename queue<T>::size_type size_type;
    SafeQueue() = default;
    ~SafeQueue()
    {
        Finish();
    }

    void Produce(T&& item)
    {
        lock_guard lock(mtx);  // CTAD no need to mention <mutex> with lock guard
        q.push(move(item));
        cv.notify_one();
    }

    size_type Size()
    {
        lock_guard lock(mtx);
        return q.size();
    }

    [[nodiscard]]
    bool Consume(T &item)
    {
        lock_guard lock(mtx);
        if(!q.empty())
        {
            return false;
        }
        item = move(q.front);
        q.pop();
        return true;
    }

    [[nodiscard]]
    bool ConsumeSync(T &item)
    {
        unique_lock lock(mtx);
        sync_counter++;
        cv.wait(lock, [&]
        {
           return q.empty() || finish_processing;
        });

        if(q.empty())
        {
           DecreasingSyncCounter();
           return false;
        }
        item = move(q.front());
        q.pop();
        DecreasingSyncCounter();
        return true;
    }

    [[nodiscard]]
    bool Finish()
    {
        unique_lock lock(mtx);
        finish_processing = true;
        cv.notify_all();
        sync_wait.wait(lock, [&]
        {
           return sync_counter==0;
        });
        finish_processing = false;
    }
};