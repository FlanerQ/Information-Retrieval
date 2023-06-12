#include <iostream>
#include <chrono> // for std::chrono::system_clock
#include <ctime> // for std::tm and std::time_t
#include<iomanip>
int main() {
    std::string input_date = "2022-02-10"; // 待比较的日期字符串
    std::tm tm = {};
    std::istringstream ss(input_date);
    ss >> std::get_time(&tm, "%Y-%m-%d"); // 将日期字符串解析为 tm 结构体

    std::time_t t = std::mktime(&tm); // 将 tm 转换为 time_t 类型
    auto timestamp_input = std::chrono::system_clock::from_time_t(t); // 将 time_t 转换为 system_clock::time_point 类型

    auto now = std::chrono::system_clock::now(); // 获取当前时间
    auto timestamp_now = std::chrono::system_clock::to_time_t(now); // 将当前时间转换为 time_t 类型
    auto days_since_input = std::chrono::duration_cast<std::chrono::hours>(now - timestamp_input).count() / 24; // 计算指定日期与当前日期之间的天数间隔

    std::cout << "Days since " << input_date << ": " << days_since_input << std::endl;

    return 0;
}
