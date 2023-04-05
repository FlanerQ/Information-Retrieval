#include <iostream>
#include <map>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <math.h>

std::map<std::string,std::vector<int>> inverted_index;

int docs_num;

int skipNum=0;

void show_list(const std::vector<int> &l){
    for(auto i:l) std::cout<<i<<" ";
    std::cout<<std::endl;
}

std::string parse_word(std::string s){
    std::string res;
    for(char c:s){
        if(isalpha(c) || c=='-'){
            res += tolower(c);
        }
    }
    return res;
}

void build_inverted_index(){
    std::fstream fs;
    fs.open("docs.txt",std::ios::in);
    if(fs.is_open()){
        std::string doc;
        std::stringstream ss;
        int line_num=1;
        while(getline(fs,doc)){
            ss.str(doc);
            std::string word;
            while(ss>>word) inverted_index[parse_word(word)].push_back(line_num);
            line_num++;
            ss.clear();
        }
        docs_num=line_num-1;
        fs.close();
    }
    fs.open("inverted.txt",std::ios::out);
    for(auto& i:inverted_index){
        fs<<i.first<<": ";
        for(auto& j:i.second) fs<<j<<" ";
        fs<<std::endl;
    }
}

std::vector<int> AND_(std::vector<int> a,std::vector<int> b){
    std::vector<int> res;
    int i=0,j=0;
    while(i<a.size() && j<b.size()){
        if(a[i]==b[j]){
            res.push_back(a[i]);
            i++;
            j++;
        }
        else if(a[i]<b[j]) {i++;skipNum++;}
        else {j++;skipNum++;}
    }
    return res;
}

std::vector<int> AND(std::vector<int> a,std::vector<int> b){
    std::vector<int> res;
    int skip_a=std::pow(a.size(),0.5);
    int skip_b=std::pow(b.size(),0.5);
    // int skip_a=3;
    // int skip_b=3;
    int i=0,j=0;
    while(i<a.size() && j<b.size()){
        if(a[i]==b[j]){
            res.push_back(a[i]);
            i++;
            j++;
        }
        else if(a[i]<b[j]){
            if((i+skip_a)<a.size() && a[i+skip_a]<b[j]){
                while((i+skip_a)<a.size() && a[i+skip_a]<b[j]){
                    i+=skip_a;
                    skipNum++;
                }
            }
            else while(a[i]<b[j]) i++;
        }
        else{
            if((j+skip_b)<b.size() && b[j+skip_b]<a[i]){
                while((j+skip_b)<b.size() && b[j+skip_b]<a[i]){
                    j+=skip_b;
                    skipNum++;
                }
            }
            else while(b[j]<a[i]) j++;
        }
    }
    return res;
}

std::vector<int> OR(std::vector<int> a,std::vector<int> b){
    std::vector<int> res;
    int i=0,j=0;
    while(i<a.size() && j<b.size()){
        if(a[i]==b[j]){
            res.push_back(a[i]);
            i++;
            j++;
        }
        else if(a[i]<b[j]){
            res.push_back(a[i]);
            i++;
        }
        else{
            res.push_back(b[j]);
            j++;
        }
    }
    while(i<a.size()){
        res.push_back(a[i]);
        i++;
    }
    while(j<b.size()){
        res.push_back(b[j]);
        j++;
    }
    return res;
}

std::vector<int> NOT(std::vector<int> a){
    std::vector<int> res;
    int k=0;
    for(int i=0;i<docs_num;i++){
        if(a[k]!=(i+1)) res.push_back(i+1);
        else k++;
    }
    return res;
}

int main(){
    build_inverted_index();

    std::vector<int> l_federated = inverted_index["federated"];

    std::vector<int> l_recommendation = inverted_index["recommendation"];
    std::vector<int> l_transfer = inverted_index["transfer"];
    std::vector<int> l_learning = inverted_index["learning"];
    std::vector<int> l_filtering = inverted_index["filtering"];
    std::vector<int> l_feedback = inverted_index["feedback"];

    std::vector<int> q1 = AND(l_federated ,l_recommendation);
    std::vector<int> q2 = AND(AND(l_transfer,l_learning),l_filtering);
    std::vector<int> q3 = AND(l_recommendation,l_feedback);
    std::vector<int> q4 = OR(l_recommendation,l_filtering);
    std::vector<int> q5 = AND(l_transfer,NOT(q4));


    show_list(q1);
    show_list(q2);
    show_list(q3);
    show_list(q4);
    show_list(q5);

    std::cout<<skipNum<<std::endl;

}
