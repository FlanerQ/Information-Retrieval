#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <dirent.h>
#include <algorithm>
#include <map>

using namespace std;

// Node for linked list
struct Node {
    string word;
    vector<string> files;
    Node* next;
};

// Inverted index class
class InvertedIndex {
public:
    Node* head;

public:
    InvertedIndex() {
        head = NULL;
    }

    // Insert a word and its corresponding file into the inverted index
    void insert(string word, string file) {
        Node* curr = head;
        Node* prev = NULL;

        // Traverse the linked list to find the correct position to insert the word
        while (curr != NULL && curr->word < word) {
            prev = curr;
            curr = curr->next;
        }

        // If the word already exists in the linked list, add the file to its vector
        if (curr != NULL && curr->word == word) {
            curr->files.push_back(file);
        }
        // Otherwise, create a new node for the word and add the file to its vector
        else {
            Node* newNode = new Node;
            newNode->word = word;
            newNode->files.push_back(file);
            newNode->next = curr;

            // If the new node is the head of the linked list, update the head
            if (prev == NULL) {
                head = newNode;
            }
            // Otherwise, insert the new node into the linked list
            else {
                prev->next = newNode;
            }
        }
    }

    // Search for a word in the inverted index and return its corresponding vector of files
    vector<string> search(string word) {
        Node* curr = head;

        // Traverse the linked list to find the node for the word
        while (curr != NULL && curr->word < word) {
            curr = curr->next;
        }

        // If the node for the word is found, return its vector of files
        if (curr != NULL && curr->word == word) {
            return curr->files;
        }
        // Otherwise, return an empty vector
        else {
            vector<string> empty;
            return empty;
        }
    }

    // Perform an AND operation between two inverted indices
    InvertedIndex operator&(const InvertedIndex& other) const {
        InvertedIndex result;
        Node* curr1 = head;
        Node* curr2 = other.head;

        // Traverse both linked lists and add files to the result index if they exist in both
        while (curr1 != NULL && curr2 != NULL) {
            // std::cout << curr1->word << " - " << curr2->word << std::endl;
            if (curr1->word == curr2->word) {
                for (string file : curr1->files) {
                    result.insert(curr1->word, file);
                }
                curr1 = curr1->next;
                curr2 = curr2->next;
                break;
            }
            else if (curr1->word < curr2->word) {
                curr1 = curr1->next;
            }
            else {
                curr2 = curr2->next;
            }
        }

        return result;
        // Return the result index
    }
    // Perform an OR operation between two inverted indices
    InvertedIndex operator|(const InvertedIndex& other) const {
        InvertedIndex result;
        Node* curr1 = head;
        Node* curr2 = other.head;

        // Traverse both linked lists and add files to the result index if they exist in either
        while (curr1 != NULL || curr2 != NULL) {
            if (curr1 != NULL && curr2 != NULL && curr1->word == curr2->word) {
                for (string file : curr1->files) {
                    result.insert(curr1->word, file);
                }
                for (string file : curr2->files) {
                    if (find(curr1->files.begin(), curr1->files.end(), file) == curr1->files.end()) {
                        result.insert(curr2->word, file);
                    }
                }
                curr1 = curr1->next;
                curr2 = curr2->next;
            }
            else if (curr1 != NULL && (curr2 == NULL || curr1->word < curr2->word)) {
                for (string file : curr1->files) {
                    result.insert(curr1->word, file);
                }
                curr1 = curr1->next;
            }
            else {
                for (string file : curr2->files) {
                    result.insert(curr2->word, file);
                }
                curr2 = curr2->next;
            }
        }

        return result;
        // Return the result index
    }
    // Perform a NOT operation on an inverted index
    InvertedIndex operator!(void) const {
        InvertedIndex result;
        Node* curr = head;

        // Traverse the linked list and add all files to the result index
        // that do not belong to the current word
        while (curr != NULL) {
            vector<string> all_files = get_all_files();
            for (string file : all_files) {
                if (find(curr->files.begin(), curr->files.end(), file) == curr->files.end()) {
                    result.insert(curr->word, file);
                }
            }
            curr = curr->next;
        }

        return result;
        // Return the result index
    }

    // Get a vector of all files in the inverted index
    vector<string> get_all_files() const {
        vector<string> all_files;
        Node* curr = head;

        // Traverse the linked list and add all files to the vector
        while (curr != NULL) {
            for (string file : curr->files) {
                if (find(all_files.begin(), all_files.end(), file) == all_files.end()) {
                    all_files.push_back(file);
                }
            }
            curr = curr->next;
        }

        return all_files;
        // Return the vector of all files
    }
    // Print all tokens in the inverted index
    void PrintTokens() {
        Node* curr = head;
        while (curr != NULL) {
            cout << curr->word << " ";
            curr = curr->next;
        }
    }


} inverted_index;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <folder_path>" << endl;
        return 1;
    }

    string folder_path = argv[1];

    // Open the folder and read each txt file
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(folder_path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            string filename = ent->d_name;
            if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".txt") {
                string filepath = folder_path + "/" + filename;
                std::cout << "open: " << filepath << std::endl;
                ifstream file(filepath);

                // Split the file into word tokens and add them to the inverted index
                string line;
                while (getline(file, line)) {
                    stringstream ss(line);
                    string word;
                    while (ss >> word) {
                        transform(word.begin(), word.end(), word.begin(), ::tolower);
                        word.erase(remove_if(word.begin(), word.end(), ::ispunct), word.end());
                        inverted_index.insert(word, filepath);
                    }
                }
            }
        }
        closedir(dir);
    }
    else {
        cout << "Error: could not open directory" << endl;
        return 1;
    }

    string query;
    cout << "Enter a query: ";
    getline(cin, query);

    vector<string> all_files = inverted_index.get_all_files();
    InvertedIndex result = inverted_index;

    // Split the query into keywords and perform the appropriate operations
    stringstream ss(query);
    string keyword;
    while (ss >> keyword) {
        transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
        cout << "keyword: " << keyword << endl;

        // If the keyword is NOT, perform a NOT operation on the result index
        if (keyword[0] == '!') {
            keyword = keyword.substr(1);
            InvertedIndex temp;
            temp.insert(keyword, "");
            result = (!temp & result);
        }
        // If the keyword is AND, perform an AND operation between the result index and the next index
        else if (keyword == "&") {
            InvertedIndex next;
            string next_keyword;
            ss >> next_keyword;
            transform(next_keyword.begin(), next_keyword.end(), next_keyword.begin(), ::tolower);
            InvertedIndex temp;
            temp.insert(next_keyword, "");
            result = (result & temp);
        }
        // If the keyword is OR, perform an OR operation between the result index and the next index
        else if (keyword == "|") {
            InvertedIndex next;
            string next_keyword;
            ss >> next_keyword;
            transform(next_keyword.begin(), next_keyword.end(), next_keyword.begin(), ::tolower);
            InvertedIndex temp;
            temp.insert(next_keyword, "");
            result = (result | temp);
        }
        // Otherwise, search for the keyword in the inverted index and add its corresponding files to the result index
        else {
            InvertedIndex temp;
            temp.insert(keyword, "");
            result = (result | temp);
        }
        // Print out all tokens from "result"
        result.PrintTokens();
    }

    std::cout << "result tokens: ";
    result.PrintTokens();
    std::cout << std:: endl;

    // Print the files in the result index
    cout << "Files containing the query \"" << query << "\":" << endl;
    for (string file : result.get_all_files()) {
        cout << file << endl;
    }

    return 0;
}
