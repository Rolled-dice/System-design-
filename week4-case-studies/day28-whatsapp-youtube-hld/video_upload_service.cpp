#include <algorithm>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

enum class VideoStatus { UPLOADING, ENCODING, READY, FAILED };

struct Video {
    std::string id;
    std::string title;
    size_t totalChunks = 0;
    std::vector<bool> chunks;
    VideoStatus status = VideoStatus::UPLOADING;
    std::vector<std::string> resolutions;
};

class VideoUploadService {
    std::unordered_map<std::string, Video> videos_;
    std::queue<std::string> encodingQueue_;
    int counter_ = 1;

    static const char* name(VideoStatus s) {
        switch (s) {
            case VideoStatus::UPLOADING: return "UPLOADING";
            case VideoStatus::ENCODING:  return "ENCODING";
            case VideoStatus::READY:     return "READY";
            case VideoStatus::FAILED:    return "FAILED";
        }
        return "?";
    }
public:
    std::string initUpload(const std::string& title, size_t numChunks) {
        std::string id = "v" + std::to_string(counter_++);
        videos_[id] = {id, title, numChunks, std::vector<bool>(numChunks, false)};
        std::cout << "[upload] init " << id << " (" << numChunks << " chunks)\n";
        return id;
    }

    void uploadChunk(const std::string& id, size_t idx) {
        auto& v = videos_[id];
        if (idx >= v.totalChunks) return;
        v.chunks[idx] = true;
        size_t done = std::count(v.chunks.begin(), v.chunks.end(), true);
        std::cout << "[upload] " << id << " chunk " << idx << " (" << done << "/" << v.totalChunks << ")\n";
        if (done == v.totalChunks) {
            v.status = VideoStatus::ENCODING;
            encodingQueue_.push(id);
            std::cout << "[upload] " << id << " all chunks received -> queued for encoding\n";
        }
    }

    void runEncodingWorker() {
        while (!encodingQueue_.empty()) {
            std::string id = encodingQueue_.front(); encodingQueue_.pop();
            auto& v = videos_[id];
            std::cout << "[encode] " << id << " transcoding...\n";
            for (const auto& res : {"144p", "360p", "720p", "1080p"})
                v.resolutions.push_back(res);
            v.status = VideoStatus::READY;
            std::cout << "[encode] " << id << " READY\n";
        }
    }

    void status(const std::string& id) {
        const auto& v = videos_[id];
        std::cout << v.id << " '" << v.title << "' [" << name(v.status) << "] resolutions:";
        for (const auto& r : v.resolutions) std::cout << " " << r;
        std::cout << "\n";
    }
};

int main() {
    VideoUploadService svc;
    auto id = svc.initUpload("My Trip Vlog", 4);
    for (size_t i = 0; i < 4; ++i) svc.uploadChunk(id, i);
    svc.runEncodingWorker();
    svc.status(id);
    return 0;
}
