#include <stdio.h>
#include <math.h>
#include <float.h>
#include "mpi.h"
#define smooth_W 15
#define edge_W 3
#define M 2048
#define smooth_L 7
#define edge_L 3
#define segma 1.5
#define edge_point 100
main() {	
/*****************************************/
	int my_rank,p;
	MPI_Status status;
	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);
/*****************************************/	
	if(my_rank == 0){
		/* Init*/
		int i,j,x,y,l,k,count;
		FILE *fr = fopen("Leaves_noise.bin", "r");
		FILE *fw1 = fopen("1_Low_Pass_Filtered.bin", "w");
		FILE *fw2 = fopen("2_Edge_Detected.bin", "w");
		float *f, *result, *gx, *gy;
		double start1, start2, end1, end2, latency, overhead_total = 0,overhead_start, overhead_end;
		unsigned char* cf;
		float edge_hx[edge_W][edge_W] = {-1, 0, 1, -2, 0, 2, -1,0,1}, edge_hy[edge_W][edge_W] = {-1, -2, -1, 0, 0, 0, 1,2,1};
		float smooth_h[smooth_W][smooth_W];
		f = (float*)malloc(M*M*sizeof(float));
		result = (float*)malloc((M/p) * M * sizeof(float));
		gx = (float*)malloc((M/p) * M * sizeof(float));
		gy = (float*)malloc((M/p) * M * sizeof(float));
		cf = (unsigned char*)malloc(M*M*sizeof(unsigned char));
		for (y=0; y<M; y++) {
			for (x=0; x<M; x++){
				f[y*M + x] = 0;
				cf[y*M + x] = 0;
				if(y < M/p) {
					result[y*M+ x] = 0;
					gx[y*M+ x] = 0;
					gy[y*M+ x] = 0;
				}
				if(y < smooth_W && x < smooth_W) {
					smooth_h[y][x] = (1/(2*M_PI*pow(segma,2))) * exp(-((pow((y-smooth_L),2) + pow((x-smooth_L),2)) / (2*pow(segma,2))));
				}
			}
		}
		/* Image Read */ 
		fread(cf, 1, M*M, fr);
		fclose(fr);
		/* Upcast to float */
		for (y=0; y<M; y++) {
			for (x=0; x<M; x++){
				f[y*M + x] = cf[y*M + x];
				cf[y*M + x] = 0;
			}
		}
		start1 = MPI_Wtime();
		overhead_start = MPI_Wtime();
		/* MPI Init Image Send M*(M/p+2*smooth_L) */
		int offset = 0;
		if(p > 1) {
			for(i = 1; i < p; i++) {
				offset = i * (M/p) - smooth_L - edge_L;
				//offset = 0;
				if(i == p-1) {
					MPI_Send(&f[offset*M], (M/p+smooth_L+edge_L)*M, MPI_FLOAT, i, i , MPI_COMM_WORLD);
				}
				else {
					MPI_Send(&f[offset*M], (M/p+2*smooth_L+2*edge_L)*M, MPI_FLOAT, i, i , MPI_COMM_WORLD);
				}
			}
		}
		overhead_end = MPI_Wtime();
		overhead_total = overhead_end - overhead_start;
		/* Smooth Operations */   
		for(i = 0; i < M/p; i++) {		
			for(j = 0; j < M; j++) {
				for(k = -smooth_L; k <= smooth_L; k++) {
					for(l = -smooth_L; l <= smooth_L; l++) {
					   if(!(j - l < 0 || i - k < 0) && (j - l < M && i - k < M))  {								
							result[i*M+j] += smooth_h[k+smooth_L][l+smooth_L] * f[(i-k)*M + (j-l)];				
					   }
					}
				}
			}		
		} 
		memcpy(f,result, (M/p) * M * sizeof(float));
		/* MPI Smooth Receive*/
		overhead_start = MPI_Wtime();
		if(p > 1) {
			for(i = 1; i < p; i++) {
				offset = i * (M/p);
				MPI_Recv(&f[offset*M], (M/p)*M, MPI_FLOAT, i, i, MPI_COMM_WORLD, &status);		
			}
		}		
		end1 = MPI_Wtime();
		overhead_end = MPI_Wtime();
		overhead_total += overhead_end - overhead_start;
		/* Downcast Smooth Image to unsigned char */
		for (y=0; y<M; y++) {
			for (x=0; x<M; x++){
				cf[y*M + x] = f[y*M + x];
			}
		}		
		
		/* Smooth Image Bin File Write */
		if(fw1 != NULL) {
			fwrite(cf, 1, M*M, fw1);
		}
		else{
			printf("fw1: NULL\n");
		}
		fclose(fw1);
		start2 = MPI_Wtime();
		/* Edge Operation ! */
			/* Differentiate x-coordinate !*/
		for(i = 0; i < M/p; i++) {		
			for(j = 0; j < M; j++) {
				for(k = -edge_L; k <= edge_L; k++) {
					for(l = -edge_L; l <= edge_L; l++) {
					   if(!(j - l < 0 || i - k < 0) && (j - l < M && i - k < M) && ((k+edge_L) < 3 && (l+edge_L < 3)))  {								
							gx[i*M+j] += edge_hx[k+edge_L][l+edge_L] * f[(i-k)*M + (j-l)];					
					   }
					}
				}
			}		
		}	
			/* Differentiate y-coordinate !*/
		for(i = 0; i < M/p; i++) {		
			for(j = 0; j < M; j++) {
				for(k = -edge_L; k <= edge_L; k++) {
					for(l = -edge_L; l <= edge_L; l++) {
					   if(!(j - l < 0 || i - k < 0) && (j - l < M && i - k < M) && ((k+edge_L) < 3 && (l+edge_L < 3)))  {								
							gy[i*M+j] += edge_hy[k+edge_L][l+edge_L] * f[(i-k)*M + (j-l)];;				
					   }
					}
				}
			}		
		}	
			/* Combine Differentiations !*/
		count = 0;
		for (y=0; y<M/p; y++) {
			for (x=0; x<M; x++){
				f[y*M+x] = sqrt(pow(gx[y*M+x],2) + pow(gy[y*M+x],2));
			}
		}

		/* MPI Edge Receive*/
		overhead_start = MPI_Wtime();
		if(p > 1) {
			for(i = 1; i < p; i++) {
				offset = i * (M/p);
				MPI_Recv(&f[offset*M], (M/p)*M, MPI_FLOAT, i, i, MPI_COMM_WORLD, &status);
			}
		}
		overhead_end = MPI_Wtime();
		end2 = MPI_Wtime();
		overhead_total += overhead_end - overhead_start;
		latency = (end2-start2) + (end1-start1);
		printf("Number of threads = %d\n",p);
		printf("Latency = %e\n",latency);
		printf("Total Overhead = %e\n",overhead_total);
		/* Downcast to unsigned char*/
		for (y=0; y<M; y++) {
			for (x=0; x<M; x++){
				if((char)f[y*M + x] > 58) {
					cf[y*M + x] = 255;
				}
				else {
					cf[y*M + x] = 1;
				}
				//cf[y*M + x] = f[y*M + x];
			}
		}
		// /* Edge Image Bin File Write */
		if(fw2!= NULL) {
			fwrite(cf, 1, M*M, fw2);
		}
		else{
			printf("fw2: NULL\n");
		}
		fclose(fw2);		
	}
	else {
		/* Init */
		int i,j,x,y,l,k,count;
		float *pf, *result, *gx, *gy;
		int rowd = 0;
		if(my_rank == p-1) {
			rowd = M/p+ smooth_L + edge_L;	
		}
		else {
			rowd = M/p + (2*smooth_L) + (2*edge_L);
		}
		pf = (float*)malloc(rowd * M * sizeof(float));
		result = (float*)malloc(rowd * M * sizeof(float));
		gx = (float*)malloc((M/p) * M * sizeof(float));
		gy = (float*)malloc((M/p) * M * sizeof(float));
		float edge_hx[edge_W][edge_W] = {-1, 0, 1, -2, 0, 2, -1,0,1}, edge_hy[edge_W][edge_W] = {-1, -2, -1, 0, 0, 0, 1,2,1};
		float smooth_h[smooth_W][smooth_W];		
		for (y=0; y<rowd; y++) {
			for (x=0; x<M; x++){
				pf[y*M + x] = 0;
				result[y*M + x] = 0;
				if(y < smooth_W && x < smooth_W) {
					smooth_h[y][x] = (1/(2*M_PI*pow(segma,2))) * exp(-((pow((y-smooth_L),2) + pow((x-smooth_L),2)) / (2*pow(segma,2))));
				}
				if(y < M/p) {
					gx[y*M + x] = 0;
					gy[y*M + x] = 0;
				}
			}
		}
		
		/* MPI Receive  M*(M/p+2*smooth_L) or M*(M/p+smooth_L) */
		if(my_rank == p-1) {
			MPI_Recv(pf, rowd * M, MPI_FLOAT, 0, my_rank, MPI_COMM_WORLD, &status);
		}
		else {
			MPI_Recv(pf, rowd * M, MPI_FLOAT, 0, my_rank, MPI_COMM_WORLD, &status);
		}

		/* Smooth Operations Matrix Multiplication*/  
		int upper_boundary = rowd;
		int memsize = 0;
		int convmax = 0;
		if(my_rank == p-1) {	
			memsize = M/p + edge_L;
			convmax = M/p + edge_L;
		}
		else {
			memsize = M/p + edge_L * 2;
			convmax = M/p + edge_L * 2;
		}
		for(i = 7; i < (7 + convmax); i++) {	
			for(j = 0; j < M; j++) {
				for(k = -smooth_L; k <= smooth_L; k++) {
					for(l = -smooth_L; l <= smooth_L; l++) {
					   if(!(j - l < 0 || i - k < 0) && (j - l < M && i - k < M))  {								
							result[(i-7)*M +j] += smooth_h[k+smooth_L][l+smooth_L] * pf[(i-k)*M + (j-l)];				
					   }
					}
				}
			}		
		}	 
		memcpy(&pf[7*M],result, memsize* M * sizeof(float));	
		/* MPI Smooth Image Send M*(M/p) */
		MPI_Send(&pf[10*M], (M/p)*M, MPI_FLOAT, 0, my_rank , MPI_COMM_WORLD);
		/* Edge Operation Convolution ! */
			/* Differentiate x-coordinate !*/
		for(i = 10; i < (M/p+10); i++) {		
			for(j = 0; j < M; j++) {
				for(k = -edge_L; k <= edge_L; k++) {
					for(l = -edge_L; l <= edge_L; l++) {
					   if(!(j - l < 0 || i - k < 0) && (j - l < M && i - k < M) && ((k+edge_L) < 3 && (l+edge_L < 3)))  {								
							gx[(i-10) * M + j] += edge_hx[k+edge_L][l+edge_L] * pf[(i-k) * M + (j-l)];
						}
					}
				}
			}		
		}	
			/* Differentiate y-coordinate !*/
		for(i = 10; i < (M/p+10); i++) {		
			for(j = 0; j < M; j++) {
				for(k = -edge_L; k <= edge_L; k++) {
					for(l = -edge_L; l <= edge_L; l++) {
					   if(!(j - l < 0 || i - k < 0) && (j - l < M && i - k < M) && ((k+edge_L) < 3 && (l+edge_L < 3)))  {								
							gy[(i-10) * M + j] += edge_hy[k+edge_L][l+edge_L] * pf[(i-k) * M + (j-l)];			
					   }
					}
				}
			}		
		}	
			/* Combine Differentiations !*/
		for (y=0; y<M/p; y++) {
			for (x=0; x<M; x++){
				pf[(10+y) * M + x] = sqrt( pow(gx[y * M + x],2) + pow(gy[y * M + x],2) );
			}
		}
		/* MPI Edge Image Send M*(M/p) */
		MPI_Send(&pf[10*M], (M/p)*M, MPI_FLOAT, 0, my_rank , MPI_COMM_WORLD);
	}
	MPI_Finalize();
}