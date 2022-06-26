#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#define MAX 400


typedef struct Dimensions {  //Dimensions of matrices
  int rows;
  int columns;
} Dimensions;


typedef struct Index{ //To keep track of rows&columns to be used by threads
  int rowIndex;
  int columnIndex;
} Index;


void checkFileError(FILE*);
void checkThreadError(int); 
Dimensions readFile(FILE*, char*, int);
void createThreads(FILE*, int, char*);
void* multiplyElement(void*);
void* multiplyRow(void*);
void printMatrix(FILE*, int, char*, clock_t);


int matrix1[MAX][MAX], matrix2[MAX][MAX], result[MAX][MAX];
Dimensions dimensions3;  //Dimensions of result matrix
int innerDim;  //needed for 2D-matrix multiplication


int main(int argc, char** argv) {
  Dimensions dimensions1, dimensions2;
  char buff[255];
   
  FILE* fp;
  fp = fopen(argv[1], "r");
  checkFileError(fp);
  dimensions1 = readFile(fp, buff, 1);  //read dimensions from file 
  dimensions2 = readFile(fp, buff, 2);
  fclose(fp);
  
  if(dimensions1.columns != dimensions2.rows)  //condition for 							   multiplication
  {
    fp = fopen("output.txt", "w");
    checkFileError(fp);
    fprintf(fp, "Matrices can't be multiplied. Inner dimensions are not compatible !");
    fclose(fp);
    exit(-1);
  }
  else
  {
    dimensions3.rows = dimensions1.rows;   //dimensions of result matrix
    dimensions3.columns = dimensions2.columns;
    innerDim = dimensions2.rows;
    
    createThreads(fp, 1, "w");  //create threads to execute multiplication
    createThreads(fp, 2, "a");
  }
   
  return 0;
}


void checkFileError(FILE* fp) {
  if(!fp)  //Error in opening file
  {
    printf("Error can't open file !\n");
    exit(-1);
  } 
}


void checkThreadError(int check) {
  if(check != 0)
  {
    printf("Error in creation of threads !");
    exit(-1);
  }
}


Dimensions readFile(FILE* fp, char* buff, int flag) {
  Dimensions dimensions;
  
  fscanf(fp, "%s", buff);  //read number of matrix rows
  dimensions.rows = atoi(buff);  //convert string to integer
  fscanf(fp, "%s", buff);  //read number of matrix columns
  dimensions.columns = atoi(buff);
  
  for(int i = 0; i < dimensions.rows; i++)
  {
    for(int j = 0; j < dimensions.columns; j++)
    {
      fscanf(fp, "%s", buff);  //scan numbers in each matrix
      if(flag == 1) {  //matrix1
        matrix1[i][j] = atoi(buff);
      } else {  //matrix2
        matrix2[i][j] = atoi(buff);
      }
    }
  }
  return dimensions;  //return matrix dimensions (rows & columns)
}


void createThreads(FILE* fp, int procedureNum, char* mode) {
  int check;
  clock_t time;
  
  if(procedureNum == 1) {
    time = clock();  //start time
    
    //Number of threads = Number of elements in result matrix
    pthread_t th_element[dimensions3.rows * dimensions3.columns];
    
    //Keeping track of row and column indices (element index)
    Index index[dimensions3.rows * dimensions3.columns];
    
    int threadNum = 0;  //threads counter
    
    for(int i = 0; i < dimensions3.rows; i++) {
      for(int j = 0; j < dimensions3.columns; j++) {
        index[threadNum].rowIndex = i;  //element's row index
        index[threadNum].columnIndex = j;  //element's column index
        check = pthread_create(&th_element[threadNum], NULL, multiplyElement, &index[threadNum]); //Creating threads to compute each 						element with the given row and 					column indices in the result matrix
        checkThreadError(check);
        threadNum++; 
     }  
    }
    
    for(int i = 0; i < threadNum; i++) {
      check = pthread_join(th_element[i], NULL); //wait for all threads to 							     finish
      checkThreadError(check); 
    }
    time = clock() - time;  //Total time = start time - end time
            
  } else {
    time = clock();  //start time
    
    //Number of threads = Number of rows in result matrix
    pthread_t th_row[dimensions3.rows];
    
    //Keeping track of row index
    Index index[dimensions3.rows];
    
    for(int i = 0; i < dimensions3.rows; i++) {
      index[i].rowIndex = i;  //row index
      
      //Creating threads to compute each row in the result matrix
      check = pthread_create(&th_row[i], NULL, multiplyRow, &index[i]);
      checkThreadError(check);  
    }
    
    for(int i = 0; i < dimensions3.rows; i++) {
      check = pthread_join(th_row[i], NULL); //wait for all threads to 							finish
      checkThreadError(check); 
    }
    time = clock() - time;  //Total time = start time - end time
  }
  
  printMatrix(fp, procedureNum, mode, time);  //print result matrix
}


void* multiplyElement(void* arg) {  //Procedure 1 (thread for each element)
  Index *index = arg;
  int sum = 0;
  for(int k = 0; k < innerDim; k++)  //compute element
  { //row and column indices are constant for the same element
    sum = sum + matrix1[index->rowIndex][k] * matrix2[k][index->columnIndex];
  }
  result[index->rowIndex][index->columnIndex] = sum;  
}


void* multiplyRow(void* arg) {  //Procedure 2 (thread for each row)
  Index *index = arg;
  int sum = 0;
  for(int j = 0; j < dimensions3.columns; j++) //loop on elements of row
  {
    for(int k = 0; k < innerDim; k++)  //compute element
    { //row index is constant for the same row
      sum = sum + matrix1[index->rowIndex][k] * matrix2[k][j];
    }
    result[index->rowIndex][j] = sum;
    sum = 0;  
  }
}


void printMatrix(FILE* fp, int procedureNum, char* mode, clock_t time) 
{ //print result matrix
  fp = fopen("output.txt", mode);  //mode = "w" in proc1 then "a" in proc2
  checkFileError(fp);
  
  if(procedureNum == 1) {  //print time taken for procedure 1
    fprintf(fp, "By Element:\n");  
  } else {  //procedure 2
    fprintf(fp, "By Row:\n"); 
  }
  
  for(int i = 0; i < dimensions3.rows; i++)
  {
    for(int j = 0; j < dimensions3.columns; j++)
    {
      fprintf(fp, "%d\t", result[i][j]);
    }
    fprintf(fp, "\n");
  }
  
  if(procedureNum == 1) {  //print time taken for procedure 1
    fprintf(fp, "END1\t[%f sec]\n\n", ((double)time) / CLOCKS_PER_SEC);  
  } else {  //procedure 2
      fprintf(fp, "END2\t[%f sec]\n", ((double)time) / CLOCKS_PER_SEC);
  }
  
  fclose(fp);
}
