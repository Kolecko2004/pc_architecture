#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


#define INTERVALS 5 // Pocet intervalu v histogramu
#define MAX_LIGHT 255 // Maximalni hodnota jasu
#define MIN_LIGHT 0 // Minimalni hodnota jasu
#define TILE_SIZE 64  // Minimální hodnota jasu

const int kernel[3][3] = {
    {  0, -1,  0 },
    { -1,  5, -1 },
    {  0, -1,  0 }
};

void apply_kernel(const unsigned char *input, unsigned char *output, int width, int height) {
    memcpy(output, input, width * height * 3);

    const int BLOCK_SIZE = 32;

    for (int by = 1; by < height - 1; by += BLOCK_SIZE) {
        for (int bx = 1; bx < width - 1; bx += BLOCK_SIZE) {
            int max_y = (by + BLOCK_SIZE < height - 1) ? by + BLOCK_SIZE : height - 1;
            int max_x = (bx + BLOCK_SIZE < width - 1) ? bx + BLOCK_SIZE : width - 1;

            for (int y = by; y < max_y; y++) {
                const unsigned char *row_above  = input + (y - 1) * width * 3;
                const unsigned char *row_center = input + y * width * 3;
                const unsigned char *row_below  = input + (y + 1) * width * 3;
                unsigned char *output_row       = output + y * width * 3;

                for (int x = bx; x < max_x; x++) {
                    int idx = x * 3;

                    for (int c = 0; c < 3; c++) {
                        int v00 = row_above[idx - 3 + c], v01 = row_above[idx + c], v02 = row_above[idx + 3 + c];
                        int v10 = row_center[idx - 3 + c], v11 = row_center[idx + c], v12 = row_center[idx + 3 + c];
                        int v20 = row_below[idx - 3 + c], v21 = row_below[idx + c], v22 = row_below[idx + 3 + c];

                        int new_value =
                            v00 * kernel[0][0] + v01 * kernel[0][1] + v02 * kernel[0][2] +
                            v10 * kernel[1][0] + v11 * kernel[1][1] + v12 * kernel[1][2] +
                            v20 * kernel[2][0] + v21 * kernel[2][1] + v22 * kernel[2][2];

                        output_row[idx + c] = (new_value < 0) ? 0 : (new_value > 255) ? 255 : (unsigned char)new_value;
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
            int bin = (gray * INTERVALS) / (MAX_LIGHT);

            if (bin >= INTERVALS) bin = INTERVALS - 1;

            histogram[bin]++;
        }
    }
}

void save_histogram(const int *histogram) {
    FILE *file = fopen("output.txt", "w");

    if (!file) {
        perror("Chyba při vytváření souboru histogram.txt");
        exit(101);
    }

    for (int idx = 0; idx < INTERVALS; idx++) {
        if (idx != 0) {
            fprintf(file, " ");
        }
        fprintf(file, "%d", histogram[idx]);
    }

    fclose(file);
}

void read_file(char *filename) {
    FILE *img_file = fopen(filename, "rb");
    if (!img_file) {
        perror("Chyba při otevírání souboru");
        exit(102);
    }

    int width, height;


    if (fscanf(img_file, "P6 %d %d 255", &width, &height) != 2) {
        fprintf(stderr, "Chyba při čtení hlavičky PPM\n");
        fclose(img_file);
        exit(103);
    }
    fgetc(img_file);


    unsigned char *valuesOfImg = malloc(width * height * 3);
    unsigned char *outputImg = malloc(width * height * 3);
    int histogram[INTERVALS];

    if (!valuesOfImg || !outputImg) {
        fprintf(stderr, "Chyba při alokaci paměti\n");
        free(valuesOfImg);
        free(outputImg);
        fclose(img_file);
        exit(104);
    }

    size_t pixelsRead = fread(valuesOfImg, 1, width * height * 3, img_file);
    fclose(img_file);
    
    if (pixelsRead != width * height * 3) {
        fprintf(stderr, "Chyba při čtení pixelových dat\n");
        free(valuesOfImg);
        free(outputImg);
        exit(105);
    }


    apply_kernel(valuesOfImg, outputImg, width, height);


    compute_histogram(outputImg, width, height, histogram);


    save_histogram(histogram);


    FILE *output_file = fopen("output.ppm", "wb");
    if (!output_file) {
        perror("Chyba při vytváření výstupního souboru");
        free(valuesOfImg);
        free(outputImg);
        exit(106);
    }

    fprintf(output_file, "P6\n%d %d\n255\n", width, height);
    fwrite(outputImg, 1, width * height * 3, output_file);
    fclose(output_file);

    free(valuesOfImg);
    free(outputImg);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Použití: %s <název souboru>\n", argv[0]);
        return 1;
    }

    read_file(argv[1]);
    return 0;
}
