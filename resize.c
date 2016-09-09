/**
 * resize.c v2 SEPT 9
 * 
 * FIXED HEADER VALUES ISSUE BELOW
 * biSizeImage is not calculated correctly Using n=4 as multiplier, the output of my program(first column) compared to CS50 program is  
 * offset  type   name              argv[1]   argv[2]
 * 2       DWORD  bfSize           fffffe86  000001e6
 * 34      DWORD  biSizeImage      fffffe50  000001b0
 * biHeight is so huge, in both student and staff.bmp : fffffff4 because height is a negative value in this case, and negatives are calculated as complements of 2(whatever)
 * 
 * FIXED VERTICAL RESIZING ISSUE
 * 
 *  I don't think it makes a difference whether I add 1 byte at the end of the malloc'ed array of structs. valgrind didn't give me any errors
 * 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "bmp.h"


BITMAPFILEHEADER change_fheader(BITMAPFILEHEADER bf, BITMAPINFOHEADER copy_bi, BITMAPINFOHEADER bi);
BITMAPINFOHEADER change_iheader(BITMAPINFOHEADER bi, int n);

int main(int argc, char* argv[])
{
    // ensure proper usage
    if (argc != 4)
    {
        printf("Usage: ./resize n infile outfile\n");
        return 1;
    }
    if ( atoi(argv[1]) < 1 || atoi(argv[1]) > (pow(2,32) - 1))
    {
       printf("Multiplier must be greater or equal to 1 and smaller or equal to 2^32-1");
       return 1; 
    }    

    // remember filenames, save in strings or char*, remember multiplier
    int n = atoi(argv[1]);
    char* infile = argv[2];
    char* outfile = argv[3];

    // open input file, you declare a file/stream variable and initialize it by opening the file name received at command line. If file open fails, return error code. 
    FILE* inptr = fopen(infile, "r");
    if (inptr == NULL)
    {
        printf("Could not open %s.\n", infile);
        return 2;
    }

    // open output file, you declare a file/stream variable and initialize it by opening the file name received at command line, If file open fails, return error code.
    FILE* outptr = fopen(outfile, "w");
    if (outptr == NULL)
    {
        fclose(inptr);
        fprintf(stderr, "Could not create %s.\n", outfile);
        return 3;
    }

    // read infile's BITMAPFILEHEADER, bf is declared as variable of type BITMAPFILEHEADER . See "bmp.h" for typedef for this structure
    BITMAPFILEHEADER bf;
    // fread 1 chunk of size BITMAPFILEHEADER from file inptr and save at address &bf 
    fread(&bf, sizeof(BITMAPFILEHEADER), 1, inptr);

    // read infile's BITMAPINFOHEADER bi is declared as variable of type BITMAPINFOHEADER . See "bmp.h" for typedef for this structure
    BITMAPINFOHEADER bi;
    // fread 1 chunk of size BITMAPINFOHEADER from file inptr and save at address &bi 
    fread(&bi, sizeof(BITMAPINFOHEADER), 1, inptr);

    // ensure infile is (likely) a 24-bit uncompressed BMP 4.0
    if (bf.bfType != 0x4d42 || bf.bfOffBits != 54 || bi.biSize != 40 || 
        bi.biBitCount != 24 || bi.biCompression != 0)
    {
        fclose(outptr);
        fclose(inptr);
        fprintf(stderr, "Unsupported file format.\n");
        return 4;
    }
    
    // make copies of the file headers to work with
    BITMAPINFOHEADER copy_bi = change_iheader(bi, n);
    BITMAPFILEHEADER copy_bf = change_fheader(bf, copy_bi, bi);

    // write outfile's BITMAPFILEHEADER
    fwrite(&copy_bf, sizeof(BITMAPFILEHEADER), 1, outptr);

    // write outfile's BITMAPINFOHEADER
    fwrite(&copy_bi, sizeof(BITMAPINFOHEADER), 1, outptr);

    // determine padding for scanlines, biwidth is width of bitmap in pixels, padding can be 1,2 or 3 bytes. If no padding is needed, meaning the size of biwidth is multiple of 4, the padding added will be 4 % 4 = 0
    int padding =  (4 - (bi.biWidth * sizeof(RGBTRIPLE)) % 4) % 4;
    
    // determine padding for output file
    int new_padding =  (4 - (n * bi.biWidth * sizeof(RGBTRIPLE)) % 4) % 4;

    // iterate over infile's scanlines, biHeight is height in pixels. At every scanline, reset the counter for vertical stretch to equal n
    for (int i = 0, biHeight = abs(bi.biHeight); i < biHeight; i++)
    {
            // save the triples in an array of structs dynamically allocated, also check if memory was allocated succesfully
            RGBTRIPLE* scanline = malloc(n * sizeof(RGBTRIPLE) * bi.biWidth + 1);
            if (scanline == NULL)
            {
            	return 5;
            } 

            // use scanline_counter for keeping track of horizontal scaling
            int scanline_counter = 0;
            
            // iterate over pixels in scanline
            for (int j = 0; j < bi.biWidth; j++)
            {
                // declare variable for temporary pixel storage
                RGBTRIPLE triple;
    
                // read RGB triple from infile
                fread(&triple, sizeof(RGBTRIPLE), 1, inptr);
    
                // save horizontally expanded triple to "scanline" array
                for (int k = 0; k < n; k++)
                {
                            // fwrite(&triple, sizeof(RGBTRIPLE), 1, outptr);
                            scanline[scanline_counter] = triple;
                            scanline_counter++;
                }
            }
            
            // skip over padding in inptr, skip an amount of bytes equal to "padding" starting at current position
            fseek(inptr, padding, SEEK_CUR);
            
            // stretch vertically, writing scanlines n times in inptr
            int stretch = n;
            while (stretch > 0)
                {
                    // write the scanline array  to the outptr file
                    for (int k = 0; k < scanline_counter; k++)
                    {
                        fwrite(&scanline[k], sizeof(RGBTRIPLE), 1, outptr);
                    }
    
                    // add padding one byte at a time(one hexadecimal digit is 4 bits)
                    for (int k = 0; k < new_padding; k++)
                    {
                        fputc(0x00, outptr);
                    }
                    stretch --;    
                }
                
            free(scanline);
        
    }

    // close infile
    fclose(inptr);

    // close outfile
    fclose(outptr);

    // that's all folks
    return 0;
}

// Receives the fileheader from input file and modifies it to match the resized bitmap
BITMAPFILEHEADER change_fheader(BITMAPFILEHEADER bf, BITMAPINFOHEADER copy_bi, BITMAPINFOHEADER bi)
{
       bf.bfSize = bf.bfSize + (copy_bi.biSizeImage - bi.biSizeImage);
       
       return bf;
}


// Receives the fileheader from input file and modifies it to match the resized bitmap
BITMAPINFOHEADER change_iheader(BITMAPINFOHEADER bi, int n)
{
    // determined new padding, in bytes
    int padding =  (4 - (n * bi.biWidth * sizeof(RGBTRIPLE)) % 4) % 4;
    
    bi.biWidth = n * bi.biWidth;
    bi.biHeight = n * bi. biHeight;
    bi.biSizeImage = bi.biWidth * abs(bi.biHeight) * sizeof(RGBTRIPLE) + padding * abs(bi.biHeight);
    
    return bi;
}