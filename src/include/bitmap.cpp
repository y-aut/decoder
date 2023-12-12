#include "bitmap.hpp"

void save_bitmap(unsigned char *img, std::string filename, int width, int height) {
    FILE *fp;
    BitMap_t bitmap;

    bitmap.File.Type = 19778;
    bitmap.File.Size = 14 + 40 + width * height * 3;
    bitmap.File.Reserved1 = 0;
    bitmap.File.Reserved2 = 0;
    bitmap.File.OffBits = 14 + 40;

    bitmap.Info.Size = 40;
    bitmap.Info.Width = width;
    bitmap.Info.Height = height;
    bitmap.Info.Planes = 1;
    bitmap.Info.BitCount = 24;
    bitmap.Info.Compression = 0;
    bitmap.Info.SizeImage = 0;
    bitmap.Info.XPixPerMeter = 0;
    bitmap.Info.YPixPerMeter = 0;
    bitmap.Info.ClrUsed = 0;
    bitmap.Info.ClrImportant = 0;

    fp = fopen(filename.c_str(), "wb");
    fwrite(&bitmap, sizeof(bitmap), 1, fp);
    fwrite(img, sizeof(*img) * width * height * 3, 1, fp);
    fclose(fp);
};
