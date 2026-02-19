#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define W 150
#define H 55

#define NORMAL 0
#define SHINY 1
#define HOLE 2

#define XCONST 0
#define YCONST 1
#define ZCONST 2

// angles (A -> x-axis | B -> y-axis | C -> z-axis)
float A = 0, B = 0, C = 0;

const int cube_width = 50; // how big the cube will look
float z_buf[W * H];     // stores z values of points (for depth perception effects)
char buf[W * H];        // stores characters to print
char render_buf[(W + 1) * H + 1]; // this string will be printed (1 is added for '\n's and '\0')
int bg = ' ';           // background
float spacing = 0.5;

const float diameter = cube_width * 0.75; 
const float radius = diameter/2;
const float heartsize = cube_width * 0.25;
float camera_dist = 90; // self-explanatory.
float z1 = 40;  // essentially z', used in projection formula
float ooz;      // one over z

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

void calculate_point(float i, float j, float k, float nx, float ny, float nz, float type) {
    float x = calcX(i, j, k);
    float y = calcY(i, j, k);
    float z = calcZ(i, j, k) + camera_dist;

    // calculating rotated normal coordinates
    float rnx = calcX(nx, ny, nz); 
    float rny = calcY(nx, ny, nz);
    float rnz = calcZ(nx, ny, nz);

    // value for how much light is hitting the surface (between 0 and 1)
    float mag = sqrt(lightsource.x*lightsource.x + lightsource.y*lightsource.y + lightsource.z*lightsource.z);
    float luminance = (rnx*lightsource.x + rny*lightsource.y + rnz*lightsource.z)/mag;

    float ooz = 1/z;

    // W/2 and H/2 are added so that the cube stays centered

    float xp = round(W/2 + z1 * x * ooz * 2); // 2 is multiplied because the height of ASCII characters is usually 2x their width
    float yp = round(H/2 + z1 * y * ooz);

    int idx = xp + yp * W;  // index calculation. 

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

void* render_faces(void* args) {
	float* p = (float*)args;
	
	/* 
	p = {start_index, end_index, shading_type, <--- common for both sides
		 fixed_value1, normal_x1, normal_y1, mode1, <--- first face args	
		 fixed_value2, normal_x2, normal_y2, mode2} <--- second face args
	*/
	
	float start_i = p[0];
    float end_i = p[1];
    float type = p[2];

    float fixed_1 = p[3];
    float nx1 = p[4], ny1 = p[5], nz1 = p[6];
    int mode1 = (int)p[7];
	
    float fixed_2 = p[8];
    float nx2 = p[9], ny2 = p[10], nz2 = p[11];
    int mode2 = (int)p[12];
	
	for (float i = start_i; i <= end_i; i += spacing) {
        for (float j = -cube_width/2; j <= cube_width/2; j += spacing) {
            
            if (mode1 == XCONST) {
            	calculate_point(fixed_1, j, i, nx1, ny1, nz1, type);
            }
            else if (mode1 == YCONST) {
            	calculate_point(i, fixed_1, j, nx1, ny1, nz1, type);
            }
            else if (mode1 == ZCONST) {
            	calculate_point(i, j, fixed_1, nx1, ny1, nz1, type);
			}
			
            if (mode2 == XCONST) {
            	calculate_point(fixed_2, j, i, nx2, ny2, nz2, type);
            }
            else if (mode2 == YCONST) {
            	calculate_point(i, fixed_2, j, nx2, ny2, nz2, type);
            }
            else if (mode2 == ZCONST) {
            	calculate_point(i, j, fixed_2, nx2, ny2, nz2, type);
        	}
        }
    }
	
	return NULL;
}

int main() {
	
	float front_back_args[] = {
		-cube_width/2, cube_width/2, NORMAL, 
		-cube_width/2, 1, 0, 0, ZCONST,   
		 cube_width/2, 0, 0, 1, ZCONST    
	};
	
	float left_right_args[] = {
		-cube_width/2, cube_width/2, NORMAL, 
		 cube_width/2, -1, 0, 0, XCONST,
		-cube_width/2,  0, 0, -1, XCONST
	};
	
	float top_bottom_args[] = {
		-cube_width/2, cube_width/2, NORMAL, 
		-cube_width/2, 0, -1, 0, YCONST,
		 cube_width/2, 0, 1, 0, YCONST
	};
	
    printf("\x1b[2J"); // ANSI code to clear terminal
	
	
    while (1) {

        // clearing both buf and z_buf
        memset(buf, bg, W * H * sizeof(char)); 
        memset(z_buf, 0, W * H * sizeof(float));

		pthread_t threads[3];
		
		pthread_create(&threads[0], NULL, render_faces, front_back_args);
		pthread_create(&threads[1], NULL, render_faces, left_right_args);
		pthread_create(&threads[2], NULL, render_faces, top_bottom_args);
		
		for (int i = 0; i < 3; i++) {
			pthread_join(threads[i], NULL);
		}

        // putting contents of buf[] into render_buf[]

        int k = 0;
        for (int j = 0; j < H; j++) {
            for (int i = 0; i < W; i++) {
                render_buf[k++] = buf[i + j*W];
            }
            render_buf[k++] = '\n';
        }
        render_buf[k] = '\0';

        printf("\x1b[H"); // ANSI code to tell the cursor to return to the start position

        // printing each frame at once (much more faster than simply iterating over buf[] and printing each char sequentially)
        fwrite(render_buf, 1, k, stdout);

        // changing angles so that the cube rotates
        A += 0.1;
        B += 0.1;
        C += 0.01;

        usleep(8000 * 2); // <--- uncomment this to make the animation have a constant framerate (non-windows)
        
        //Sleep(1000/60); // <--- uncomment this to make the animation have a constant framerate (windows)

    }
    return 0;
}
