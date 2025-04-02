#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define INTERVALS 5 // Pocet intervalu v histogramu
#define MAX_LIGHT 255 // Maximalni hodnota jasu
#define MIN_LIGHT 0 // Minimalni hodnota jasu
#define TILE_SIZE 64 // velikost bloku proi zpracovani obrazu

const int kernel[3][3] = {
    {  0, -1,  0 },
    { -1,  5, -1 },
    {  0, -1,  0 }
};

void apply_kernel(unsigned char *input, unsigned char *output, int width, int height) {
    memcpy(output, input, width * 3 * height);

    for (int y0 = 1; y0 < height - 1; y0 += TILE_SIZE) {
        for (int x0 = 1; x0 < width - 1; x0 += TILE_SIZE) {
            int y_max = fmin(y0 + TILE_SIZE, height - 1);
            int x_max = fmin(x0 + TILE_SIZE, width - 1);
            
            for (int y = y0; y < y_max; y++) {
                for (int x = x0; x < x_max; x++) {
                    for (int c = 0; c < 3; c++) {
                        int original_value = input[(y * width + x) * 3 + c];
                        printf("Original Pixel [%d, %d], Channel %d, Value: %d\n", x, y, c, original_value);
                        int new_value = 0;
                        for (int ky = -1; ky <= 1; ky++) {
                            for (int kx = -1; kx <= 1; kx++) {
                                new_value += input[((y + ky) * width + (x + kx)) * 3 + c] * kernel[ky + 1][kx + 1];
                                printf("Pixel [%d, %d], Channel %d, New value: %d\n", x, y, c, new_value);
                            }
                        }
                        output[(y * width + x) * 3 + c] = (unsigned char)fmax(0, fmin(255, new_value));
                    }
                }
            }
        }   
    }
}

void compute_histogram(const unsigned char *image, int width, int height, int *histogram) {
    memset(histogram, 0, INTERVALS * sizeof(int));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx_pixel = (y * width + x) * 3;
            int gray = round(0.2126 * image[idx_pixel] + 0.7152 * image[idx_pixel + 1] + 0.0722 * image[idx_pixel + 2]);
            int bin = (gray * INTERVALS) / (MAX_LIGHT + 1);
            histogram[bin]++;
        }
    }
}

void save_histogram(const int *histogram) {
    FILE *file = fopen("output.txt", "w");
    if (!file) {
        perror("Error opening output.txt");
        exit(1);
    }
    fprintf(file, "%d %d %d %d %d", histogram[0], histogram[1], histogram[2], histogram[3], histogram[4]);
    fclose(file);
}

void process_image(const char *filename) {
    FILE *img_file = fopen(filename, "rb");
    if (!img_file) {
        perror("Error opening file");
        exit(1);
    }

    int width, height;
    int max_val;
    if (fscanf(img_file, "P6 %d %d %d", &width, &height, &max_val) != 3 || max_val != 255) {
        fprintf(stderr, "Error: Invalid PPM format or max value != 255\n");
        exit(101);
    }
    fgetc(img_file); 

    unsigned char *output = malloc(width * height * 3);
    unsigned char *input = malloc(width * height * 3);
    int histogram[INTERVALS];

    size_t pixelsRead = fread(output, 1, width * height * 3, img_file);
    if (pixelsRead != (width * height * 3)) {
        exit(102);
    }
    fclose(img_file);

    apply_kernel(input, output, width, height);
    compute_histogram(output, width, height, histogram);
    save_histogram(histogram);

    FILE *output_file = fopen("output.ppm", "wb");
    fprintf(output_file, "P6\n%d\n%d\n255\n", width, height);
    fwrite(output, 1, width * height * 3, output_file);
    fclose(output_file);

    free(input);
    free(output);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        return 1;
    }
    process_image(argv[1]);
    return 0;
}