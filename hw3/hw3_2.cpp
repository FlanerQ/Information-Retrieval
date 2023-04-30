
#include <bitset>
#include <iostream>
#include <vector>

using namespace std;

// VB encode
std::vector<uint8_t> vb_encode(uint32_t value) {
    std::vector<uint8_t> encoded;
    while (value >= 0x80) {
        encoded.push_back((value & 0x7F) | 0x80);
        value >>= 7;
    }
    encoded.push_back(value);
    return encoded;
}

// VB decode
uint32_t vb_decode(const std::vector<uint8_t> &encoded) {
    uint32_t value = 0;
    for (size_t i = 0; i < encoded.size(); i++) {
        value |= (encoded[i] & 0x7F) << (7 * i);
        if ((encoded[i] & 0x80) == 0) {
            break;
        }
    }
    return value;
}

void print(uint32_t value) {
    cout << "original " << value << endl;
    std::vector<uint8_t> encoded = vb_encode(value);
    uint32_t decoded = vb_decode(encoded);
    cout << "encoded ";
    for (auto &i : encoded)
        cout << bitset<sizeof(i) * 8>(i) << " ";
    cout << endl;
    cout << "decoded " << decoded << endl << endl;
}

int main() {
    print(722);
    print(936);
    print(724);
    return 0;
}