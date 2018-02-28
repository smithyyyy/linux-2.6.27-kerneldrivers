
typedef unsigned long UINT4;

typedef struct {
  UINT4 m_state[4];			/* state (ABCD) */
  UINT4 m_count[2];			/* number of bits, modulo 2^64 (lsb first) */
  unsigned char m_buffer[64];	/* input buffer */
} MD5_CTX;

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *p, unsigned int iSize);
void MD5Final(unsigned char *p, MD5_CTX *);

void AddHash32(unsigned char *h, unsigned char *p);
