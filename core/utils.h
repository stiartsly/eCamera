#pragma once

int32_t getSaveDataSize(const char* path);
int32_t loadSaveData   (const char* path, uint8_t* dataToSave,  size_t length);
void    storeSaveData  (const char* path, const uint8_t* dataToStore, size_t length);

void toxBootstrap(void (*cb)(const char*, uint16_t, const uint8_t*));
void toxDumpAddr(uint8_t* addr);

