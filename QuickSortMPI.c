#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
 
void swap(int* arr, int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}
 
void quicksort(int* arr, int start, int end) {
    int pivot, index;
 
    if(end <= 1) {
        return;
    }

    pivot = arr[start + end / 2];
    swap(arr, start, start + end / 2);
 
    index = start;
 
    for(int i = start + 1; i < start + end; i++) {
        if(arr[i] < pivot) {
            index++;
            swap(arr, i, index);
        }
    }
 
    swap(arr, start, index);
 
    quicksort(arr, start, index - start);
    quicksort(arr, index + 1, start + end - index - 1);
}
 
int* merge(int* arr1, int n1, int* arr2, int n2) {
    int* result = (int*)malloc((n1 + n2) * sizeof(int));
    int i = 0;
    int j = 0;
    int k;
 
    for(k = 0; k < n1 + n2; k++) {
        if(i >= n1) {
            result[k] = arr2[j];
            j++;
        }
        else if(j >= n2) {
            result[k] = arr1[i];
            i++;
        }
 
        else if(arr1[i] < arr2[j]) {
            result[k] = arr1[i];
            i++;
        }
 
        else {
            result[k] = arr2[j];
            j++;
        }
    }
    return result;
}
 
int main(int argc, char* argv[]) {
    int numelements;
    int* data = NULL;
    int chunk_size, own_chunk_size;
    int* chunk;
    double total_time_start;
    double total_time_end;
    double total_time;
    double communication_time_start;
    double communication_time_end;
    double communication_send_time_start;
    double communication_send_time_end;
    double communication_recv_time_start;
    double communication_recv_time_end;
    double communication_time;
    double communication_send_time;
    double communication_recv_time = 0;
    double computation_time_start;
    double computation_time_end;
    double computation_time;
    double computation_time_final;
    double communication_start;
    double communication_total;
    double communication_final;
    char array_type[8];
    MPI_Status status;
 
    int numtasks, taskid;

    numelements = atoi(argv[1]);
    int rc = MPI_Init(&argc, &argv);
 
    if(rc != MPI_SUCCESS) {
        printf("Error in creating MPI program.\n Terminating......\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
 
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    
    if(taskid == 0) {
        chunk_size = (numelements % numtasks == 0) ? (numelements / numtasks) : (numelements / (numtasks - 1));
    
        data = (int *)malloc(numtasks * chunk_size * sizeof(int));
        if(strcmp(argv[2],"reverse") == 0) {
            strcpy(array_type, "reverse");
            for(int i = numelements - 1; i >= 0; i--) {
                data[i] = i;
            }
        }
        else if(strcmp(argv[2],"sorted") == 0) {
            strcpy(array_type, "sorted");
            for(int i = 0; i < numelements; i++) {
                data[i] = i;
            }
        }
        else if(strcmp(argv[2],"random") == 0) {
            strcpy(array_type, "random");
            srand(time(NULL));
            for(int i = 0; i < numelements; i++) {
                int r = rand() % numelements;
                data[i] = r;
            }
        }

        for(int i = numelements; i < numtasks * chunk_size; i++) {
            data[i] = 0;
        }

    }
 
    MPI_Barrier(MPI_COMM_WORLD);
    if(taskid == 0) {
        total_time_start = MPI_Wtime();
    }
 
    MPI_Bcast(&numelements, 1, MPI_INT, 0, MPI_COMM_WORLD);
 
    chunk_size = (numelements % numtasks == 0) ? (numelements / numtasks) : (numelements / (numtasks - 1));
 
    chunk = (int *)malloc(chunk_size * sizeof(int));
 
    MPI_Scatter(data, chunk_size, MPI_INT, chunk, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);
    free(data);
    data = NULL;
 
    own_chunk_size = (numelements >= chunk_size*(taskid + 1)) ? chunk_size : (numelements - chunk_size*taskid);
 
    computation_time_start = MPI_Wtime();
    quicksort(chunk, 0, own_chunk_size);
    computation_time_end = MPI_Wtime();
    computation_time = computation_time_end - computation_time_start;
    if(taskid == 0) {
        communication_time_start = MPI_Wtime();
        communication_send_time_start = MPI_Wtime();
    }
    for(int step = 1; step < numtasks; step = 2 * step) {
        if(taskid % (2 * step) != 0) {
            communication_start = MPI_Wtime();
            MPI_Send(chunk, own_chunk_size, MPI_INT, taskid - step, 0, MPI_COMM_WORLD);
            communication_total = (MPI_Wtime() - communication_start);
            break;
        }
        if(taskid == 0) {
            communication_send_time_end = MPI_Wtime();
            communication_send_time = communication_send_time_end - communication_send_time_start;
        }
        
        if(taskid + step < numtasks) {
            int received_chunk_size = (numelements >= chunk_size * (taskid + 2 * step)) ? (chunk_size * step) : (numelements - chunk_size * (taskid + step));
            int* chunk_received;
            chunk_received = (int*)malloc(received_chunk_size * sizeof(int));
            if(taskid == 0) {
                communication_recv_time_start = MPI_Wtime();
            }
            communication_start = MPI_Wtime();
            MPI_Recv(chunk_received, received_chunk_size, MPI_INT, taskid + step, 0, MPI_COMM_WORLD, &status);
            communication_total += (MPI_Wtime() - communication_start);
            if(taskid == 0) {
                communication_recv_time_end = MPI_Wtime();
                communication_recv_time += communication_recv_time_end - communication_recv_time_start;
            }

            computation_time_start = MPI_Wtime();
            data = merge(chunk, own_chunk_size, chunk_received, received_chunk_size);
            computation_time_end = MPI_Wtime();
            computation_time += computation_time_end - computation_time_start;
 
            free(chunk);
            free(chunk_received);
            chunk = data;
            own_chunk_size = own_chunk_size + received_chunk_size;
        }
        if(taskid == 0) {
            communication_time_end = MPI_Wtime();
        }
    }

    MPI_Reduce(&computation_time, &computation_time_final, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&communication_total, &communication_final, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
 
    if(taskid == 0) {
        total_time_end = MPI_Wtime();
        total_time = total_time_end - total_time_start;
        communication_time = communication_time_end - communication_time_start;
        /*printf("Sorted array is: \n");
 
        for(int i = 0; i < numelements; i++) {
            printf("%d  ", chunk[i]);
        }
 
        printf("\n\nQuicksort %d ints on %d process\n", numelements, numtasks);
        printf("Total Time: %f, Communication Time: %f, Communication Send Time: %f, Communication Recieve Time: %f Computation Time: %f\n", total_time, communication_time, communication_send_time, communication_recv_time, computation_time);*/
        printf("%f,%f,%d,%d,%s\n", computation_time_final, communication_final, numtasks, numelements, array_type);
    }
 
    MPI_Finalize();
    return 0;
}
