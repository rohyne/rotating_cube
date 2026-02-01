#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#define W 80
#define H 45

// angles (A -> x-axis | B -> y-axis | C -> z-axis)
float A = 0, B = 0, C = 0;

const int cube_width = 40; // how big the cube will look
float z_buf[W * H];     // stores z values of points (for depth perception effects)
char buf[W * H];        // stores characters to print
int bg = ' ';           // background

float x, y, z;  // coordinates for each vertex
float xp, yp;   // projected x and y coordinates 
float camera_dist = 100; // self-explanatory.
float z1 = 40;  // essentially z', used in projection formula
float ooz;      // one over z
int idx;        // cell index   

/* 
    === euler rotation coordinates ===

    derived from the matrix equation (x, y, z) = R_x(A) * R_y(B) * R_z(C) * (i, j, k)

    where R_x(A), R_y(B), R_z(C) are 3-d rotation matrices

    =================================
*/

float calcX(float i, float j, float k) {
  return j * sin(A) * sin(B) * cos(C) - k * cos(A) * sin(B) * cos(C) +
         j * cos(A) * sin(C) + k * sin(A) * sin(C) + i * cos(B) * cos(C);
}

float calcY(float i, float j, float k) {
  return j * cos(A) * cos(C) + k * sin(A) * cos(C) -
         j * sin(A) * sin(B) * sin(C) + k * cos(A) * sin(B) * sin(C) -
         i * cos(B) * sin(C);
}

float calcZ(float i, float j, float k) {
  return k * cos(A) * cos(B) - j * sin(A) * cos(B) + i * sin(B);
}

// takes the rotated coordinates and applies z buffering + loads in the character

void calculatepoint(float i, float j, float k, int ch) {
    x = calcX(i, j, k);
    y = calcY(i, j, k);
    z = calcZ(i, j, k) + camera_dist;

    ooz = 1/z;

    // W/2 and H/2 are added so that the cube stays centered

    xp = round(W/2 + z1 * x * ooz * 2); // 2 is multiplied because the height of ASCII characters is usually 2x their width
    yp = round(H/2 + z1 * y * ooz);

    idx = xp + yp * W;  // index calculation. 

    if (idx >= 0 && idx < W * H) {
        if (ooz > z_buf[idx]) {
            z_buf[idx] = ooz;
            buf[idx] = ch;
        }
    }

}

int main() {
    printf("\x1b[2J"); // ANSI code to clear terminal

    while (1) {

        // clearing both buf and z_buf
        memset(buf, bg, W * H * sizeof(char)); 
        memset(z_buf, 0, W * H * sizeof(float));

        // setting up contents of buf[] for each frame
        for (float i = -cube_width/2; i < cube_width/2; i += 0.15) {
            for (float j = -cube_width/2; j < cube_width/2; j += 0.15) {
                calculatepoint(i, j, cube_width/2, '*');
                calculatepoint(-cube_width/2, j, i, '%');
                calculatepoint(-i, j, -cube_width/2, '#');
                calculatepoint(cube_width/2, j, -i, '%');
                calculatepoint(i, cube_width/2, -j, '@');
                calculatepoint(i, -cube_width/2, j,  '&');
            }
        }

        printf("\x1b[H"); // ANSI code to tell the cursor to return to the start position

        // printing contents of buf[]
        for (int i = 0; i < W * H; i++) {
            putchar(i % W ? buf[i] : 10);
        }


        // changing angles so that the cube rotates
        A += 0.1;
        B += 0.1;
        C += 0.1;

        //usleep(8000); // <--- uncomment this to slow down the animation
    }

    return 0;
}



