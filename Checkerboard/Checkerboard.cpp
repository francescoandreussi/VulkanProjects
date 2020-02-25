#define cimg_display 0
#define cimg_use_tif

#define WIDTH 1920
#define HEIGHT 1080
#define RAN_LEN 10

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <math.h>
#include <CImg.h>
#include <ImageMagick-7/Magick++.h>

using namespace cimg_library;

class Checkerboard {
private:
    /* data */
public:
    Checkerboard(/* args */);
    ~Checkerboard();

    static void checkers();
};

void Checkerboard::checkers(){
    CImg<u_short> checkers(WIDTH, HEIGHT, 1, 3);
    u_int val = 0;
    for (int i = 0; i < HEIGHT; i = i + RAN_LEN) {
        for (int j = 0; j < WIDTH; j++) {
            u_short colour = val % 2;
            for (int k = 0; k < RAN_LEN; k++) {
                if ( ((j-960)*(j-960) + (i+k-540)*(i+k-540)) < (540*540) ) {
                    checkers(j, i + k, 0, 0) = colour * 255;
                    checkers(j, i + k, 0, 1) = colour * 255;
                    checkers(j, i + k, 0, 2) = colour * 255;
                } else {
                    checkers(j, i + k, 0, 0) = 0;
                    checkers(j, i + k, 0, 1) = 0;
                    checkers(j, i + k, 0, 2) = 0;
                }
            }
            std::cout << i << " " << j << " " << colour << std::endl;
            if (j % RAN_LEN == 0 && j != 0) {
                val++;
            }
        }
        //val++;
    }

    checkers.save("checkerboard.png");
}

int main(int argc, char* argv[]){
    Checkerboard::checkers();

    return 0;
}
