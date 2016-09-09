/**
 * whodunit.c
 *
 
 */
       
#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"

int main(int argc, char* argv[])
{
    // ensure proper usage
    if (argc != 3)
    {
        printf("Usage: ./copy infile outfile\n");
        return 1;
    }

    // remember filenames, save in strings or char*
    char* infile = argv[1];
    char* outfile = argv[2];

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

    // write outfile's BITMAPFILEHEADER
    fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, outptr);

    // write outfile's BITMAPINFOHEADER
    fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, outptr);

    // determine padding for scanlines, biwidth is width of bitmap in pixels, padding can be 1,2 or 3 bytes. If no padding is needed, meaning the size of biwidth is multiple of 4, the padding added will be 4 % 4 = 0
    int padding =  (4 - (bi.biWidth * sizeof(RGBTRIPLE)) % 4) % 4;

    // iterate over infile's scanlines, biHeight is height in pixels
    for (int i = 0, biHeight = abs(bi.biHeight); i < biHeight; i++)
    {
        // iterate over pixels in scanline, pixel by pixel
        for (int j = 0; j < bi.biWidth; j++)
        {
            // temporary storage
            RGBTRIPLE triple;

            // read RGB triple from infile
            fread(&triple, sizeof(RGBTRIPLE), 1, inptr);
            
            // change red pixels to white pixels 0000ff to ffffff
            if ((triple.rgbtBlue == 0x00) && (triple.rgbtGreen == 0x00) && (triple.rgbtRed == 0xff))
            {
            triple.rgbtBlue = 0xff;
            triple.rgbtGreen = 0xff;
            triple.rgbtRed = 0xff;
            }
            
            /* if there is more blue than green, max the blue
            if (triple.rgbtBlue > triple.rgbtGreen)
            {
            triple.rgbtBlue = 0xff;
            triple.rgbtGreen = 0x00;
            triple.rgbtRed = 0x00;
            }
            */
            
            /*if there is more green than blue, max the green
            
            if (triple.rgbtGreen > triple.rgbtBlue )
            {
            triple.rgbtBlue = 0x00;
            triple.rgbtGreen = 0xff;
            triple.rgbtRed = 0x00;
            }
            */
            
            // write RGB triple to outfile
            fwrite(&triple, sizeof(RGBTRIPLE), 1, outptr);
        }

        // skip over padding,
        fseek(inptr, padding, SEEK_CUR);

        // then add it back (to demonstrate how) one byte at a time(one hexadecimal digit is 4 bits)
        for (int k = 0; k < padding; k++)
        {
            fputc(0x00, outptr);
        }
    }

    // close infile
    fclose(inptr);

    // close outfile
    fclose(outptr);

    // that's all folks
    return 0;
}
