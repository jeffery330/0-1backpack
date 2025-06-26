#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

typedef struct {
    int id;
    int weight;
    double value;
    double density;
} Item;

/* 全局变量用于回溯法 */
double g_backtracking_max_value = 0.0;
int* g_backtracking_best_selection = NULL;
int g_backtracking_num_items;
int g_backtracking_capacity;
Item* g_backtracking_items;

/* 比较函数，用于按密度降序排序 */
int compareItems(const void* a, const void* b) {
    Item* itemA = (Item*)a;
    Item* itemB = (Item*)b;
    if (itemB->density > itemA->density) return 1;
    if (itemB->density < itemA->density) return -1;
    return 0;
}

/* 生成随机物品 */
void generate_items(Item* items, int n) {
    int i;
    printf("生成 %d 个随机物品...\n", n);
    for (i = 0; i < n; i++) {
        items[i].id = i + 1;
        items[i].weight = (rand() % 100) + 1;  /* 1-100之间 */
        items[i].value = ((rand() % 90001) + 10000) / 100.0;  /* 100.00-1000.00之间 */
        items[i].density = items[i].value / items[i].weight;
    }
    printf("物品生成完成。\n\n");
}

/* 打印解决方案到控制台 */
void print_solution(const char* method_name, Item* items, int* selection, int n, int capacity, double execution_time) {
    double total_value = 0;
    int total_weight = 0;
    int selected_count = 0;
    int i;
    
    printf("\n========== [%s] 算法结果 ==========\n", method_name);
    printf("执行时间: %.2f ms\n", execution_time);
    printf("背包容量: %d\n", capacity);
    printf("\n选中的物品:\n");
    printf("%-8s %-8s %-10s\n", "物品编号", "重量", "价值");
    printf("--------------------------------\n");
    
    for (i = 0; i < n; i++) {
        if (selection[i]) {
            printf("%-8d %-8d %-10.2f\n", items[i].id, items[i].weight, items[i].value);
            total_value += items[i].value;
            total_weight += items[i].weight;
            selected_count++;
        }
    }
    
    printf("--------------------------------\n");
    printf("选中物品数量: %d\n", selected_count);
    printf("总重量: %d\n", total_weight);
    printf("总价值: %.2f\n", total_value);
    printf("容量利用率: %.2f%%\n", (double)total_weight / capacity * 100);
    printf("==========================================\n\n");
}

/* 将算法结果写入CSV文件 */
void write_to_csv(const char* filename, const char* method_name, Item* items, int* selection, int n, int capacity, double execution_time) {
    FILE* fp = fopen(filename, "a");
    if (!fp) {
        printf("无法打开CSV文件进行写入: %s\n", filename);
        return;
    }
    
    double total_value = 0;
    int total_weight = 0;
    int selected_count = 0;
    int i;
    
    // 写入算法执行摘要
    fprintf(fp, "算法: %s\n", method_name);
    fprintf(fp, "执行时间: %.2f ms\n", execution_time);
    fprintf(fp, "背包容量: %d\n", capacity);
    fprintf(fp, "\n");
    
    // 写入选中的物品表头
    fprintf(fp, "物品编号,重量,价值\n");
    
    // 写入选中的物品数据
    for (i = 0; i < n; i++) {
        if (selection[i]) {
            fprintf(fp, "%d,%d,%.2f\n", items[i].id, items[i].weight, items[i].value);
            total_value += items[i].value;
            total_weight += items[i].weight;
            selected_count++;
        }
    }
    
    // 写入摘要统计
    fprintf(fp, "\n");
    fprintf(fp, "选中物品数量: %d\n", selected_count);
    fprintf(fp, "总重量: %d\n", total_weight);
    fprintf(fp, "总价值: %.2f\n", total_value);
    fprintf(fp, "容量利用率: %.2f%%\n", (double)total_weight / capacity * 100);
    fprintf(fp, "==========================================\n\n");
    
    fclose(fp);
}

/* 蛮力法 */
void brute_force_knapsack(Item* items, int n, int C, const char* csv_filename) {
    clock_t start = clock();
    
    double max_value = 0.0;
    int* best_selection = (int*)calloc(n, sizeof(int));
    long long i;
    int j;
    
    if (!best_selection) {
        printf("蛮力法内存分配失败。\n");
        return;
    }

    printf("蛮力法开始计算（将检查 %lld 种组合）...\n", 1LL << n);
    
    for (i = 0; i < (1LL << n); i++) {
        int current_weight = 0;
        double current_value = 0.0;
        int* current_selection = (int*)calloc(n, sizeof(int));
        if(!current_selection) continue;

        for (j = 0; j < n; j++) {
            if ((i >> j) & 1) {
                current_weight += items[j].weight;
                current_value += items[j].value;
                current_selection[j] = 1;
            }
        }

        if (current_weight <= C && current_value > max_value) {
            max_value = current_value;
            memcpy(best_selection, current_selection, n * sizeof(int));
        }
        free(current_selection);
        
        /* 进度显示（每100万次显示一次） */
        if (i > 0 && i % 1000000 == 0) {
            printf("已处理: %lld / %lld (%.1f%%)\n", i, 1LL << n, (double)i / (1LL << n) * 100);
        }
    }

    clock_t end = clock();
    double execution_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    print_solution("蛮力法", items, best_selection, n, C, execution_time);
    if (csv_filename) {
        write_to_csv(csv_filename, "蛮力法", items, best_selection, n, C, execution_time);
    }
    free(best_selection);
}

/* 动态规划法 */
void dynamic_programming_knapsack(Item* items, int n, int C, const char* csv_filename) {
    clock_t start = clock();
    
    printf("动态规划法开始计算（创建 %d x %d 的DP表）...\n", n+1, C+1);
    
    double** dp = (double**)malloc((n + 1) * sizeof(double*));
    int** keep = (int**)malloc((n + 1) * sizeof(int*));
    int i, w, current_cap;
    
    if (!dp || !keep) {
        printf("动态规划法内存分配失败。\n");
        if (dp) free(dp);
        if (keep) free(keep);
        return;
    }

    for (i = 0; i <= n; i++) {
        dp[i] = (double*)malloc((C + 1) * sizeof(double));
        keep[i] = (int*)malloc((C + 1) * sizeof(int));
        if (!dp[i] || !keep[i]) {
            int k;
            printf("动态规划法DP表内存分配失败。\n");
            for(k = 0; k < i; k++) { 
                free(dp[k]); 
                free(keep[k]); 
            }
            free(dp); 
            free(keep);
            return;
        }
    }
    
    /* 填充DP表 */
    for (i = 0; i <= n; i++) {
        for (w = 0; w <= C; w++) {
            if (i == 0 || w == 0) {
                dp[i][w] = 0;
                keep[i][w] = 0;
            } else if (items[i-1].weight <= w) {
                double value_if_taken = items[i-1].value + dp[i-1][w - items[i-1].weight];
                double value_if_not_taken = dp[i-1][w];
                if (value_if_taken > value_if_not_taken) {
                    dp[i][w] = value_if_taken;
                    keep[i][w] = 1;
                } else {
                    dp[i][w] = value_if_not_taken;
                    keep[i][w] = 0;
                }
            } else {
                dp[i][w] = dp[i-1][w];
                keep[i][w] = 0;
            }
        }
        
        /* 进度显示 */
        if (i > 0 && (i % (n/10) == 0 || i == n)) {
            printf("DP表填充进度: %d/%d (%.1f%%)\n", i, n, (double)i / n * 100);
        }
    }

    /* 重构解 */
    {
        int* selection = (int*)calloc(n, sizeof(int));
        if (selection) {
            current_cap = C;
            for (i = n; i > 0; i--) {
                if (keep[i][current_cap] == 1) {
                    selection[i-1] = 1;
                    current_cap -= items[i-1].weight;
                }
            }
            
            clock_t end = clock();
            double execution_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
            
            print_solution("动态规划法", items, selection, n, C, execution_time);
            if (csv_filename) {
                write_to_csv(csv_filename, "动态规划法", items, selection, n, C, execution_time);
            }
            free(selection);
        }
    }

    /* 释放内存 */
    for (i = 0; i <= n; i++) {
        free(dp[i]);
        free(keep[i]);
    }
    free(dp);
    free(keep);
}

/* 贪心法 */
void greedy_knapsack(Item* items, int n, int C, const char* csv_filename) {
    clock_t start = clock();
    
    printf("贪心法开始计算（按价值密度排序）...\n");
    
    Item* sorted_items = (Item*)malloc(n * sizeof(Item));
    int current_weight = 0;
    int* selection;
    int i;
    
    if (!sorted_items) {
        printf("贪心法内存分配失败。\n");
        return;
    }
    memcpy(sorted_items, items, n * sizeof(Item));
    qsort(sorted_items, n, sizeof(Item), compareItems);

    selection = (int*)calloc(n, sizeof(int));
    if (!selection) {
        printf("贪心法选择数组内存分配失败。\n");
        free(sorted_items);
        return;
    }

    for (i = 0; i < n; i++) {
        if (current_weight + sorted_items[i].weight <= C) {
            selection[sorted_items[i].id - 1] = 1;
            current_weight += sorted_items[i].weight;
        }
    }

    clock_t end = clock();
    double execution_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    print_solution("贪心法", items, selection, n, C, execution_time);
    if (csv_filename) {
        write_to_csv(csv_filename, "贪心法", items, selection, n, C, execution_time);
    }
    
    free(sorted_items);
    free(selection);
}

/* 回溯法辅助函数 */
double calculate_bound(int index, int current_weight, double current_value) {
    double bound = current_value;
    int remaining_capacity = g_backtracking_capacity - current_weight;
    int i = index;

    while (i < g_backtracking_num_items && g_backtracking_items[i].weight <= remaining_capacity) {
        remaining_capacity -= g_backtracking_items[i].weight;
        bound += g_backtracking_items[i].value;
        i++;
    }

    if (i < g_backtracking_num_items) {
        bound += (double)remaining_capacity * g_backtracking_items[i].density;
    }

    return bound;
}

void backtrack_recursive(int index, int current_weight, double current_value, int* current_selection) {
    if (index == g_backtracking_num_items) {
        if (current_value > g_backtracking_max_value) {
            g_backtracking_max_value = current_value;
            memcpy(g_backtracking_best_selection, current_selection, g_backtracking_num_items * sizeof(int));
        }
        return;
    }

    if (calculate_bound(index, current_weight, current_value) <= g_backtracking_max_value) {
        return;
    }

    if (current_weight + g_backtracking_items[index].weight <= g_backtracking_capacity) {
        current_selection[index] = 1;
        backtrack_recursive(index + 1, current_weight + g_backtracking_items[index].weight, current_value + g_backtracking_items[index].value, current_selection);
    }

    current_selection[index] = 0;
    backtrack_recursive(index + 1, current_weight, current_value, current_selection);
}

/* 回溯法 */
void backtracking_knapsack(Item* items, int n, int C, const char* csv_filename) {
    clock_t start = clock();
    
    printf("回溯法开始计算（带剪枝优化）...\n");
    
    Item* sorted_items = (Item*)malloc(n * sizeof(Item));
    int* current_selection;
    int* final_selection;
    int i;
    
    if(!sorted_items) { 
        printf("回溯法内存分配失败。\n"); 
        return; 
    }
    memcpy(sorted_items, items, n * sizeof(Item));
    qsort(sorted_items, n, sizeof(Item), compareItems);

    g_backtracking_items = sorted_items;
    g_backtracking_num_items = n;
    g_backtracking_capacity = C;
    g_backtracking_max_value = 0.0;
    
    g_backtracking_best_selection = (int*)calloc(n, sizeof(int));
    current_selection = (int*)calloc(n, sizeof(int));
    if (!g_backtracking_best_selection || !current_selection) {
        printf("回溯法选择数组内存分配失败。\n");
        if(sorted_items) free(sorted_items);
        if(g_backtracking_best_selection) free(g_backtracking_best_selection);
        if(current_selection) free(current_selection);
        return;
    }

    backtrack_recursive(0, 0, 0.0, current_selection);
    
    final_selection = (int*)calloc(n, sizeof(int));
    if (final_selection) {
        for(i = 0; i < n; i++) {
            if(g_backtracking_best_selection[i] == 1) {
                final_selection[sorted_items[i].id - 1] = 1;
            }
        }
        
        clock_t end = clock();
        double execution_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        
        print_solution("回溯法", items, final_selection, n, C, execution_time);
        if (csv_filename) {
            write_to_csv(csv_filename, "回溯法", items, final_selection, n, C, execution_time);
        }
        free(final_selection);
    }
    
    free(sorted_items);
    free(g_backtracking_best_selection);
    free(current_selection);
}

/* 显示菜单 */
void show_menu() {
    printf("\n=========== 0-1背包问题===========\n");
    printf("支持的物品数量: 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 20000, 40000, 80000, 160000, 320000\n");
    printf("支持的背包容量: 10000, 100000, 1000000\n");
    printf("物品重量范围: 1-100\n");
    printf("物品价值范围: 100.00-1000.00\n");
    printf("================================================\n");
}

/* 检查输入有效性 */
int is_valid_n(int n) {
    int valid_n[] = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 20000, 40000, 80000, 160000, 320000};
    int i;
    for (i = 0; i < sizeof(valid_n)/sizeof(valid_n[0]); i++) {
        if (n == valid_n[i]) return 1;
    }
    return 0;
}

int is_valid_capacity(int capacity) {
    return (capacity == 10000 || capacity == 100000 || capacity == 1000000);
}

/* 算法可行性检查 */
void check_algorithm_feasibility(int n, int capacity) {
    printf("\n============ 算法可行性分析 ============\n");
    
    /* 贪心法 - 总是可行 */
    printf("✓ 贪心法: 可行 (时间复杂度: O(n log n))\n");
    
    /* 动态规划法 */
    long long dp_complexity = (long long)n * capacity;
    if (dp_complexity <= 400000000LL) {  /* 4亿的阈值 */
        printf("✓ 动态规划法: 可行 (时间复杂度: O(n×C) = %lld)\n", dp_complexity);
    } else {
        printf("✗ 动态规划法: 不可行 (时间复杂度过高: %lld > 4亿)\n", dp_complexity);
    }
    
    /* 回溯法 */
    if (n <= 25) {
        printf("✓ 回溯法: 可行 (但可能较慢)\n");
    } else {
        printf("✗ 回溯法: 不可行 (N=%d > 25, 时间复杂度: O(2^%d))\n", n, n);
    }
    
    /* 蛮力法 */
    if (n <= 25) {
        printf("✓ 蛮力法: 可行 (但会很慢)\n");
    } else {
        printf("✗ 蛮力法: 不可行 (N=%d > 25, 需要检查 2^%d 种组合)\n", n, n);
    }
    
    printf("========================================\n\n");
}

/* 主函数 */
int main() {
    int n, capacity, choice;
    Item* items = NULL;
    clock_t program_start, program_end;
    char csv_filename[100];
    char student_info[100];
    
    srand((unsigned int)time(NULL));
    
    #if defined(_WIN32) || defined(_WIN64)
    SetConsoleOutputCP(65001);
    #endif

    // 获取学生信息用于文件名
    printf("请输入学号-姓名 (例如: 20231060000-张三): ");
    scanf("%s", student_info);
    sprintf(csv_filename, "%s-数据.csv", student_info);
    
    // 初始化CSV文件，写入表头
    FILE* fp = fopen(csv_filename, "w");
    if (fp) {
        fprintf(fp, "0-1背包问题实验数据\n\n");
        fclose(fp);
    } else {
        printf("无法创建CSV文件: %s\n", csv_filename);
        return 1;
    }

    while (1) {
        show_menu();
        
        /* 输入物品数量 */
        printf("请输入物品数量 (输入0退出): ");
        if (scanf("%d", &n) != 1) {
            printf("输入无效，请重新输入。\n");
            while (getchar() != '\n');  /* 清空输入缓冲区 */
            continue;
        }
        
        if (n == 0) {
            printf("程序退出。\n");
            break;
        }
        
        if (!is_valid_n(n)) {
            printf("无效的物品数量！请选择支持的数量。\n");
            continue;
        }
        
        /* 输入背包容量 */
        printf("请输入背包容量: ");
        if (scanf("%d", &capacity) != 1) {
            printf("输入无效，请重新输入。\n");
            while (getchar() != '\n');
            continue;
        }
        
        if (!is_valid_capacity(capacity)) {
            printf("无效的背包容量！请选择: 10000, 100000, 或 1000000。\n");
            continue;
        }
        
        /* 生成物品数据 */
        printf("\n正在为 N=%d, C=%d 生成测试数据...\n", n, capacity);
        items = (Item*)malloc(n * sizeof(Item));
        if (!items) {
            printf("内存分配失败！\n");
            continue;
        }
        
        generate_items(items, n);
        
        /* 分析算法可行性 */
        check_algorithm_feasibility(n, capacity);
        
        /* 记录程序开始时间（不包含数据生成时间） */
        program_start = clock();
        
        /* 选择要运行的算法 */
        printf("请选择要运行的算法:\n");
        printf("1. 贪心法\n");
        printf("2. 动态规划法\n");
        printf("3. 回溯法\n");
        printf("4. 蛮力法\n");
        printf("5. 运行所有可行的算法\n");
        printf("选择 (1-5): ");
        
        if (scanf("%d", &choice) != 1) {
            printf("输入无效。\n");
            free(items);
            while (getchar() != '\n');
            continue;
        }
        
        printf("\n开始算法测试...\n");
        printf("======================================\n");
        
        switch (choice) {
            case 1:
                greedy_knapsack(items, n, capacity, csv_filename);
                break;
                
            case 2:
                if ((long long)n * capacity <= 400000000LL) {
                    dynamic_programming_knapsack(items, n, capacity, csv_filename);
                } else {
                    printf("动态规划法: 问题规模过大，跳过执行。\n");
                }
                break;
                
            case 3:
                if (n <= 25) {
                    backtracking_knapsack(items, n, capacity, csv_filename);
                } else {
                    printf("回溯法: N=%d 过大，跳过执行。\n", n);
                }
                break;
                
            case 4:
                if (n <= 25) {
                    brute_force_knapsack(items, n, capacity, csv_filename);
                } else {
                    printf("蛮力法: N=%d 过大，跳过执行。\n", n);
                }
                break;
                
            case 5:
                /* 贪心法 */
                greedy_knapsack(items, n, capacity, csv_filename);
                
                /* 动态规划法 */
                if ((long long)n * capacity <= 400000000LL) {
                    dynamic_programming_knapsack(items, n, capacity, csv_filename);
                } else {
                    printf("动态规划法: 问题规模过大，跳过执行。\n\n");
                }
                
                /* 回溯法 */
                if (n <= 25) {
                    backtracking_knapsack(items, n, capacity, csv_filename);
                } else {
                    printf("回溯法: N=%d 过大，跳过执行。\n\n", n);
                }
                
                /* 蛮力法 */
                if (n <= 25) {
                    brute_force_knapsack(items, n, capacity, csv_filename);
                } else {
                    printf("蛮力法: N=%d 过大，跳过执行。\n\n", n);
                }
                break;
                
            default:
                printf("无效选择。\n");
                break;
        }
        
        /* 记录程序结束时间 */
        program_end = clock();
        double total_execution_time = ((double)(program_end - program_start)) / CLOCKS_PER_SEC * 1000.0;
        
        printf("======================================\n");
        printf("程序总执行时间: %.2f ms\n", total_execution_time);
        printf("======================================\n\n");
        
        free(items);
        
        /* 询问是否继续 */
        printf("是否继续测试其他配置? (y/n): ");
        char continue_choice;
        scanf(" %c", &continue_choice);
        if (continue_choice != 'y' && continue_choice != 'Y') {
            break;
        }
    }
    
    printf("测试完成！数据已保存至: %s\n", csv_filename);
    printf("该文件可以直接用Excel打开，或转换为xlsx格式。\n");
    return 0;
}