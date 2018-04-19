
Compile 2.C using the following command

mpiicpc -O3 -openmp -o 2 2.C

## 13x SPEEDUP with hybrid implementation on 2 nodes (1 Master, 2 Readers, 4 Mappers, 1 Sender, 8 Reducers and 2 Writers) vs
## hybrid implementation on 1 node (1 Master, 1 Reader, 1 Mapper, 1 Sender, 1 Reducer and 1 Writer)
