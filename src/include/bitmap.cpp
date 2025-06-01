#include "bitmap.hpp"
#include <unistd.h>

void save_bitmap(unsigned char *img, std::string filename, int width, int height) {
    FILE *fp;
    BitMap_t bitmap;

    int row_stride = ((width * 3 + 3) / 4) * 4; // 4バイトアライン
    int data_size = row_stride * height;

    bitmap.File.Type = 0x4D42;
    bitmap.File.Size = 14 + 40 + data_size;
    bitmap.File.Reserved1 = 0;
    bitmap.File.Reserved2 = 0;
    bitmap.File.OffBits = 14 + 40;

    bitmap.Info.Size = 40;
    bitmap.Info.Width = width;
    bitmap.Info.Height = height;
    bitmap.Info.Planes = 1;
    bitmap.Info.BitCount = 24;
    bitmap.Info.Compression = 0;
    bitmap.Info.SizeImage = data_size;
    bitmap.Info.XPixPerMeter = 0;
    bitmap.Info.YPixPerMeter = 0;
    bitmap.Info.ClrUsed = 0;
    bitmap.Info.ClrImportant = 0;

    fp = fopen(filename.c_str(), "wb");
    fwrite(&bitmap, sizeof(bitmap), 1, fp);

    BYTE padding[3] = {0, 0, 0};
    int padding_size = row_stride - width * 3;

    for (int y = 0; y < height; y++) {
        fwrite(&img[y * width * 3], 1, width * 3, fp);
        fwrite(padding, 1, padding_size, fp);
    }

    fflush(fp);
    fsync(fileno(fp));
    fclose(fp);
}
