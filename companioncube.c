#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define W 150
#define H 55

#define NORMAL 0
#define SHINY 1
#define HOLE 2

// angles (A -> x-axis | B -> y-axis | C -> z-axis)
float A = 0, B = 0, C = 0;

const int cube_width = 50; // how big the cube will look
float z_buf[W * H];     // stores z values of points (for depth perception effects)
char buf[W * H];        // stores characters to print
char render_buf[(W + 1) * H + 1]; // this string will be printed (1 is added for '\n's and '\0')
int bg = ' ';           // background
float spacing = 0.5;

float x, y, z;  // coordinates for each vertex
float xp, yp;   // projected x and y coordinates 
float rnx, rny, rnz; // rotated normal coordinates
float luminance; 
const float diameter = cube_width * 0.75; 
const float radius = diameter/2;
const float heartsize = cube_width * 0.25;
float camera_dist = 90; // self-explanatory.
float z1 = 40;  // essentially z', used in projection formula
float ooz;      // one over z
int idx;        // cell index   

char shades[] = ".,-~:;=!*#$@"; // shades (darkest to brightest)
char shines[] = "@$#*!=;:~`,."; 
int shadelen = sizeof(shades)/sizeof(char);

typedef struct {
    float x, y, z;
} point;

point lightsource = {100, 100, -100};

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

// takes the rotated coordinates and applies shading + loads chars into buf[]

void calculatepoint(float i, float j, float k, float nx, float ny, float nz, int type) {
    x = calcX(i, j, k);
    y = calcY(i, j, k);
    z = calcZ(i, j, k) + camera_dist;

    // calculating rotated normal coordinates
    rnx = calcX(nx, ny, nz); 
    rny = calcY(nx, ny, nz);
    rnz = calcZ(nx, ny, nz);

    // value for how much light is hitting the surface (between 0 and 1)
    float mag = sqrt(lightsource.x*lightsource.x + lightsource.y*lightsource.y + lightsource.z*lightsource.z);
    luminance = (rnx*lightsource.x + rny*lightsource.y + rnz*lightsource.z)/mag;

    ooz = 1/z;

    // W/2 and H/2 are added so that the cube stays centered

    xp = round(W/2 + z1 * x * ooz * 2); // 2 is multiplied because the height of ASCII characters is usually 2x their width
    yp = round(H/2 + z1 * y * ooz);

    idx = xp + yp * W;  // index calculation. 

    if ((idx >= 0 && idx < W * H) && (ooz > z_buf[idx])) {
        if (type == NORMAL) {
            z_buf[idx] = ooz;
            
            int shade_idx = (int)((luminance)*(shadelen - 1)); // assigning a value between 0 and (shadelen - 1) to get a shade
            if (shade_idx < 0) shade_idx = 0; // darkest case
            if (shade_idx > shadelen - 1) shade_idx = shadelen - 1; // brightest case

            buf[idx] = shades[shade_idx];
        }
        if (type == SHINY) {
            z_buf[idx] = ooz;
            
            int shine_idx = (int)((luminance)*(shadelen - 1));
            if (shine_idx < 0) shine_idx = 0;
            if (shine_idx > shadelen - 1) shine_idx = shadelen - 1;

            buf[idx] = shines[shine_idx];
        }
        if (type == HOLE) {
            buf[idx] = bg;
        }
    }
}


int main() {


    printf("\x1b[2J"); // ANSI code to clear terminal

    while (1) {

        // clearing both buf and z_buf
        memset(buf, bg, W * H * sizeof(char)); 
        memset(z_buf, 0, W * H * sizeof(float));

        // loading chars into buf[] for each frame
    
        // sides of the cube
        for (float i = -cube_width/2; i <= cube_width/2; i += spacing) {
            for (float j = -cube_width/2; j <= cube_width/2; j += spacing) {
                calculatepoint(-i, j, -cube_width/2, 1, 0, 0, NORMAL);  // front    (+z)
                calculatepoint(i, j, cube_width/2, 0, 0, 1, NORMAL);    // back     (-z)
                calculatepoint(cube_width/2, j, -i, -1, 0, 0, NORMAL);  // right    (+x)
                calculatepoint(-cube_width/2, j, i, 0, 0, -1, NORMAL);  // left     (-x)
                calculatepoint(i, -cube_width/2, j,  0, -1, 0, NORMAL); // top      (+y)
                calculatepoint(i, cube_width/2, -j, 0, 1, 0, NORMAL);   // bottom   (-y)
            }
        }

        // circle on each face
        for (float i = -radius; i <= radius; i += spacing) {
            for (float j = -radius; j <= radius; j += spacing) {
                if (i*i + j*j <= radius*radius) { 
                    calculatepoint(-i, j, -(cube_width/2 + 0.1), 1, 0, 0, HOLE);
                    calculatepoint(i, j, (cube_width/2 + 0.1), 0, 0, 1, HOLE);
                    calculatepoint((cube_width/2 + 0.1), j, -i, -1, 0, 0, HOLE);
                    calculatepoint(-(cube_width/2 + 0.1), j, i, 0, 0, -1, HOLE);
                    calculatepoint(i, -(cube_width/2 + 0.1), j, 0, -1, 0, HOLE);
                    calculatepoint(i, (cube_width/2 + 0.1), -j, 0, 1, 0, HOLE);
                }
            }
        }
        
        // heart in each circle
        for (float i = -heartsize; i <= heartsize; i += spacing) {
            for (float j = -heartsize; j <= heartsize; j += spacing) { 
                float s = 2.0 / heartsize; 
                float x_h = i * s;
                float y_h = j * s;

                float term = (x_h * x_h + y_h * y_h - 1);
                if (term * term * term - x_h * x_h * y_h * y_h * y_h <= 0) {
                    calculatepoint(-i, -j, -(cube_width/2 + 0.1), 1, 0, 0, SHINY);
                    calculatepoint(i, -j, (cube_width/2 + 0.1), 0, 0, 1, SHINY);
                    calculatepoint((cube_width/2 + 0.1), -j, -i, -1, 0, 0, SHINY);
                    calculatepoint(-(cube_width/2 + 0.1), -j, i, 0, 0, -1, SHINY);
                    calculatepoint(i, -(cube_width/2 + 0.1), j, 0, -1, 0, SHINY);
                    calculatepoint(i, (cube_width/2 + 0.1), -j, 0, 1, 0, SHINY);
                }
            }
        }

        // putting contents of buf[] into render_buf[]

        int k = 0;
        for (int j = 0; j < H; j++) {
            for (int i = 0; i < W; i++) {
                render_buf[k++] = buf[i + j*W];
            }
            render_buf[k++] = '\n';
        }
        render_buf[k] == '\0';

        printf("\x1b[H"); // ANSI code to tell the cursor to return to the start position

        // printing each frame at once (much more faster than simply iterating over buf[] and printing each char sequentially)
        fwrite(render_buf, 1, k, stdout);

        // changing angles so that the cube rotates
        A += 0.1;
        B += 0.1;
        C += 0.01;

        //usleep(1000/60); // <--- uncomment this to make the animation have a constant framerate (non-windows)
        
        //Sleep(1000/60); // <--- uncomment this to make the animation have a constant framerate (windows)

    }
    return 0;
}