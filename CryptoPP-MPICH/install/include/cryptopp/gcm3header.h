#ifndef __GCM3HEADER_H
#define __GCM3HEADER_H

#ifdef __cplusplus
extern "C" {
#endif

extern int encrypt( unsigned char *buf, unsigned char *ciphertext , int count);

extern int decrypt( unsigned char *ciphertext, unsigned char *buf , int count);

#ifdef __cplusplus
}
#endif
#endif

