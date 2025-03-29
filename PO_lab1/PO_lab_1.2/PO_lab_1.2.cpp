#include <iostream>
#include <chrono>
#include <thread>

using std::chrono::nanoseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

const int rows = 10;
const int columns = 10;


void function(int arr[rows][columns], int row_start, int row_num, int* max, int* min)
{
    *max = arr[0][0];
    *min = arr[0][0];
    for (int i = row_start; i < (row_start + row_num); i++){
        for (int j = 0; j < columns; j++) {
            if (arr[i][j] > *max)
            {
                *max = arr[i][j];
            }
            if (arr[i][j] < *min)
            {
                *min = arr[i][j];
            }
        }
    }
    
    //std::cout << "max = " << *max << std::endl << "min = " << *min;
}

int main()
{
    srand(time(NULL));
    int n = 100;
    int arr[rows][columns];
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            arr[i][j] = rand() % n;
            //std::cout << arr[i][j] << "\t";
        }
        //std::cout << "\n";
    }


    auto payload_begin = high_resolution_clock::now();

    const int thread_num = 2;
    int row_per_thread = rows / thread_num;

    std::thread threads[thread_num];
    int arr_max[thread_num];
    int arr_min[thread_num];

    for (int i = 0; i < thread_num; i++)
    {
        threads[i] = std::thread(function, arr, i * row_per_thread, row_per_thread, &arr_max[i], &arr_min[i]);
    }

    for (int i = 0; i < thread_num; i++)
    {
        threads[i].join();
    }

    int global_max = arr[0][0];
    int global_min = arr[0][0];

    for (int i = 0; i < thread_num; i++)
    {
        if (arr_max[i] > global_max)
        {
            global_max = arr_max[i];
        }
        if (arr_min[i] < global_min)
        {
            global_min = arr_min[i];
        }
    }


    auto payload_end = high_resolution_clock::now();


    auto elapsed = duration_cast<nanoseconds>(payload_end - payload_begin);
    printf("\nPayload time = %.9f seconds \n", (elapsed.count() * 1e-9));
    std::cout << "Payload time = " << elapsed.count() << " nanoseconds" << std::endl;

    std::cout << "global max = " << global_max << " global min = " << global_min << std::endl;

    return 0;
}