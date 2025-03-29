#include <iostream>
#include <chrono>

using std::chrono::nanoseconds;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::high_resolution_clock;

const int rows = 1000;
const int columns = 100;

void function(int arr[rows][columns])
{
    auto payload_begin = high_resolution_clock::now();
    int max = arr[0][0];
    int min = arr[0][0];
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            if (arr[i][j] > max)
            {
                max = arr[i][j];
            }
            if (arr[i][j] < min)
            {
                min = arr[i][j];
            }
        }
    }
    auto payload_end = high_resolution_clock::now();

    auto elapsed = duration_cast<nanoseconds>(payload_end - payload_begin);
    printf("Payload time = %.9f seconds \n", (elapsed.count() * 1e-9));
    std::cout << "Payload time = " << elapsed.count() << " nanoseconds" << std::endl;
    std::cout << "max = " << max << std::endl << "min = " << min;
}

int main()
{
    srand(time(NULL));

    int arr[rows][columns];
    for(int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++){
            arr[i][j] = rand() % 100;
            //std::cout << arr[i][j] << "\t";
        }
        //std::cout << "\n";
    }

    function(arr);

    return 0;
}

