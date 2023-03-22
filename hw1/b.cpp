#include<iostream>
#include<string>
#include<map>
#include<math.h>
#include<vector>

using namespace std;

int n,x;
int res=0;

struct SHEN{
    int hp;
    int type;
};

SHEN *a;

struct lr{
    int l,r;
};

vector<lr> b,c;

void getres(){
    int flag=0;
    int samenum=1;
    for(int i=0;i<n-1;i++){
        if(a[i].type==a[i+1].type){
            if(flag==1){
                flag=0;
                lr p;
                p.l=i-samenum+1;
                p.r=i-1;
                b.push_back(p);
                samenum=1;
            }
            else samenum++;
        }
        else{
            if(flag==1) samenum++;
            else{
                flag=1;
                if(samenum==2) res+=a[i-1].hp;
                lr p;
                p.l=i-samenum+1;
                p.r=i-1;
                if(samenum>2) c.push_back(p);
                samenum=1;
            }   
        }
    }
    for(lr v:c){
        for(int i=v.l;i<=v.r;i++) res+=a[i].hp;
    }

    for(lr v:b){
        if((v.r-v.l%2)==1){
            for(int i=v.l;i<=v.r;i+=2){
                if((a[i].hp+a[i+1].hp)>x){
                    res+=x;
                }
                else res+=(a[i].hp+a[i+1].hp);
            }
        }
        else{
            
        }
    }
}

int main(){
    string str;
    cin>>n>>x;
    a=new SHEN[n];
    for(int i=0;i<n;i++)  cin>>a[i].hp;
    cin>>str;
    for(int i=0;i<n;i++){
        if(str[i]=='F') a[i].type=0;
        else if(str[i]=='W') a[i].type=1;
        else a[i].type=2;
    }
    if(n==1) {
        cout<<a[0].hp<<endl;
        return;
    }
    getres();
    cout<<res<<endl;
}
