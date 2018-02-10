#include <cuda.h>
#include <math.h>

#include <stdio.h>

__global__ void calcDominationMap(float *pFitnessArray, int numFitnessMeasures, int numGenomes, short *pDominationMap)
{
	int genomeIndex1 = blockIdx.x * blockDim.x + threadIdx.x;
	int genomeIndex2 = blockIdx.y * blockDim.y + threadIdx.y;

	if (genomeIndex1 >= numGenomes || genomeIndex2 >= numGenomes)
		return;
	
	int fitnessOffset1 = genomeIndex1*numFitnessMeasures;
	int fitnessOffset2 = genomeIndex2*numFitnessMeasures;

	int betterOrEqualCount = 0;
	int equalCount = 0;

	for (int i = 0 ; i < numFitnessMeasures ; i++)
	{
		float val1 = pFitnessArray[fitnessOffset1+i];
		float val2 = pFitnessArray[fitnessOffset2+i];

		if (val1 == val2)
		{
			equalCount++;
			betterOrEqualCount++;
		}
		else if (val1 < val2)
			betterOrEqualCount++;
	}

	int mapIndex = genomeIndex1*numGenomes+genomeIndex2;

	if (betterOrEqualCount == numFitnessMeasures && equalCount < numFitnessMeasures)
	{
		// 1 dominates 2
		pDominationMap[mapIndex] = 1;
	}
	else
		pDominationMap[mapIndex] = 0;
}

__global__ void calcDominatedByCount(int numGenomes, short *pDominationMap, int *pCount)
{
	int genomeIndex = blockIdx.x * blockDim.x + threadIdx.x;

	if (genomeIndex >= numGenomes)
		return;
	
	int offset = genomeIndex;
	int count = 0;

	for (int i = 0 ; i < numGenomes ; i++, offset += numGenomes)
	{
		if (pDominationMap[offset])
		{
			count++;
		}
	} 
	pCount[genomeIndex] = count; 
}

__global__ void calcDominatesList(int numGenomes, short *pDominationMap, int *pCount) // domination map will be reused for this
{
	int genomeIndex = blockIdx.x * blockDim.x + threadIdx.x;

	if (genomeIndex >= numGenomes)
		return;
	
	int offset = genomeIndex*numGenomes;
	int storeOffset = offset;
	int count = 0;

	for (int i = 0 ; i < numGenomes ; i++, offset++)
	{
		if (pDominationMap[offset])
		{
			pDominationMap[storeOffset] = i;
			storeOffset++;
			count++;
		}
	}

	pCount[genomeIndex] = count;
}

__global__ void parallelCopy(short *pDominationMap, int dstOffset, int srcOffset, int num)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;

	if (i >= num)
		return;
	
	pDominationMap[dstOffset+i] = pDominationMap[srcOffset+i];
}

bool allocDeviceMemory(int numGenomes, int numFitnessMeasures, float **pDeviceFitnessArray, short **pDeviceDominationMap,
                       int **pDeviceDominatedByCount, int **pDeviceDominatesCount, short **pHostDominationMap)
{
	*pDeviceFitnessArray = 0;
	*pDeviceDominationMap = 0;
	*pDeviceDominatedByCount = 0;
	*pDeviceDominatesCount = 0;
	*pHostDominationMap = 0;

	if (cudaMalloc((void **)pDeviceFitnessArray, numGenomes*numFitnessMeasures*sizeof(float)) != cudaSuccess)
	{
		*pDeviceFitnessArray = 0;
		return false;
	}

	if (cudaMalloc((void **)pDeviceDominationMap, numGenomes*numGenomes*sizeof(short)) != cudaSuccess)
	{
		cudaFree(*pDeviceFitnessArray);
		*pDeviceFitnessArray = 0;
		*pDeviceDominationMap = 0;
		return false;
	}

	if (cudaMalloc((void **)pDeviceDominatedByCount, numGenomes*sizeof(int)) != cudaSuccess)
	{
		cudaFree(pDeviceFitnessArray);
		cudaFree(pDeviceDominationMap);
		*pDeviceFitnessArray = 0;
		*pDeviceDominationMap = 0;
		*pDeviceDominatedByCount = 0;
		return false;
	}

	if (cudaMalloc((void **)pDeviceDominatesCount, numGenomes*sizeof(int)) != cudaSuccess)
	{
		cudaFree(pDeviceFitnessArray);
		cudaFree(pDeviceDominationMap);
		cudaFree(pDeviceDominatedByCount);
		*pDeviceFitnessArray = 0;
		*pDeviceDominationMap = 0;
		*pDeviceDominatedByCount = 0;
		*pDeviceDominatesCount = 0;
		return false;
	}

	if (cudaHostAlloc((void **)pHostDominationMap, numGenomes*numGenomes*sizeof(short), 0) != cudaSuccess)
	{
		cudaFree(pDeviceFitnessArray);
		cudaFree(pDeviceDominationMap);
		cudaFree(pDeviceDominatedByCount);
		cudaFree(pDeviceDominatesCount);
		*pDeviceFitnessArray = 0;
		*pDeviceDominationMap = 0;
		*pDeviceDominatedByCount = 0;
		*pDeviceDominatesCount = 0;
		*pHostDominationMap = 0;
		return false;
	}

	return true;
}

void freeDeviceMemory(float *pDeviceFitnessArray, short *pDeviceDominationMap,
                      int *pDeviceDominatedByCount, int *pDeviceDominatesCount, short *pHostDominationMap)
{
	cudaFree(pDeviceFitnessArray);
	cudaFree(pDeviceDominationMap);
	cudaFree(pDeviceDominatedByCount);
	cudaFree(pDeviceDominatesCount);
	cudaFreeHost(pHostDominationMap);
}
	
void calcDominationInfo(int numGenomes, int numFitnessMeasures, float *pHostFitnessArray, short *pHostDominationMap,
			int *pHostDominatedByCount, int *pHostDominatesCount,
                        float *pDeviceFitnessArray, short *pDeviceDominationMap, 
			int *pDeviceDominatedByCount, int *pDeviceDominatesCount,
			int blockSize1, int blockSize2, int *pOffsets)
{
	cudaMemcpy(pDeviceFitnessArray, pHostFitnessArray, numGenomes*numFitnessMeasures*sizeof(float), cudaMemcpyHostToDevice);

	int blockSize = blockSize1;
	int numBlocks = numGenomes/blockSize;

	if ((numGenomes % blockSize) != 0)
		numBlocks++;

	dim3 threads(blockSize,blockSize);
	dim3 blocks(numBlocks, numBlocks);

	calcDominationMap<<<blocks, threads>>>(pDeviceFitnessArray, numFitnessMeasures, numGenomes, pDeviceDominationMap);

	blockSize = blockSize2;
	numBlocks = numGenomes/blockSize;

	if ((numGenomes % blockSize) != 0)
		numBlocks++;

	calcDominatedByCount<<<numBlocks, blockSize>>>(numGenomes, pDeviceDominationMap, pDeviceDominatedByCount);
	calcDominatesList<<<numBlocks, blockSize>>>(numGenomes, pDeviceDominationMap, pDeviceDominatesCount);

	cudaMemcpy(pHostDominatedByCount, pDeviceDominatedByCount, numGenomes*sizeof(int), cudaMemcpyDeviceToHost);
	cudaMemcpy(pHostDominatesCount, pDeviceDominatesCount, numGenomes*sizeof(int), cudaMemcpyDeviceToHost);
#if 0
	int destOffset = pHostDominatesCount[0];
	int roomLeft = numGenomes-pHostDominatesCount[0];
	int totalCount = pHostDominatesCount[0];
	
	pOffsets[0] = 0;

	for (int i = 1 ; i < numGenomes ; i++)
	{
		pOffsets[i] = totalCount;

		if (roomLeft == 0) // nothing to do then, already next to each other
		{
			destOffset += numGenomes;

			printf("HERE\n");
		}
		else
		{
			int srcOffset = i*numGenomes;
			int totalToCopy = pHostDominatesCount[i];

			while (totalToCopy > 0)
			{
				int numCopied = totalToCopy;

				if (numCopied > roomLeft)
					numCopied = roomLeft;

				// blockSize is still ok
				numBlocks = numCopied/blockSize;
				if ((numCopied % blockSize) != 0)
					numBlocks++;
				
				parallelCopy<<<numBlocks, blockSize>>>(pDeviceDominationMap, destOffset, srcOffset, numCopied);

				destOffset += numCopied;
				srcOffset += numCopied;
				totalToCopy -= numCopied;
			}
		}

		roomLeft += (numGenomes-pHostDominatesCount[i]);
		totalCount += pHostDominatesCount[i];
	}

	printf("%d vs %d\n",totalCount, numGenomes*numGenomes);
	cudaMemcpy(pHostDominationMap, pDeviceDominationMap, totalCount*sizeof(short), cudaMemcpyDeviceToHost);
#else
	for (int i = 0 ; i < numGenomes ; i++)
		pOffsets[i] = i*numGenomes;

	cudaMemcpy(pHostDominationMap, pDeviceDominationMap, numGenomes*numGenomes*sizeof(short), cudaMemcpyDeviceToHost);
#endif

}

bool checkCuda(int *pBlockSize1, int *pBlockSize2)
{
	int numDevices = 0;

	cudaGetDeviceCount(&numDevices);

	if (numDevices == 0)
		return false;

	// TODO: For now, we only check device 0

	cudaDeviceProp deviceProp;
	cudaGetDeviceProperties(&deviceProp, 0);

	if (deviceProp.major == 9999 && deviceProp.minor == 9999)
		return false;

	int maxThreads = deviceProp.maxThreadsPerBlock;

	*pBlockSize2 = maxThreads;

	double r = sqrt((double)maxThreads);

	*pBlockSize1 = (int)r;

	return true;
}

