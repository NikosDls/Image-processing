#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tiffio.h"

#define R 256
#define C 256

int debug;

/* for RGB image create three output files with these extensions */
char *color[3] = {"red", "grn", "blu"};

char text[80];
FILE *ofp[3];
unsigned char *obuf[3];

int main(int argc, char *argv[]) {
    char c;
    int errflg;
    extern int optind;
    extern char *optarg;
    TIFF* tif;
    uint32 width, height, *raster, *p;
    uint16 depth;
    size_t npixels;
    int i, j;
    uint32 x, y, z;
    unsigned char *pr, *pg, *pb;
    unsigned char tableR[R][C], tableG[R][C], tableB[R][C];
    unsigned char tableRnegative[R][C];
    unsigned char tableRframe[R + 2][C + 2], structuringElement[3][3];
    unsigned char tableRerosion[R + 2][C + 2], tableRdilation[R + 2][C + 2];
    unsigned char tableRrotate[R][C];
    unsigned char tableRbrightness[R][C];
    double angle;
    int hwidth, hheight, xt, yt, xs, ys;
    double sina, cosa;
    int direction;
    double brightness;
    double temp, newValue;
    int r, g, b;

    debug = errflg = 0;

    int choice;

    while ((c = getopt(argc, argv, "d")) != EOF)
        switch (c) {
            case 'd':
                debug = 1;
                break;
            case '?':
                errflg = 1;
                break;
        }
    if (errflg) {
        usage(argv[0]);
        exit(1);
    }

    if (optind < argc) {
        if ((tif = TIFFOpen(argv[optind], "r")) == NULL) {
            ;
            fprintf(stderr, "Can't open input file \"%s\"\n", argv[optind]);
            exit(1);
        }
        optind++;
    } else {
        fprintf(stderr, "Must specify input file\n");
        usage(argv[0]);
        exit(1);
    }

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    npixels = width * height;

    /* See if this is a color or gray image */
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &depth);
    if (depth != 3 && depth != 1) {
        fprintf(stderr, "Unrecognized image depth = %d\n");
        TIFFClose(tif);
        exit(1);
    }

    if (debug)
        printf("%d x %d x %d = %d pixels\n", width, height, depth, npixels);

    raster = (uint32*) _TIFFmalloc(npixels * sizeof (uint32));
    if (raster != NULL) {
        if (TIFFReadRGBAImageOriented(tif, width, height, raster, ORIENTATION_TOPLEFT, 0)) {

            do {
                printf("----------\n"
                        "1. Negative image\n"
                        "2. Change color\n"
                        "3. Decrease image\n"
                        "4. Increase image\n"
                        "5. Rotate image\n"
                        "6. Increase/Decrease brightness\n"
                        "----------\n"
                        "Choice: ");
                scanf("%d", &choice);
            } while (choice < 1 || choice > 6);

            if (depth == 3) {
                p = raster;
                for (y = 0; y < height; y++) {
                    pr = obuf[0];
                    pg = obuf[1];
                    pb = obuf[2];
                    for (x = 0; x < width; x++) {
                        z = *p++;

                        *pr++ = TIFFGetR(z);
                        *pg++ = TIFFGetG(z);
                        *pb++ = TIFFGetB(z);
                    }
                    //for (i = 0; i < 3; i++)
                    //fwrite(obuf[i], 1, width, ofp[i]);
                }
            } else /* image has only shades of black */ {
                switch (choice) {
                    case 2:
                        if (optind < argc) {
                            for (i = 0; i < 3; i++) {
                                if ((obuf[i] = (unsigned char *) malloc(width)) == NULL) {
                                    fprintf(stderr, "Can't malloc output buffer\n");
                                    TIFFClose(tif);
                                    exit(1);
                                }
                                /* Use the output file name as the basename for the file and add extensions for each color */
                                sprintf(text, "%s.%s", argv[optind], color[i]);
                                if ((ofp[i] = fopen(text, "w")) == NULL) {
                                    fprintf(stderr, "Can't open %s for output\n", text);
                                    TIFFClose(tif);
                                    exit(1);
                                }
                            }
                        } else {
                            fprintf(stderr, "Must specify output file for color output\n");
                            TIFFClose(tif);
                            exit(1);
                        }
                        break;

                    default:
                        if ((obuf[0] = (unsigned char *) malloc(width)) == NULL) {
                            fprintf(stderr, "Can't malloc output buffer\n");
                            TIFFClose(tif);
                            exit(1);
                        }
                        if (optind < argc) {
                            if ((ofp[0] = fopen(argv[optind], "w")) == NULL) {
                                fprintf(stderr, "Can't open %s for output\n", argv[optind]);
                                TIFFClose(tif);
                                exit(1);
                            }
                        } else
                            ofp[0] = stdout;
                }

                p = raster;
                for (y = 0; y < height; y++) {
                    pr = obuf[0];
                    for (x = 0; x < width; x++) {
                        z = *p++;
                        tableR[y][x] = TIFFGetR(z);
                        //*pr++ = TIFFGetR(z);
                    }
                    //fwrite(obuf[0], 1, width, ofp[0]);
                }
            }

            if (choice == 3 || choice == 4) {
                // creating image with frames
                for (i = 0; i < height + 2; i++) {
                    for (j = 0; j < width + 2; j++) {
                        if (i == 0 || i == 257 || j == 0 || j == 257) {
                            tableRframe[i][j] = 255;
                        } else {
                            tableRframe[i][j] = tableR[i - 1][j - 1];
                        }
                    }
                }
                // -----
	
                // creating structuring element
                for (i = 0; i < 3; i++) {
                    for (j = 0; j < 3; j++) {
                        if (i == 1 || j == 1) {
                            structuringElement[i][j] = 0;
                        } else {
                            structuringElement[i][j] = 255;
                        }
                    }
                }
                // -----
            }
            
            switch (choice) {
                case 1:
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (tableR[y][x] == 0) {
                                tableRnegative[y][x] = 255;
                            } else {
                                tableRnegative[y][x] = 0;
                            }
                        }
                    }

                    for (y = 0; y < height; y++) {
                        pr = obuf[0];
                        for (x = 0; x < width; x++) {
                            *pr++ = tableRnegative[y][x];
                        }
                        fwrite(obuf[0], 1, width, ofp[0]);
                    }

                    break;

                case 2:
                    printf("Give the RGB values(0-255)\n");
                    do {
                        printf("R: ");
                        scanf("%d", &r);
                    } while (r < 0 || r > 255);

                    do {
                        printf("G: ");
                        scanf("%d", &g);
                    } while (g < 0 || g > 255);

                    do {
                        printf("B: ");
                        scanf("%d", &b);
                    } while (b < 0 || b > 255);

                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (tableR[y][x] == 0) { /* change black color to RGB */
                                tableR[y][x] == r;
                                tableG[y][x] == g;
                                tableB[y][x] == b;
                            } else { /* white remain white */
                                tableR[y][x] == 255;
                                tableG[y][x] == 255;
                                tableB[y][x] == 255;
                            }
                        }
                    }

                    for (y = 0; y < height; y++) {
                        pr = obuf[0];
                        pg = obuf[1];
                        pb = obuf[2];
                        for (x = 0; x < width; x++) {
                            *pr++ = tableR[y][x];
                            *pg++ = tableG[y][x];
                            *pb++ = tableB[y][x];
                        }
                        
                        for (i = 0; i < 3; i++) {
                            fwrite(obuf[i], 1, width, ofp[i]);
                        }
                    }

                    break;

                case 3:
                    for (i = 0; i < height + 2; i++) {
                        for (j = 0; j < width + 2; j++) {
                            if (structuringElement[0][1] == tableRframe[i - 1][j]
                                    && structuringElement[1][0] == tableRframe[i][j - 1]
                                    && structuringElement[1][1] == tableRframe[i][j]
                                    && structuringElement[1][2] == tableRframe[i][j + 1]
                                    && structuringElement[2][1] == tableRframe[i + 1][j]
                                    ) {
                                tableRerosion[i][j] = 0;
                            } else {
                                tableRerosion[i][j] = 255;
                            }
                        }
                    }

                    for (y = 0; y < height + 2; y++) {
                        pr = obuf[0];
                        for (x = 0; x < width + 2; x++) {
                            *pr++ = tableRerosion[y][x];
                        }
                        fwrite(obuf[0], 1, width + 2, ofp[0]);
                    }

                    break;

                case 4:
                    for (i = 0; i < height + 2; i++) {
                        for (j = 0; j < width + 2; j++) {
                            if (structuringElement[0][1] == tableRframe[i - 1][j]
                                    || structuringElement[1][0] == tableRframe[i][j - 1]
                                    || structuringElement[1][1] == tableRframe[i][j]
                                    || structuringElement[1][2] == tableRframe[i][j + 1]
                                    || structuringElement[2][1] == tableRframe[i + 1][j]
                                    ) {
                                tableRdilation[i][j] = 0;
                            } else {
                                tableRdilation[i][j] = 255;
                            }
                        }
                    }

                    for (y = 0; y < height + 2; y++) {
                        pr = obuf[0];
                        for (x = 0; x < width + 2; x++) {
                            *pr++ = tableRdilation[y][x];
                        }
                        fwrite(obuf[0], 1, width + 2, ofp[0]);
                    }

                    break;

                case 5:
                    do {

                        printf("\n----------\n1. Right\n2. Left\n3. Rotate with angle\n----------\nChoice: ");

                        scanf("%d", &direction);
                    } while (direction < 1 || direction > 3);

                    switch (direction) {
                        case 1:
                            for (x = 0; x < width; x++) {
                                for (y = 0; y < height; y++) {
                                    tableRrotate[y][width - x - 1] = tableR[x][y];
                                }
                            }

                            break;

                        case 2:
                            for (x = 0; x < width; x++) {
                                for (y = 0; y < height; y++) {
                                    tableRrotate[height - y - 1][x] = tableR[x][y];
                                }
                            }

                            break;

                        case 3:
                            do {

                                printf("Give the rotation angle: ");
                                scanf("%lf", &angle);
                            } while (angle < 0 || angle > 360);

                            for (int x = 0; x < width; x++) {
                                for (int y = 0; y < height; y++) {
                                    hwidth = width / 2;
                                    hheight = height / 2;

                                    xt = x - hwidth;
                                    yt = y - hheight;

                                    sina = sin(-angle);
                                    cosa = cos(-angle);

                                    xs = (int) round((cosa * xt - sina * yt) + hwidth);
                                    ys = (int) round((sina * xt + cosa * yt) + hheight);

                                    if (xs >= 0 && xs < width && ys >= 0 && ys < height) {
                                        // set pixel (x,y) to color at (xs,ys)
                                        tableRrotate[x][y] = tableR[xs][ys];
                                    } else {
                                        // set pixel (x,y) to some default background
                                        tableRrotate[x][y] = 255;
                                    }
                                }
                            }

                            break;
                    }

                    for (y = 0; y < height; y++) {
                        pr = obuf[0];
                        for (x = 0; x < width; x++) {
                            *pr++ = tableRrotate[y][x];
                        }
                        fwrite(obuf[0], 1, width, ofp[0]);
                    }

                    break;

                case 6:
                    do {
                        printf("Give the amount of increasement or decreasement (range -100%% to 100%%): ");
                        scanf("%lf", &brightness);
                    } while (brightness < -100 || brightness > 100);

                    temp = brightness / 100.0;
                    //printf("%lf", temp);

                    for (int y = 0; y < height; y++) {
                        for (int x = 0; x < width; x++) {

                            if (tableR[y][x] == 255) {
                                tableRbrightness[y][x] = 255;
                            } else {

                                newValue = tableR[y][x] + (-temp * 255);

                                //newValue = tableR[y][x] * (1.0 + temp);
                                //printf("%u %lf\t", tableR[y][x], newValue);

                                if (newValue > 255.0) {
                                    tableRbrightness[y][x] = 255;
                                } else if (newValue < 0.0) {
                                    tableRbrightness[y][x] = 0;
                                } else {
                                    tableRbrightness[y][x] = (unsigned char) newValue;
                                    //printf("%u\t", tableRbrightness[y][x]);
                                }
                            }

                        }
                    }

                    for (y = 0; y < height; y++) {
                        pr = obuf[0];
                        for (x = 0; x < width; x++) {
                            *pr++ = tableRbrightness[y][x];
                        }
                        fwrite(obuf[0], 1, width, ofp[0]);
                    }

                    break;
            }

        }
        _TIFFfree(raster);
    }
    TIFFClose(tif);
    exit(0);
}

usage(s)
char *s;
{
    fprintf(stderr, "Usage: %s input-TIFF-file output-raw-file\n", s);
}
