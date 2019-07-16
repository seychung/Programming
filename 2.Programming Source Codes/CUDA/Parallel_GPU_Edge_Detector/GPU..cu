#include <stdio.h>
#include <cuda.h>
#include <math.h>
#include <cuda_runtime.h>
#include <sys/timeb.h>

__global__ void smooth(float *smooth_fid,float *fid,float *smooth_h,int M)
{
const int smooth_L = 7;
const int h_size = 15;
__shared__ float h[h_size][h_size];
// Fill the filter
for(int i = 0; i < 15; i++) 
  for(int j = 0; j < 15; j++) 
    h[i][j] = smooth_h[i* h_size + j];

const int pixel_address = threadIdx.x + (blockIdx.x *blockDim.x) +(threadIdx.y *M ) + (blockIdx.y * blockDim.y) * M;
float sum = 0;
float value = 0;

for (int i = -smooth_L; i <= smooth_L; i++)	
  for (int j = -smooth_L; j <= smooth_L; j++)	
    {
      // check row 
      if (blockIdx.x == 0 && (threadIdx.x + i) < 0)	// left boarder
        value = 0;
      else if ( blockIdx.x == (gridDim.x - 1) && (threadIdx.x + i) > blockDim.x-1 )	// right boarder
        value = 0;
      else 
      { 
        // check column  
        if (blockIdx.y == 0 && (threadIdx.y + j) < 0)	// top boarder
          value = 0;
        else if ( blockIdx.y == (gridDim.y - 1) && (threadIdx.y + j) > blockDim.y-1 )	// bottom boarder
          value = 0;
        else	// boundary-in case
          value = fid[pixel_address + i + j * M];
      }
    sum += value * h[smooth_L + i][smooth_L + j];
    }
smooth_fid[pixel_address] = sum; 
}

__global__ void edge(float *edge_fid,float *smooth_fid, int M)
{
const int edge_L = 3;
const int h_size = 7;
float edge_hx[h_size][h_size] = {-1, 0, 1, -2, 0, 2, -1,0,1}, edge_hy[h_size][h_size] = {-1, -2, -1, 0, 0, 0, 1,2,1};
const int pixel_address = threadIdx.x + (blockIdx.x *blockDim.x) +(threadIdx.y * M) +(blockIdx.y * blockDim.y) * M;
float sum1 = 0;
float sum2 = 0;
float value1 = 0;
float value2 = 0;

for (int i = -edge_L; i <= edge_L; i++)	
  for (int j = -edge_L; j <= edge_L; j++)	
    {
      // check row 
      if (blockIdx.x == 0 && (threadIdx.x + i) < 0)	// left boarder
        value1 = 0;
      else if ( blockIdx.x == (gridDim.x - 1) && (threadIdx.x + i) > blockDim.x-1 )	// right boarder
        value1 = 0;
      else 
      { 
        // check column 
        if (blockIdx.y == 0 && (threadIdx.y + j) < 0)	// top boarder
          value1 = 0;
        else if ( blockIdx.y == (gridDim.y - 1) && (threadIdx.y + j) > blockDim.y-1 )	// bottom boarder
          value1 = 0;
        else	// boundary-in case
          value1 = smooth_fid[pixel_address + i + j * M];
      } 
    sum1 += value1 * edge_hx[edge_L + i][edge_L + j];
    }
for (int i = -edge_L; i <= edge_L; i++)	
  for (int j = -edge_L; j <= edge_L; j++)	
    {
      // check row 
      if (blockIdx.x == 0 && (threadIdx.x + i) < 0)	// left boarder
        value2 = 0;
      else if ( blockIdx.x == (gridDim.x - 1) && (threadIdx.x + i) > blockDim.x-1 )	// right boarder
        value2 = 0;
      else 
      { 
        // check col 
        if (blockIdx.y == 0 && (threadIdx.y + j) < 0)	// top boarder
          value2 = 0;
        else if ( blockIdx.y == (gridDim.y - 1) && (threadIdx.y + j) > blockDim.y-1 )	// bottom boarder
          value2 = 0;
        else	// boundary-in case
          value2 = smooth_fid[pixel_address + i + j * M];
      } 
    sum2 += value2 * edge_hy[edge_L + i][edge_L + j];
    }    
    
edge_fid[pixel_address] = sqrt(((sum1*sum1) + (sum2*sum2))); 
}
main() {
/* Time Declaration */
struct timeb tmb;
struct timeb tma;
/* File Read Declaration*/
FILE *fr = fopen("Leaves_noise.bin", "r");
FILE *fw1 = fopen("1_Low_Pass_Filtered.bin", "w");
FILE *fw2 = fopen("2_Edge_Detected.bin", "w");
/* Declaration */
cudaError er1,er2,er3,er4,er5;
const int smooth_W = 15;
const int edge_W = 7;
const int smooth_L = 7;
const int edge_L = 3;
const int segma = 1.5;
const int M = 2048;
const int B = 2;
const int image_size = M * M;
  /* GPU */
float* fid;  //float image for gpu
float* smooth_fid;  //Smoothed float image for gpu
float* edge_fid;  //Edged float image for gpu
  /* CPU */
float* fih;  //float image for cpu
float* smooth_fih;  //Smoothed float image for cpu
float* edge_fih;  //Edged float image for cpu
unsigned char* cih;  //unsighed char image for cpu
  /* Filter */
float* smooth_filter_d; //gpu smooth filter
float* smooth_filter_h; //cpu smooth filter

/* cuda memory allocation global*/
er1 = cudaMalloc((void**) &fid, image_size * sizeof(float));
er2 = cudaMalloc((void**) &smooth_fid, image_size * sizeof(float));
er3 = cudaMalloc((void**) &edge_fid, image_size * sizeof(float));
er4 = cudaMalloc((void**) &smooth_filter_d, smooth_W * smooth_W * sizeof(float));
if(er1 != 0 || er2 != 0 || er3 != 0 || er4 != 0) {
  printf("Cuda Success Failed\n");
  return 0;
}

/* CPU  Matrices Initilization*/
fih = (float*)malloc(image_size * sizeof(float));
smooth_fih = (float*)malloc(image_size * sizeof(float));
edge_fih = (float*)malloc(image_size * sizeof(float));
smooth_filter_h = (float*)malloc(smooth_W * smooth_W * sizeof(float));
cih = (unsigned char*)malloc(image_size * sizeof(unsigned char));
for (int y=0; y<M; y++) {
	for (int x=0; x<M; x++){
     if(y < smooth_W && x < smooth_W) {
       smooth_filter_h[y*smooth_W + x] = (1/(2*M_PI*pow(segma,2))) * exp(-((pow((y-smooth_L),2) + pow((x-smooth_L),2)) / (2*pow(segma,2))));
     }
     smooth_fih[y*M + x] = 0;
     edge_fih[y*M + x] = 0;
     fih[y*M + x] = 0;
     cih[y*M + x] = 0; 
	}
}

/* Orinigal Image Read*/
fread(cih, 1, M*M, fr);
fclose(fr);

/* Upcast Image to Float */
for (int y=0; y<M; y++) {
	for (int x=0; x<M; x++){
     fih[y*M + x] = cih[y*M + x]; 
	}
}
/* cuda setup dims */
dim3 dimBlock(B, B);
dim3 dimGrid(M/B, M/B);
/* Start Time */
ftime(&tmb);
/* cuda memory copy*/
cudaMemcpy(fid, fih, image_size * sizeof(float) , cudaMemcpyHostToDevice);
cudaMemcpy(smooth_fid, smooth_fih, image_size * sizeof(float) , cudaMemcpyHostToDevice);
cudaMemcpy(edge_fid, edge_fih, image_size * sizeof(float) , cudaMemcpyHostToDevice);
cudaMemcpy(smooth_filter_d, smooth_filter_h, smooth_W * smooth_W * sizeof(float) , cudaMemcpyHostToDevice);
/* launch cuda matrix multiplication*/
smooth<<<dimGrid, dimBlock>>>(smooth_fid, fid, smooth_filter_d,M);
edge<<<dimGrid, dimBlock>>>(edge_fid, smooth_fid,M);
/* CPU gets result from GPU*/
cudaMemcpy(smooth_fih, smooth_fid, image_size * sizeof(float), cudaMemcpyDeviceToHost);
cudaMemcpy(edge_fih, edge_fid, image_size * sizeof(float), cudaMemcpyDeviceToHost);
/* End Time */
ftime(&tma); 
printf("B = %d\n",B);
if((tma.millitm - tmb.millitm) < 0) {
  printf("Latency: %ld (seconds), ", tma.time - 1 - tmb.time);
  printf("%d (mlliseconds)\n", (tma.millitm + 1) - tmb.millitm);
}
else {
  printf("Latency: %ld (seconds), ", tma.time-tmb.time);
  printf("%d (mlliseconds)\n", tma.millitm - tmb.millitm);
}
/* Write Image Files */
  /* Downcast to unsigned char*/
for (int y=0; y<M; y++) {
	for (int x=0; x<M; x++){
    cih[y*M + x] = smooth_fih[y*M + x];
	}
}
  /* Smooth Image Write*/
if(fw1 != NULL) {
	fwrite(cih, 1, M*M, fw1);
}
else{
	printf("fw1: NULL\n");
}
fclose(fw1);

  /* Downcast to unsigned char*/
for (int y=0; y<M; y++) {
	for (int x=0; x<M; x++){        
    if((char)edge_fih[y*M + x] > 58) {
      cih[y*M + x] = 255;
    }
    else {
      cih[y*M + x] = 1;
    }
  }
}
  /* Edge Image Write*/
if(fw2!= NULL) {
	fwrite(cih, 1, M*M, fw2);
}
else{
	printf("fw2: NULL\n");
}
fclose(fw2);

/* Free allocations*/
cudaFree(fid); cudaFree(smooth_fid); cudaFree(edge_fid); cudaFree(smooth_filter_d);
free(fih); free(cih); free(smooth_fih); free(edge_fih); free(smooth_filter_h);
return 0;
}
