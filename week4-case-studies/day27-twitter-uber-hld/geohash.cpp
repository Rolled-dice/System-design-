#include <iostream>
#include <string>

class GeoHash {
    static constexpr const char* BASE32 = "0123456789bcdefghjkmnpqrstuvwxyz";
public:
    static std::string encode(double lat, double lng, int precision = 6) {
        double latLo = -90,  latHi = 90;
        double lngLo = -180, lngHi = 180;
        std::string hash;
        int bits = 0, bitVal = 0;
        bool isLng = true;

        while ((int)hash.size() < precision) {
            if (isLng) {
                double mid = (lngLo + lngHi) / 2;
                if (lng >= mid) { bitVal = (bitVal << 1) | 1; lngLo = mid; }
                else            { bitVal = (bitVal << 1);     lngHi = mid; }
            } else {
                double mid = (latLo + latHi) / 2;
                if (lat >= mid) { bitVal = (bitVal << 1) | 1; latLo = mid; }
                else            { bitVal = (bitVal << 1);     latHi = mid; }
            }
            isLng = !isLng;
            if (++bits == 5) {
                hash += BASE32[bitVal];
                bits = 0; bitVal = 0;
            }
        }
        return hash;
    }
};

int main() {
    auto h1 = GeoHash::encode(37.7749, -122.4194, 7);
    auto h2 = GeoHash::encode(37.7750, -122.4195, 7);
    auto h3 = GeoHash::encode(40.7128, -74.0060,  7);

    std::cout << "SF1:  " << h1 << "\n";
    std::cout << "SF2:  " << h2 << " (close to SF1, shares prefix)\n";
    std::cout << "NYC:  " << h3 << " (very different)\n";
    return 0;
}
