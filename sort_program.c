#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h> 
typedef struct { int l, r; } SItem;
typedef struct { SItem* items; int top, cap; } Stack;
Stack* createStack(int cap) { 
    Stack* s = malloc(sizeof(Stack)); 
    s->cap = cap; s->top = -1; 
    s->items = malloc(cap * sizeof(SItem)); 
    return s; 
}
void push(Stack* s, int l, int r) { 
    if (s->top == s->cap - 1) { 
        s->cap *= 2; 
        s->items = realloc(s->items, s->cap * sizeof(SItem)); 
    } 
    s->items[++s->top] = (SItem){l, r}; 
}
SItem pop(Stack* s) { 
    return s->top >= 0 ? s->items[s->top--] : (SItem){-1, -1}; 
}
int isEmpty(Stack* s) { return s->top == -1; }
void freeStack(Stack* s) { free(s->items); free(s); }
void swap(int* a, int* b) { int t = *a; *a = *b; *b = t; }
int partition(int arr[], int l, int r) {
    int m = l + (r - l) / 2;
    if (arr[l] > arr[m]) swap(&arr[l], &arr[m]);
    if (arr[l] > arr[r]) swap(&arr[l], &arr[r]);
    if (arr[m] > arr[r]) swap(&arr[m], &arr[r]);
    swap(&arr[m], &arr[r]);
    int pivot = arr[r];
    int i = l - 1;
    for (int j = l; j < r; j++) {
        if (arr[j] <= pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[r]);
    return i + 1;
}
void quickSortRec(int arr[], int l, int r) {
    if (l < r) {
        int pi = partition(arr, l, r);
        quickSortRec(arr, l, pi - 1);
        quickSortRec(arr, pi + 1, r);
    }
}
void quickSortIter(int arr[], int l, int r) {
    Stack* s = createStack(1000);
    push(s, l, r);
    while (!isEmpty(s)) {
        SItem it = pop(s);
        if (it.l < it.r) {
            int pi = partition(arr, it.l, it.r);
            if (pi - 1 > it.l) push(s, it.l, pi - 1);
            if (pi + 1 < it.r) push(s, pi + 1, it.r);
        }
    }
    freeStack(s);
}

void merge(int arr[], int l, int m, int r) {
    int n1 = m - l + 1, n2 = r - m;
    int* L = malloc(n1 * sizeof(int));
    int* R = malloc(n2 * sizeof(int));
    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int i = 0; i < n2; i++) R[i] = arr[m + 1 + i];
    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    free(L); free(R);
}

void mergeSort(int arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}
void parallelMergeSortSections(int arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        if (r - l > 1000) {
            #pragma omp parallel sections
            {
                #pragma omp section
                parallelMergeSortSections(arr, l, m); 
                
                #pragma omp section  
                parallelMergeSortSections(arr, m + 1, r); 
            }
        } else {
            parallelMergeSortSections(arr, l, m);
            parallelMergeSortSections(arr, m + 1, r);
        }
        
        merge(arr, l, m, r);
    }
}
void parallelMergeSortTasks(int arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        if (r - l > 1000) {
            #pragma omp task shared(arr)
            parallelMergeSortTasks(arr, l, m);
            
            #pragma omp task shared(arr)  
            parallelMergeSortTasks(arr, m + 1, r);
            
            #pragma omp taskwait 
        } else {
            parallelMergeSortTasks(arr, l, m);
            parallelMergeSortTasks(arr, m + 1, r);
        }
        
        merge(arr, l, m, r);
    }
}

void parallelMergeSortWrapper(int arr[], int l, int r) {
    #pragma omp parallel
    {
        #pragma omp single nowait
        parallelMergeSortTasks(arr, l, r);
    }
}

int isSorted(int arr[], int n) {
    for (int i = 1; i < n; i++) {
        if (arr[i] < arr[i - 1]) return 0;
    }
    return 1;
}
double getTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}
void generateData(const char* filename, int count) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("无法创建文件: %s\n", filename);
        return;
    }
    fprintf(file, "%d\n", count);
    srand(time(NULL));
    for (int i = 0; i < count; i++) {
        fprintf(file, "%d\n", rand() % 1000000);
    }
    fclose(file);
    printf("生成数据: %s (%d条记录)\n", filename, count);
}
int* readData(const char* filename, int* count) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("无法打开文件: %s\n", filename);
        return NULL;
    }
    fscanf(file, "%d", count);
    int* data = malloc(*count * sizeof(int));
    for (int i = 0; i < *count; i++) {
        fscanf(file, "%d", &data[i]);
    }
    fclose(file);
    return data;
}
void testAlgorithm(const char* name, void (*sortFunc)(int[], int, int), 
                   int arr[], int n, const char* resultFile) {
    int* testArr = malloc(n * sizeof(int));
    memcpy(testArr, arr, n * sizeof(int));
    double startTime = getTime();
    sortFunc(testArr, 0, n - 1);
    double timeUsed = getTime() - startTime;
    printf("%s: %.6f 秒 - %s\n", name, timeUsed, 
           isSorted(testArr, n) ? "排序正确" : "排序错误");
    FILE* file = fopen(resultFile, "a");
    if (file) {
        fprintf(file, "%s,%.6f\n", name, timeUsed);
        fclose(file);
    }
    
    free(testArr);
}
int main() {
    printf("=== 排序算法性能测试 ===\n");
    generateData("test_data.txt", 100000);
    int dataSize;
    int* data = readData("test_data.txt", &dataSize);
    if (!data) {
        return 1;
    }
    printf("测试数据量: %d\n\n", dataSize);
    const char* resultFile = "test_results.csv";
    FILE* csv = fopen(resultFile, "w");
    if (csv) {
        fprintf(csv, "算法,时间(秒)\n");
        fclose(csv);
    }
    testAlgorithm("快速排序(递归)", quickSortRec, data, dataSize, resultFile);
    testAlgorithm("快速排序(迭代)", quickSortIter, data, dataSize, resultFile);
    testAlgorithm("归并排序(串行)", mergeSort, data, dataSize, resultFile);
    testAlgorithm("归并排序(并行-sections)", parallelMergeSortSections, data, dataSize, resultFile);
    testAlgorithm("归并排序(并行-tasks)", parallelMergeSortWrapper, data, dataSize, resultFile);

    free(data);
    printf("\n性能结果已保存到: %s\n", resultFile);
    printf("测试完成！\n");
    return 0;
}