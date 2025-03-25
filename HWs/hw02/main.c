#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define INTERVALS 5   // Počet intervalů histogramu
#define MAX_LIGHT 255 // Maximální hodnota jasu
#define MIN_LIGHT 0 

// Funkce pro aplikaci konvoluční masky na vstupní obraz
void apply_kernel(const unsigned char *input, unsigned char *output, int width, int height) {
    int kernel[3][3] = {
        {  0, -1,  0 },
        { -1,  5, -1 },
        {  0, -1,  0 }
    };

    // Optimalizovaná práce s pamětí: použijeme pointery a výpočet indexů předem
    for (int ydx = 0; ydx < height; ydx++) {
        for (int idx = 0; idx < width; idx++) {
            for (int idc = 0; idc < 3; idc++) { // Pro každý barevný kanál (R, G, B)
                if (idx == 0 || idx == width - 1 || ydx == 0 || ydx == height - 1) {
                    // Okrajový pixel se pouze překopíruje
                    output[(ydx * width + idx) * 3 + idc] = input[(ydx * width + idx) * 3 + idc];
                } 
                
                else {
                    int new_value = 0;

                    // Konvoluce 3x3
                    for (int ky = -1; ky <= 1; ky++) {
                        for (int kx = -1; kx <= 1; kx++) {
                            int pixel_x = idx + kx;
                            int pixel_y = ydx + ky;
                            int index = (pixel_y * width + pixel_x) * 3 + idc;
                            new_value += input[index] * kernel[ky + 1][kx + 1];
                        }
                    }

                    // Clamping hodnot na rozsah [0,255]
                    if (new_value < MIN_LIGHT) new_value = MIN_LIGHT;
                    if (new_value > MAX_LIGHT) new_value = MAX_LIGHT;

                    output[(ydx * width + idx) * 3 + idc] = (unsigned char)new_value;
                }
            }
        }
    }
}

// Funkce pro výpočet histogramu ve stupních šedi
void compute_histogram(const unsigned char *image, int width, int height, int *histogram) {
    for (int idx = 0; idx < INTERVALS; idx++) {
        histogram[idx] = 0;
    }

    // Procházení všech pixelů a konverze do stupňů šedi
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            int gray = round(0.2126 * image[idx] + 0.7152 * image[idx + 1] + 0.0722 * image[idx + 2]);

            // Určení intervalu histogramu
            int bin = gray / (MAX_LIGHT / INTERVALS);
            if (bin >= INTERVALS) bin = INTERVALS - 1; // Poslední interval je zprava uzavřený

            histogram[bin]++;
        }
    }
}

// Funkce pro uložení histogramu do souboru
void save_histogram(const int *histogram) {
    FILE *file = fopen("histogram.txt", "w");

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

    char magic[3];
    int width, height, maxval;

    // Čtení hlavičky PPM
    if (fscanf(img_file, "%2s %d %d %d", magic, &width, &height, &maxval) != 4) {
        fprintf(stderr, "Chyba při čtení hlavičky PPM\n");
        fclose(img_file);
        exit(103);
    }

    fgetc(img_file); // Přeskočení jednoho znaku (newline)

    printf("Formát: %s\n", magic);
    printf("Šířka: %d\n", width);
    printf("Výška: %d\n", height);
    printf("Maxval: %d\n", maxval);

    // Alokace paměti pro obrázek
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

    // Načtení pixelových dat
    size_t pixelsRead = fread(valuesOfImg, 1, width * height * 3, img_file);
    fclose(img_file);
    
    if (pixelsRead != width * height * 3) {
        fprintf(stderr, "Chyba při čtení pixelových dat\n");
        free(valuesOfImg);
        free(outputImg);
        exit(105);
    }

    // Aplikace ostřícího filtru
    apply_kernel(valuesOfImg, outputImg, width, height);

    // Výpočet histogramu
    compute_histogram(outputImg, width, height, histogram);

    // Uložení histogramu
    save_histogram(histogram);

    // Uložení upraveného obrázku
    FILE *output_file = fopen("output.ppm", "wb");
    if (!output_file) {
        perror("Chyba při vytváření výstupního souboru");
        free(valuesOfImg);
        free(outputImg);
        exit(106);
    }

    fprintf(output_file, "P6\n%d %d\n%d\n", width, height, maxval);
    fwrite(outputImg, 1, width * height * 3, output_file);
    fclose(output_file);

    printf("Zaostřený obrázek uložen jako output.ppm\n");
    printf("Histogram uložen jako histogram.txt\n");

    // Uvolnění paměti
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
