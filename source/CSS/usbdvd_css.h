#ifndef USBDVD_CSS_H
#define USBDVD_CSS_H

#include <cstdint>

void CryptKey( int i_key_type, int i_variant,
                      const uint8_t *p_challenge, uint8_t *p_key );
void DecryptKey( uint8_t invert, const uint8_t *p_key,
                        const uint8_t *p_crypted, uint8_t *p_result );
int AttackPattern( const uint8_t p_sec[ 2048 ],
                          uint8_t *p_key ,uint32_t * i_success,uint32_t * i_tries );                
int dvdcss_unscramble( uint8_t * p_key, uint8_t *p_sec );

#endif