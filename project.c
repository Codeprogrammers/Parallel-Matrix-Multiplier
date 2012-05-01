#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "sys/time.h"

int main(int argc, char* argv[])
{
	int n = atoi(argv[1]);
	MPI_Init(&argc, &argv);
	int namelen;
	int rank, size, source;
        char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Status status;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Get_processor_name(processor_name,&namelen);
	int *matrixB;
	matrixB = (int*)calloc(n*n, sizeof(int));
	int x;
	for(x = 0; x < n * n; x++)
	{
		matrixB[x] = 1;
	}
	int rows = atoi(argv[2]);
	int bsize = n * rows;
	if(rank == 0)
	{
		int *matrixA;
		int *matrixC;
		int i, j;
		int rem, quo;
		int last = n % rows;
		int num = size - 1;
		int done;
		int sendSize;
		if(n / rows < size - 1)
		{
			num = n / rows;
			if(n % rows != 0)
			{
				num++;
			}
		}
		int time;
		struct timeval tpstart, tpend;
		MPI_Request *sreq = (MPI_Request*)calloc(size, sizeof(MPI_Request));
		MPI_Request *rreq = (MPI_Request*)calloc(size, sizeof(MPI_Request));
		for(i = 0; i < size; i++)
		{
			MPI_Request temp1, temp2;
			sreq[i] = temp1;
			rreq[i] = temp2;
		}
		//CREATE MATRICES
		matrixA = (int*)calloc(n*n, sizeof(int));

		matrixC = (int*)calloc(n*n, sizeof(int));
		//FILL MATRICES
		for(i = 0; i < n; i++)
		{
			for(j = 0; j < n; j++)
			{
				matrixA[i*n + j] = 1;
				matrixC[i*n + j] = 0;
			}
		}
		//SEND MATRICES TO SLAVE PROCESSES
		gettimeofday(&tpstart, NULL);
		for(i = 1; i <= num; i++)
		{
			sendSize = bsize;
			if(bsize * i > n * n)
			{
				sendSize = last * n;
			}
			MPI_Isend(&sendSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &sreq[i]);
			MPI_Wait(&sreq[i], &status);
			MPI_Isend(&matrixA[bsize * (i - 1)], sendSize, MPI_INT, i, 0, MPI_COMM_WORLD, &sreq[i]);
			MPI_Irecv(&matrixC[bsize * (i - 1)], sendSize, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &rreq[i]);
		}
		/*for(i = 0; i < n; i++)
		{
			for(j = 0; j < n; j++)
			{
				printf("%i  ", matrixA[i * n + j]);
			}
			printf("\n\n");
		}*/
		int next = bsize * (num - 1);
		while(next < n * n)
		{
			for(i = 1; i < size; i++)
			{
				MPI_Test(&rreq[i], &done, &status);
				if(done)
				{
					sendSize = bsize;
					if(next + bsize > n * n)
					{
						sendSize = last * n;
					}
					MPI_Isend(&sendSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &sreq[i]);
					MPI_Wait(&sreq[i], &status);
					MPI_Isend(&matrixA[next], sendSize, MPI_INT, i, 0, MPI_COMM_WORLD, &sreq[i]);
					MPI_Irecv(&matrixC[next], sendSize, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &rreq[i]);
					next += bsize;
					if(next >= n * n)
					{
						break;
					}
				}
			}
		}
		for(i = 1; i <= num; i++)
		{
			MPI_Wait(&rreq[i], &status);
			sendSize = 0;
			MPI_Isend(&sendSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &sreq[i]);
		}
		gettimeofday(&tpend, NULL);
		time = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + (tpend.tv_usec - tpstart.tv_usec);
		printf("%i\n", time);
		if(n <= 16)
		{
			printf("MatrixA:\n");
			for(i = 0; i < n; i++)
			{
				for(j = 0; j < n; j++)
				{
					printf("%i  ", matrixA[i*n + j]);
				}
				printf("\n");
			}
			printf("\n\n");
			printf("MatrixB:\n");
			for(i = 0; i < n; i++)
			{
				for(j = 0; j < n; j++)
				{
					printf("%i  ", matrixB[i*n + j]);
				}
				printf("\n");
			}
			printf("\n\n");
			printf("MatrixC:\n");
			for(i = 0; i < n; i++)
			{
				for(j = 0; j < n; j++)
				{
					printf("%i  ", matrixC[i*n + j]);
				}
				printf("\n");
			}
			printf("\n\n");
		}
	}
	else
	{
		int recvCount;
		int *chunkA;
		int *results;
		int a,b,c;
		int sum;
		MPI_Request sreq, rreq;
		chunkA = (int*)calloc(rows * n, sizeof(int));
		results = (int*)calloc(rows * n, sizeof(int));
		while(1){
		MPI_Irecv(&recvCount, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &rreq);
		MPI_Wait(&rreq, &status);
		if(recvCount == 0)
		{
			break;
		}
		MPI_Irecv(&chunkA[0], recvCount, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &rreq);
		MPI_Wait(&rreq, &status);
		for(a = 0; a < rows; a++)
		{
			for(b = 0; b < n; b++)
			{
				sum = 0;
				for(c = 0; c < n; c++)
				{
					sum = sum + (chunkA[a * n + c] * matrixB[c * n + b]);
				}
				results[a * n + b] = sum;
			}
		}
		MPI_Isend(&results[0], recvCount, MPI_INT, 0, rank, MPI_COMM_WORLD, &sreq);
		}
	}
	MPI_Finalize();
	return 0;
}
