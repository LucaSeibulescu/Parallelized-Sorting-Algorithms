#include <stdio.h>
#include <stdlib.h>
#include <time.h>

 void printArr(int arr[], int n) {
    int i;
    for(i = 0; i < n; ++i) {
        printf("%d ", arr[i]);
    }
}

__device__ int dataSize;

__global__ void partition(int *arr, int *arrLow, int *arrHigh, int n) {
    int z = blockIdx.x * blockDim.x + threadIdx.x;
    dataSize = 0;
    __syncthreads();
    if(z < n) {
        int end = arrHigh[z];
        int start = arrLow[z];
        int x = arr[end];
        int i = (start - 1);
        int temp;
        for(int j = start; j <= end - 1; j++) {
            if(arr[j] <= x) {
                i++;
                temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
        temp = arr[i + 1];
        arr[i + 1] = arr[end];
        arr[end] = temp;
        int k = (i + 1);
        if(k - 1 > start) {
            int index = atomicAdd(&dataSize, 1);
            arrLow[index] = start;
            arrHigh[index] = k - 1;  
        }
        if(k + 1 < end) {
            int index = atomicAdd(&dataSize, 1);
            arrLow[index] = k+1;
            arrHigh[index] = end; 
        }
    }
}
 
void quickSortIterative(int arr[], int start, int end, int threads, int numelements) {
    clock_t startTime, endTime;
    int lowStack[end - start + 1], highStack[end - start + 1];
 
    int top = -1, *data, *data_low, *data_high;
 
    lowStack[++top] = start;
    highStack[top] = end;

    startTime = clock();
    cudaMalloc(&data, (end - start + 1)*sizeof(int));
    cudaMemcpy(data, arr, (end - start + 1)*sizeof(int), cudaMemcpyHostToDevice);

    cudaMalloc(&data_low, (end - start + 1)*sizeof(int));
    cudaMemcpy(data_low, lowStack, (end - start + 1)*sizeof(int), cudaMemcpyHostToDevice);

    cudaMalloc(&data_high, (end - start + 1)*sizeof(int));
    cudaMemcpy(data_high, highStack, (end - start + 1)*sizeof(int), cudaMemcpyHostToDevice);
    endTime = clock();
    double communication = ((double) (endTime - startTime)) / CLOCKS_PER_SEC;
    int blocks = numelements/threads;
    int n_i = 1; 
    startTime = clock();
    while(n_i > 0) {
        partition<<<blocks,threads>>>( data, data_low, data_high, n_i);
        int answer;
        cudaMemcpyFromSymbol(&answer, dataSize, sizeof(int), 0, cudaMemcpyDeviceToHost); 
        if(answer < 1024) {
            threads = answer;
        }
        else {
            threads = 1024;
            blocks = answer/threads + (answer%threads==0?0:1);
        }
        n_i = answer;
        cudaMemcpy(arr, data, (end - start + 1)*sizeof(int), cudaMemcpyDeviceToHost);
    }
    endTime = clock();
    double computation = ((double) (endTime - startTime)) / CLOCKS_PER_SEC;
    //printf("Communication Time: %f, Computation Time: %f", communication, computation);
    printf("%f,%f,", computation, communication);
}
 

 
int main(int argc, char* argv[]) {
    int numelements;
    int threads;
    char array_type[8];
    clock_t startTotal, endTotal;
    numelements = atoi(argv[1]);
    startTotal = clock();
    int arr[numelements];
    if(strcmp(argv[2],"reverse") == 0) {
        strcpy(array_type, "reverse");
        for(int i = numelements - 1; i >= 0; i--) {
            arr[i] = i;
        }
    }
    else if(strcmp(argv[2],"sorted") == 0) {
        strcpy(array_type, "sorted");
        for(int i = 0; i < numelements; i++) {
            arr[i] = i;
        }
    }
    else if(strcmp(argv[2],"random") == 0) {
        strcpy(array_type, "random");
        srand(time(NULL));
        for(int i = 0; i < numelements; i++) {
            arr[i] = rand() % numelements;
        }
    }
    threads = atoi(argv[3]);
    int n = sizeof(arr) / sizeof(*arr);
    quickSortIterative(arr, 0, n - 1, threads, numelements);
    endTotal = clock();
    double total_time = ((double) (endTotal - startTotal)) / CLOCKS_PER_SEC;
    //printArr(arr, n);
    //printf("\nTotal Time: %f", total_time);
    printf("%d,%d,%s\n", threads, numelements, array_type);
    return 0;
}