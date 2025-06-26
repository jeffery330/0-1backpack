#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// 定义物品结构体
typedef struct {
    int weight;
    int value;
    double density; // 价值/重量比
} Item;

// 用于排序的比较函数
int compareItems(const void *a, const void *b) {
    Item *itemA = (Item *)a;
    Item *itemB = (Item *)b;
    if (itemA->density < itemB->density) return 1;
    if (itemA->density > itemB->density) return -1;
    return 0;
}

// 动态规划法 - O(n×C)
double dynamicProgramming(Item *items, int n, int capacity) {
    clock_t start = clock();
    
    // 创建二维DP表
    int **dp = (int**)malloc((n + 1) * sizeof(int *));
    for (int i = 0; i <= n; i++) {
        dp[i] = (int *)calloc(capacity + 1, sizeof(int));
    }
    
    // 填充DP表
    for (int i = 1; i <= n; i++) {
        for (int w = 1; w <= capacity; w++) {
            if (items[i-1].weight <= w) {
                dp[i][w] = fmax(dp[i-1][w], dp[i-1][w-items[i-1].weight] + items[i-1].value);
            } else {
                dp[i][w] = dp[i-1][w];
            }
        }
    }
    
    int maxValue = dp[n][capacity];
    
    // 释放内存
    for (int i = 0; i <= n; i++) {
        free(dp[i]);
    }
    free(dp);
    
    clock_t end = clock();
    double executionTime = ((double)(end - start) / CLOCKS_PER_SEC) * 1000; // 转换为毫秒
    return executionTime;
}

// 贪心法 - O(n log n)
double greedyAlgorithm(Item *items, int n, int capacity) {
    clock_t start = clock();
    
    // 按密度排序
    qsort(items, n, sizeof(Item), compareItems);
    
    int currentWeight = 0;
    int maxValue = 0;
    
    // 贪心选择
    for (int i = 0; i < n; i++) {
        if (currentWeight + items[i].weight <= capacity) {
            currentWeight += items[i].weight;
            maxValue += items[i].value;
        } else {
            // 0-1背包问题不能部分装入，所以跳出循环
            break;
        }
    }
    
    clock_t end = clock();
    double executionTime = ((double)(end - start) / CLOCKS_PER_SEC) * 1000; // 转换为毫秒
    return executionTime;
}

int main() {
    int capacities[] = {100000}; // 背包容量
    int numCapacities = sizeof(capacities) / sizeof(capacities[0]);
    
    // 修改后的物品数量列表，从1000递增到40000
    int nValues[] = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 
                     10000, 15000, 20000, 25000, 30000, 35000, 40000};
    int numNValues = sizeof(nValues) / sizeof(nValues[0]);
    
    // 生成CSV文件名
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char filename[100];
    sprintf(filename, "knapsack_results_N=%d_C=%d.csv", nValues[numNValues-1], capacities[0]);
    
    // 打开CSV文件
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("无法创建CSV文件\n");
        return 1;
    }
    
    // 写入CSV表头
    fprintf(file, "N=%d, C=%d\n", nValues[numNValues-1], capacities[0]);
    fprintf(file, "算法,执行时间 (ms)\n");
    
    // 遍历不同的背包容量
    for (int c = 0; c < numCapacities; c++) {
        int capacity = capacities[c];
        
        // 遍历不同的物品数量
        for (int i = 0; i < numNValues; i++) {
            int n = nValues[i];
            
            // 分配物品数组内存
            Item *items = (Item *)malloc(n * sizeof(Item));
            if (items == NULL) {
                printf("内存分配失败\n");
                fclose(file);
                return 1;
            }
            
            // 生成随机物品数据
            srand(time(NULL) + i);
            for (int j = 0; j < n; j++) {
                items[j].weight = rand() % 100 + 1; // 随机重量(1-100)
                items[j].value = rand() % 100 + 1;  // 随机价值(1-100)
                items[j].density = (double)items[j].value / items[j].weight;
            }
            
            // 记录动态规划法执行时间
            double dpTime = dynamicProgramming(items, n, capacity);
            fprintf(file, "动态规划法,%.6f\n", dpTime);
            
            // 记录贪心法执行时间
            double greedyTime = greedyAlgorithm(items, n, capacity);
            fprintf(file, "贪心法,%.6f\n", greedyTime);
            
            // 释放内存
            free(items);
        }
    }
    
    // 关闭文件
    fclose(file);
    printf("CSV文件已生成: %s\n", filename);
    
    return 0;
}    