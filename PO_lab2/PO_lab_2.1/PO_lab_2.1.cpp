#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

using std::chrono::nanoseconds;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::high_resolution_clock;

const int numElements = 100000000;
const int maxValue = 10000;

void NoParallelization(const std::vector<int>& arr)
{
    int min = maxValue;
    int count =  0;
    for (int i = 0; i < arr.size(); i++) {
        if (min == arr[i]) {
            count++;
        }
        else if (arr[i] < min) {
            min = arr[i];
            count = 1;
        }
    }
    //std::cout << "NoParallelization:" << "\t Min = " << min << "\t Count = " << count << std::endl;
}

int min_forMutex = maxValue;
int count_forMutex = 1;
std::mutex Mutex;
void BlockingByMutex(const std::vector<int>& arr)
{
    for (int i = 0; i < arr.size(); i++) {
        std::lock_guard<std::mutex> mut(Mutex);
        if (min_forMutex == arr[i]) {
            count_forMutex++;
        }
        else if (arr[i] < min_forMutex) {
            min_forMutex = arr[i];
            count_forMutex = 1;
        }
    }
}

std::atomic<int> min_forAtomic(maxValue);
std::atomic<int> count_forAtomic(1);
void AtomicVar(const std::vector<int>& arr)
{
    for (int i = 0; i < arr.size(); i++) {
        int val = arr[i];
        int current_min = min_forAtomic.load();

        bool updated_min = false;

        while (val < current_min) {
            if (min_forAtomic.compare_exchange_weak(current_min, val)) {
                count_forAtomic.store(1);
                updated_min = true;
                break;
            }
        }

        if (!updated_min && val == min_forAtomic.load()) {
            count_forAtomic.fetch_add(1);
        }
    }
}

int main()
{
    srand(time(NULL));

    std::vector<int> arr;
    for (int i = 0; i < numElements; i++) {
        arr.push_back(rand() % maxValue);
        //std::cout << arr[i] << "\t";
    }
    
    // NoParallelization
    auto NoParallelization_begin = high_resolution_clock::now();

    NoParallelization(arr);

    auto NoParallelization_end = high_resolution_clock::now();
    auto NoParallelization_elapsed = duration_cast<nanoseconds>(NoParallelization_end - NoParallelization_begin);
    std::cout << std::endl << "NoParallelization:" << std::endl;
    printf("Time for NoParallelization = %.9f seconds ", (NoParallelization_elapsed.count() * 1e-9));
    std::cout << "= " << NoParallelization_elapsed.count() << " nanoseconds" << std::endl;


    // BlockingByMutex
    auto BlockingByMutex_begin = high_resolution_clock::now();

    const int thread_num = 10;
    int num_per_thread = numElements / thread_num;
    std::thread threads[thread_num];
    
    for (int i = 0; i < thread_num; i++)
    {
        std::vector<int> new_arr(arr.begin() + num_per_thread * i, arr.begin() + num_per_thread * (i + 1));
        threads[i] = std::thread(BlockingByMutex, new_arr);
    }

    for (int i = 0; i < thread_num; i++)
    {
        threads[i].join();
    }

    auto BlockingByMutex_end = high_resolution_clock::now();
    auto BlockingByMutex_elapsed = duration_cast<nanoseconds>(BlockingByMutex_end - BlockingByMutex_begin);
    std::cout << std::endl << "----------------------------------------------" << std::endl << "BlockingByMutex:" << std::endl;
    std::cout << "Number of Threds:" << thread_num << std::endl;
    printf("Time for BlockingByMutex = %.9f seconds ", (BlockingByMutex_elapsed.count() * 1e-9));
    std::cout << "= " << BlockingByMutex_elapsed.count() << " nanoseconds" << std::endl;
    std::cout << "BlockingByMutex:" << "\t Min = " << min_forMutex << "\t Count = " << count_forMutex << std::endl;


    // AtomicVar
    auto AtomicVar_begin = high_resolution_clock::now();

    const int thread_num_atomic = 10;
    int num_per_thread_atomic = numElements / thread_num_atomic;
    std::thread threads_atomic[thread_num_atomic];

    for (int i = 0; i < thread_num_atomic; i++)
    {
        std::vector<int> new_arr(arr.begin() + num_per_thread_atomic * i, arr.begin() + num_per_thread_atomic * (i + 1));
        threads_atomic[i] = std::thread(AtomicVar, new_arr);
    }

    for (int i = 0; i < thread_num_atomic; i++)
    {
        threads_atomic[i].join();
    }

    auto AtomicVar_end = high_resolution_clock::now();
    auto AtomicVar_elapsed = duration_cast<nanoseconds>(AtomicVar_end - AtomicVar_begin);
    std::cout << std::endl << "----------------------------------------------" << std::endl << "AtomicVar:" << std::endl;
    std::cout << "Number of Threds:" << thread_num_atomic << std::endl;
    printf("Time for AtomicVar = %.9f seconds ", (AtomicVar_elapsed.count() * 1e-9));
    std::cout << "= " << AtomicVar_elapsed.count() << " nanoseconds" << std::endl;
    std::cout << "AtomicVar:" << "\t\t Min = " << min_forAtomic << "\t Count = " << count_forAtomic << std::endl;
    std::cout << std::endl << "----------------------------------------------";

    return 0;
}

//void AtomicVar(const std::vector<int>& arr)
//{
//    int Old_Min = maxValue;
//    int Old_Count = 1;
//    int new_min = Old_Min;
//    bool reset = false;
//
//    for (int i = 0; i < arr.size(); i++) {
//        new_min = arr[i];
//
//        do {
//            Old_Min = min_forAtomic.load();
//            if (Old_Min != new_min) {
//                break;
//            }
//            Old_Count = count_forAtomic.load();
//        } while (!count_forAtomic.compare_exchange_weak(Old_Count, Old_Count + 1));
//
//        do {
//            Old_Min = min_forAtomic.load();
//            if (Old_Min <= new_min) {
//                reset = false;
//                break;
//            }
//            reset = true;
//        } while (!min_forAtomic.compare_exchange_weak(Old_Min, new_min));
//
//        if (reset) {
//            count_forAtomic.store(1);
//        }
//        reset = false;
//    }
//}