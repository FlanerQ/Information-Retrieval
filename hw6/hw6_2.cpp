#include <iostream>
#include <cmath>

using namespace std;

const int MAXN = 1000;
const double EPS = 1e-8;

double a[MAXN][MAXN], x[MAXN], y[MAXN];

int n;

double norm(double *v) {
    double res = 0;
    for (int i = 1; i <= n; ++i) res += v[i] * v[i];
    return sqrt(res);
}

void normalize(double *v) {
    double len = norm(v);
    for (int i = 1; i <= n; ++i) v[i] /= len;
}

void mul(double *v, double *res) {
    for (int i = 1; i <= n; ++i) {
        res[i] = 0;
        for (int j = 1; j <= n; ++j) {
            res[i] += a[i][j] * v[j];
        }
    }
}

// 幂迭代求特征向量
void powerIteration() {
    for (int i = 1; i <= n; ++i) x[i] = 1.0;

    double lambda = 0, last_lambda = 0;
    while (true) {
        mul(x, y); 
        lambda = y[1] / x[1];
        if (fabs(lambda - last_lambda) < EPS) break; // 判断迭代是否收敛
        last_lambda = lambda;
        normalize(y); 
        swap(x, y); 
    }

    cout << "The main eigenvector is:" << endl;
    //normalize
    double len =0;
    for (int i = 1; i <= n; ++i) {
        len+=x[i];
    }
    for (int i = 1; i <= n; ++i) {
        x[i]/=len;
    }
    for (int i = 1; i <= n; ++i) cout << x[i] << " ";
    cout << endl;

    cout << "The corresponding eigenvalue is: " << lambda << endl;
}

int main() {
    cin >> n;
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= n; ++j) {
            cin >> a[i][j];
        }
    }
    powerIteration();
    return 0;
}
