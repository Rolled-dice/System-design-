#include <iostream>
#include <string>

class Base62 {
    static constexpr const char* ALPHABET =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
public:
    static std::string encode(uint64_t n) {
        if (n == 0) return "0";
        std::string s;
        while (n > 0) {
            s += ALPHABET[n % 62];
            n /= 62;
        }
        return std::string(s.rbegin(), s.rend());
    }

    static uint64_t decode(const std::string& s) {
        uint64_t n = 0;
        for (char c : s) {
            n *= 62;
            if (c >= '0' && c <= '9')      n += c - '0';
            else if (c >= 'A' && c <= 'Z') n += c - 'A' + 10;
            else if (c >= 'a' && c <= 'z') n += c - 'a' + 36;
        }
        return n;
    }
};

int main() {
    for (uint64_t n : {0ULL, 1ULL, 61ULL, 62ULL, 1000000ULL, 3521614606207ULL}) {
        auto enc = Base62::encode(n);
        auto dec = Base62::decode(enc);
        std::cout << n << " -> " << enc << " -> " << dec << "\n";
    }
    return 0;
}
