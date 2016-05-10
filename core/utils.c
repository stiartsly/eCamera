#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include "tox/tox.h"
#include "vlog.h"

int32_t getSaveDataSize(const char* path)
{
    assert(path);

    FILE* fp = fopen(path, "rb");
    if (!fp) {
        vlogE("File (%s) not found", path);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int sz = ftell(fp);
    fclose(fp);

    return sz;
}

int32_t loadSaveData(const char* path, uint8_t* dataToSave, size_t length)
{
    assert(path);
    assert(dataToSave);
    assert(length);

    FILE* fp = NULL;
    uint8_t* data = NULL;
    int sz = 0;

    fp = fopen(path, "rb");
    if (!fp) {
        vlogE("File(%s) not found", path);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    sz   = ftell(fp);
    if (sz > (int)length) {
        sz = (int)length;
    }
    fseek(fp, 0, SEEK_SET);
    int ret = fread(dataToSave, sz, 1, fp);
    fclose(fp);
    if (ret != 1) {
        vlogE("Read file error");
        return -1;
    }
    return 0;
}

void storeSaveData(const char* path, const uint8_t* dataToStore, size_t length)
{
    assert(path);
    assert(dataToStore);
    assert(length);

    FILE* fp = NULL;
    fp = fopen(path, "wb");
    if (!fp) {
        vlogE("File open failed (%s)", path);
        return;
    }

    int sz = fwrite(dataToStore, length, 1, fp);
    if (sz != 1) {
        vlogE("File write failed");
        fclose(fp);
        return;
    }
    fflush(fp);
    fclose(fp);
    return;
}

#define FRADDR_TOSTR_CHUNK_LEN 8

static
void fraddr_to_str(uint8_t *id_bin, char *id_str)
{
    uint32_t i, delta = 0, pos_extra, sum_extra = 0;

    for (i = 0; i < TOX_ADDRESS_SIZE; i++) {
        sprintf(&id_str[2 * i + delta], "%02hhX", id_bin[i]);

        if ((i + 1) == TOX_PUBLIC_KEY_SIZE)
            pos_extra = 2 * (i + 1) + delta;

        if (i >= TOX_PUBLIC_KEY_SIZE)
            sum_extra |= id_bin[i];

        if (!((i + 1) % FRADDR_TOSTR_CHUNK_LEN)) {
            id_str[2 * (i + 1) + delta] = ' ';
            delta++;
        }
    }

    id_str[2 * i + delta] = 0;

    if (!sum_extra)
        id_str[pos_extra] = 0;
}

void toxDumpAddr(uint8_t* addr)
{
    char buf[200] = {0};
    int off = 0;

    off = sprintf(buf, "[ID]: ");
    fraddr_to_str(addr, buf + off);

    printf("%s", buf);
    printf("\n");
    return;
}


