#include <iostream>

class Amplifier  { public: void on() { std::cout << "Amp on\n"; }     void off() { std::cout << "Amp off\n"; } };
class DVDPlayer  { public: void on() { std::cout << "DVD on\n"; }     void off() { std::cout << "DVD off\n"; } void play(const std::string& m) { std::cout << "Playing " << m << "\n"; } };
class Projector  { public: void on() { std::cout << "Proj on\n"; }    void off() { std::cout << "Proj off\n"; } };
class Lights     { public: void dim()  { std::cout << "Lights dim\n"; } void bright() { std::cout << "Lights bright\n"; } };

class HomeTheaterFacade {
    Amplifier amp_;
    DVDPlayer dvd_;
    Projector proj_;
    Lights    lights_;
public:
    void watchMovie(const std::string& movie) {
        std::cout << "-- get ready --\n";
        lights_.dim();
        proj_.on();
        amp_.on();
        dvd_.on();
        dvd_.play(movie);
    }
    void endMovie() {
        std::cout << "-- shutting down --\n";
        dvd_.off();
        amp_.off();
        proj_.off();
        lights_.bright();
    }
};

int main() {
    HomeTheaterFacade th;
    th.watchMovie("Inception");
    th.endMovie();
    return 0;
}
