
#include "qiof_bitmap.h"

char *g_bitmap = NULL;
uint64_t g_size = 0;
uint64_t g_base = 0;
int qiof_bitmap_init(uint64_t size, uint64_t start)
{
    QIOF_LOG("init bitmap,size is %lu...................................\n", size);
    g_bitmap = (char *)malloc(((size - 1) / 8 + 1) * sizeof(char));
    if (g_bitmap == NULL)
        return 0;
    g_base = start;
    g_size = (size - 1) / 8 + 1;
    memset(g_bitmap, 0x0, g_size);
    return 1;
}

int qiof_bitmap_set(uint64_t index)
{
    uint64_t quo = (index - g_base) / 8;
    uint64_t remainder = (index - g_base) % 8;
    unsigned char x = (0x1 << remainder);
    if (quo > g_size)
        return 0;
    g_bitmap[quo] |= x;
    return 1;
}

int qiof_bitmap_get(uint64_t i)
{
    uint64_t quo = (i) / 8;
    uint64_t remainder = (i) % 8;
    unsigned char x = (0x1 << remainder);
    unsigned char res;
    if (quo > g_size)
        return -1;
    res = g_bitmap[quo] & x;
    return res > 0 ? 1 : 0;
}

uint64_t qiof_bitmap_data(uint64_t index)
{
    return (index + g_base);
}
void qiof_bitmap_free(void)
{
    QIOF_LOG("free bitmap...................................\n");
    free(g_bitmap);
    g_bitmap = NULL;
}
int qiof_bitmap_check(void)
{
    if (g_bitmap)
        return 1;
    return 0;
}