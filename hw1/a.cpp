#include<iostream>
#include<string>
#include<map>
#include<math.h>

using namespace std;

int n;
int m;
int num=0;
int *a;

void dfs(int i){
    if(i>(m>>1)) return;
    if(a[2*i]!=a[2*i+1]) num++;
    dfs(2*i);
    dfs(2*i+1);
}
int main(){
    string s;
    cin>>n>>s;
    m=1<<n;
    a=new int[m];
    for(int i=1;i<m;i++){      
        if(s[i-1]=='A') a[i]=1;
        else a[i]=0;
    }
    dfs(1);
    int mod=10;
    for(int i=0;i<8;i++) mod*=10;
    mod+=7;
    cout<<(1<<num)%mod<<endl;
}
