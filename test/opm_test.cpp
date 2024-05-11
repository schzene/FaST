#include <iostream>
#include <vector>
#include <omp.h>
#include <module.h>
using namespace std;

vector<vector<int>> multiplyMatrix(vector<vector<int>> &A, vector<vector<int>> &B)
{
    int m = A.size();
    int n = A[0].size();
    int p = B[0].size();

    vector<vector<int>> result(m, vector<int>(p, 0));

    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < p; j++)
        {
            for (int k = 0; k < n; k++)
            {
                result[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    return result;
}

vector<vector<int>> multiplyMatrix_omp(vector<vector<int>> &A, vector<vector<int>> &B)
{
    int m = A.size();
    int n = A[0].size();
    int p = B[0].size();

    vector<vector<int>> result(m, vector<int>(p, 0));
#pragma omp parallel for
    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < p; j++)
        {
            for (int k = 0; k < n; k++)
            {
                result[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    return result;
}

void printMatrix(const vector<vector<int>> &matrix)
{
    for (const auto &row : matrix)
    {
        for (int element : row)
        {
            cout << element << " ";
        }
        cout << endl;
    }
}

int main()
{
    int M = 128;
    int N = 768;
    int K = 3072;
    vector<vector<int>> A(M, vector<int>(N, 0));
    vector<vector<int>> B(N, vector<int>(K, 0));

    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {
            A[i][j] = rand() % 10;
        }
    }
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < K; j++)
        {
            B[i][j] = rand() % 10;
        }

        // auto start_time = high_resolution_clock::now();
        INIT_TIMER;
        START_TIMER;
        vector<vector<int>> result1 = multiplyMatrix(A, B);
        STOP_TIMER("Single mat");

        // auto cost_single = high_resolution_clock::now() - start_time;

        // auto start_time2 = high_resolution_clock::now();

        START_TIMER;
        vector<vector<int>> result2 = multiplyMatrix_omp(A, B);
        STOP_TIMER("omp");
        // auto cost_omp = high_resolution_clock::now() - start_time2;

        // cout << "signle_time: " << cost_single.count() << endl;
        // cout << "Result of matrix multiplication (A * B):" << endl;
        printMatrix(result1);

        // cout << "omp_time: " << cost_omp.count() << endl;
        // cout << "Result of matrix multiplication (A * B):" << endl;
        // printMatrix(result2);
        return 0;
    }
}