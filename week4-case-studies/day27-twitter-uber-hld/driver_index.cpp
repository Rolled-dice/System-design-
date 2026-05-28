#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

struct GeoPoint { double lat, lng; };

class DriverIndex {
    std::unordered_map<std::string, GeoPoint> drivers_;

    static double haversineKm(double lat1, double lng1, double lat2, double lng2) {
        constexpr double R = 6371.0;
        auto rad = [](double d){ return d * M_PI / 180.0; };
        double dLat = rad(lat2 - lat1);
        double dLng = rad(lng2 - lng1);
        double a = std::sin(dLat/2) * std::sin(dLat/2) +
                   std::cos(rad(lat1)) * std::cos(rad(lat2)) *
                   std::sin(dLng/2) * std::sin(dLng/2);
        return R * 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    }
public:
    void update(const std::string& id, double lat, double lng) {
        drivers_[id] = {lat, lng};
    }
    void remove(const std::string& id) { drivers_.erase(id); }

    std::vector<std::pair<std::string, double>> nearby(double lat, double lng, double radiusKm) {
        std::vector<std::pair<std::string, double>> out;
        for (const auto& [id, p] : drivers_) {
            double d = haversineKm(lat, lng, p.lat, p.lng);
            if (d <= radiusKm) out.emplace_back(id, d);
        }
        std::sort(out.begin(), out.end(), [](auto& a, auto& b){ return a.second < b.second; });
        return out;
    }
};

int main() {
    DriverIndex idx;
    idx.update("d1", 37.7749, -122.4194);
    idx.update("d2", 37.7750, -122.4180);
    idx.update("d3", 37.8000, -122.4000);
    idx.update("d4", 40.7128, -74.0060);

    auto r = idx.nearby(37.7749, -122.4194, 5.0);
    std::cout << "Drivers within 5km of SF center:\n";
    for (const auto& [id, dist] : r)
        std::cout << "  " << id << " @ " << dist << "km\n";
    return 0;
}
