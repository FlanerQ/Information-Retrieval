#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>

int dis[30][30];//编辑距离矩阵

int distance(std::string a, std::string b) // 求编辑距离函数
{
    int la = a.length();
    int lb = b.length();
    std::memset(dis, 0, sizeof(dis));
    for (int i=1; i<=la; i++) dis[i][0]=i; //若b为空 则删除a
    for (int i=1; i<=lb; i++) dis[0][i]=i; //若a为空 则插入b
    for (int i=1; i<=la; i++)
        for (int j=1; j<=lb; j++){
            dis[i][j] = std::max(la, lb); //初始化 f[i][j]<=max(la, lb)
            dis[i][j] = std::min(dis[i][j], dis[i-1][j-1]+(int)(a[i-1]!=b[j-1])); //两字符不相等 则替换
            dis[i][j] = std::min(dis[i][j], dis[i-1][j]+1); // 在a中删除一个字符
            dis[i][j] = std::min(dis[i][j], dis[i][j-1]+1); // 在a中插入一个字符
        }
    return dis[la][lb]; //返回编辑距离
}

int main()
{
    std::string s1, s2;
    while (std::cin >> s1 >> s2) //输入
        std::cout << s1 << " " << s2 << " " << distance(s1, s2) << std::endl << std::endl; //输出
        // std::cout<<distance(s1,s2)<<std::endl;
}


