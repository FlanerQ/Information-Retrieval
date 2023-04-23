#include <iostream>
#include <vector>
#include <bitset>
#include <cmath>
using namespace std;

// Gamma code encode
vector<uint8_t> gamma_encode(int num){
    vector<uint8_t> encoded;
    int len=0;
    int n=num;
    while (num >1) {
        len++;
        encoded.push_back(1);
        num >>= 1;
    }
    encoded.push_back(0);
    int offset=n-pow(2,len);
    int mask=1<<len;
    while(mask>0){
        encoded.push_back(mask & offset ? 1 : 0);
        mask>>=1;
    }
    return encoded;
}

// Gamma code decode 
int gamma_decode(vector<uint8_t>& encoded) {
    int num = 1;
    int offset=0;
    int flag=0;
    for(auto& i: encoded){
        if(i!=0 && flag==0) num<<=1;
        else flag=1;
        if(flag==1){
            offset<<=1;
            if(i==1) offset+=1;
        }
    }
    num+=offset;
    return num;
} 

void print(int num){
    cout<<"original "<<num<<endl;
    vector<uint8_t> encoded;
    encoded=gamma_encode(num);
    cout<<"encoded ";
    for(uint8_t& i: encoded) {
        char c=i?'1':'0';
        cout<<c<<" ";
    }
    cout<<endl;
    int decoded = gamma_decode(encoded);
    cout<<"decoded "<<decoded<<endl<<endl;
}

int main() {
    print(1025);
    print(722);
    print(936);
    print(724);
    
    return 0;
}