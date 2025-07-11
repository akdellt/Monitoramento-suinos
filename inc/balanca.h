#ifndef BALANCA_H
#define BALANCA_H

extern MFRC522Ptr_t mfrc;

long hx711_read();
float convert_to_weight(long raw_value, float scale_factor, float offset);
void coleta_peso();
void rfid_init();
void teste();

#endif