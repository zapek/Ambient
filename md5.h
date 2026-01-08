#ifndef AMBIENT_MD5_H
#define AMBIENT_MD5_H

/* Data structure for MD5 (Message-Digest) computation */
typedef struct {
	ULONG buf[4];         /* scratch buffer */
	ULONG i[2];           /* number of _bits_ handled mod 2^64 */
	unsigned char in[64]; /* input buffer */
} MD5_CTX;

void MD5Init(MD5_CTX *mdContext);
void MD5Update(MD5_CTX *mdContext, CONST_STRPTR inBuf, unsigned int inLen);
void MD5Final(unsigned char digest[16], MD5_CTX *mdContext);
void MD5Transform(ULONG *buf, ULONG *in);;

#endif /* AMBIENT_MD5_H */
