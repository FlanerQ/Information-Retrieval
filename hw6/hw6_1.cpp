#include <cmath>
#include <iostream>
#include <vector>
const int MAXN = 1000;
const double EPS = 1e-8;
const double alpha = 0.1;

double a[MAXN][MAXN], b[MAXN][MAXN];

int n;

void createMatrix() {
    for (int i = 0; i < n; i++) {
        std::vector<int> index;
        for (int j = 0; j < n; j++) {
            if (a[i][j] == 1)
                index.push_back(j);
        }
        for (int k : index)
            b[i][k] = 1.0 / index.size();
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            b[i][j] *= (1.0 - alpha);
            b[i][j] += (alpha / n);
        }
    }
    std::cout<<"P: "<<std::endl;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            std::cout<<b[i][j]<<" ";
        }
        std::cout<<std::endl;
    }
    std::cout<<std::endl;
}

double* mul(double *v) {
    double* res=new double[n];
    for (int i = 0; i < n; i++) {
        res[i]=0;
        for (int j = 0; j <n; j++) {
            res[i]+=v[j]*b[j][i];
        }
    }
    return std::move(res);
}

void powerIteration(){
    double* x=new double[n];
    for(int i=0;i<n;i++) x[i]=b[0][i];
    std::cout<<"Start power iteration:"<<std::endl;
    while(true){
        double *res=mul(x);
        for (int i = 0; i < n; ++i) {
            std::cout << res[i] << " ";
        }
        std::cout<<std::endl;
        int flag=0;
        for(int i=0;i<n;i++){
            if(fabs(res[i]-x[i])>EPS) flag++; 
        }
        x=std::move(res);
        if(flag==0) break;
    }
    for (int i = 0; i < n; ++i) {
        std::cout << x[i] << " ";
    }
}

int main() {
    std::cin >> n;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            std::cin >> a[i][j];
        }
    }
    std::cout<<std::endl;
    createMatrix();
    powerIteration();
    return 0;
}
