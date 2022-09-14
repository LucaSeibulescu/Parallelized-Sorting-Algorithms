rm quick_sort_cuda.csv
echo Computation Time, Receive Time, Send Time, Threads, Size, Array Type >> merge_sort_mpi.csv
for array_type in reverse random sorted
do
  for (( size=512; size<=131072; size *= 2 ))
  do
    for (( thread=1; thread<=512; thread *=2 ))
    do

      ./QuickSortCUDA $size $array_type $thread >> quick_sort_cuda.csv
    done
  done
done