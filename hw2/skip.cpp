#include <iostream>
#include <map>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>

using namespace std;

// Define a struct for skip pointers
struct SkipPointer {
    int value;
    SkipPointer* next;
};

void show_list(const std::vector<int> &l){
    for(auto i:l) std::cout<<i<<" ";
    std::cout<<std::endl;
}

// Define a function to create skip pointers for a given postings list
SkipPointer* createSkipPointers(int* postingsList, int length, int skipInterval) {
    SkipPointer* head = new SkipPointer();
    head->value = postingsList[0];
    head->next = nullptr;
    SkipPointer* current = head;
    int i = skipInterval;
    while (i < length) {
        SkipPointer* skip = new SkipPointer();
        skip->value = postingsList[i];
        skip->next = nullptr;
        current->next = skip;
        current = skip;
        i += skipInterval;
    }
    return head;
}

// Define a function to perform intersection using skip pointers
vector<int> intersectWithSkipPointers(int* list1, int length1, int* list2, int length2, int skipInterval) {
    vector<int> result;
    int i = skipInterval;
    int j = skipInterval;
    while (i < length1 && j < length2) {
        if (list1[i] == list2[j]) {
            result.push_back(list1[i]);
            i += skipInterval;
            j += skipInterval;
        } else if (list1[i] < list2[j]) {
            i += skipInterval;
        } else {
            j += skipInterval;
        }
    }
    return result;
}

int main(){
    // Call intersectWithSkipPointers with appropriate arguments to perform intersection using skip pointers
int list1[] = {1, 3, 4, 7, 9, 10, 11};
int length1 = sizeof(list1) / sizeof(list1[0]);
int list2[] = {2, 3, 4, 6, 7, 8, 10, 11};
int length2 = sizeof(list2) / sizeof(list2[0]);
int skipInterval = 2;
vector<int> result = intersectWithSkipPointers(list1, length1, list2, length2, skipInterval);
show_list(result);
}