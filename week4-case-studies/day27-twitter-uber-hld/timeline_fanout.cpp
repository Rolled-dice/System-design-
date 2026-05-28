#include <deque>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class TwitterSim {
    static constexpr size_t MAX_TIMELINE = 800;
    static constexpr size_t CELEB_THRESHOLD = 100000;

    std::unordered_map<std::string, std::unordered_set<std::string>> followers_;
    std::unordered_map<std::string, std::unordered_set<std::string>> following_;
    std::unordered_map<std::string, std::deque<std::string>> userTweets_;
    std::unordered_map<std::string, std::deque<std::string>> homeTimeline_;
    long tweetCounter_ = 0;
    int fanoutWrites_ = 0;

public:
    void follow(const std::string& a, const std::string& b) {
        following_[a].insert(b);
        followers_[b].insert(a);
    }

    bool isCeleb(const std::string& u) const {
        auto it = followers_.find(u);
        return it != followers_.end() && it->second.size() >= CELEB_THRESHOLD;
    }

    std::string post(const std::string& user, const std::string& text) {
        std::string tid = "t" + std::to_string(++tweetCounter_) + ":" + user + ":" + text;
        auto& userQ = userTweets_[user];
        userQ.push_front(tid);
        if (userQ.size() > MAX_TIMELINE) userQ.pop_back();

        if (isCeleb(user)) {
            std::cout << "[push skipped: " << user << " is celeb]\n";
            return tid;
        }
        for (const auto& f : followers_[user]) {
            auto& tl = homeTimeline_[f];
            tl.push_front(tid);
            if (tl.size() > MAX_TIMELINE) tl.pop_back();
            ++fanoutWrites_;
        }
        return tid;
    }

    std::vector<std::string> homeTimeline(const std::string& user, size_t limit = 10) {
        std::vector<std::string> out;
        const auto& precomp = homeTimeline_[user];
        for (const auto& t : precomp) {
            out.push_back(t);
            if (out.size() >= limit) break;
        }
        for (const auto& celeb : following_[user]) {
            if (!isCeleb(celeb)) continue;
            for (const auto& t : userTweets_[celeb]) {
                out.push_back(t + " [pull-celeb]");
                if (out.size() >= limit) break;
            }
            if (out.size() >= limit) break;
        }
        return out;
    }

    int fanoutWrites() const { return fanoutWrites_; }
};

int main() {
    TwitterSim tw;
    tw.follow("alice", "bob");
    tw.follow("alice", "celebrity");
    tw.follow("dave",  "bob");

    for (int i = 0; i < 100001; ++i) tw.follow("fake" + std::to_string(i), "celebrity");

    tw.post("bob", "hello world");
    tw.post("celebrity", "I dropped a new album");
    tw.post("bob", "good morning");

    std::cout << "fanout writes: " << tw.fanoutWrites() << " (celeb skipped)\n";

    std::cout << "alice's timeline:\n";
    for (const auto& t : tw.homeTimeline("alice", 5)) std::cout << "  " << t << "\n";
    return 0;
}
