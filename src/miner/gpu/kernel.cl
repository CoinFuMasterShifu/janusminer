#define SHA256_S0(x) (ROL32(x, 25) ^ ROL32(x, 14) ^ SHR(x, 3))
#define SHA256_S1(x) (ROL32(x, 15) ^ ROL32(x, 13) ^ SHR(x, 10))

#define SHA256_S2(x) (ROL32(x, 30) ^ ROL32(x, 19) ^ ROL32(x, 10))
#define SHA256_S3(x) (ROL32(x, 26) ^ ROL32(x, 21) ^ ROL32(x, 7))

#define SHA256_P(a, b, c, d, e, f, g, h, x, K)                   \
    {                                                            \
        temp1 = h + SHA256_S3(e) + SHA256_F1(e, f, g) + (K + x); \
        d += temp1;                                              \
        h = temp1 + SHA256_S2(a) + SHA256_F0(a, b, c);           \
    }

#define SHA256_F0(y, x, z) bitselect(z, y, z ^ x)
#define SHA256_F1(x, y, z) bitselect(z, y, x)

#define SHA256_R0 (W0 = SHA256_S1(W14) + W9 + SHA256_S0(W1) + W0)
#define SHA256_R1 (W1 = SHA256_S1(W15) + W10 + SHA256_S0(W2) + W1)
#define SHA256_R2 (W2 = SHA256_S1(W0) + W11 + SHA256_S0(W3) + W2)
#define SHA256_R3 (W3 = SHA256_S1(W1) + W12 + SHA256_S0(W4) + W3)
#define SHA256_R4 (W4 = SHA256_S1(W2) + W13 + SHA256_S0(W5) + W4)
#define SHA256_R5 (W5 = SHA256_S1(W3) + W14 + SHA256_S0(W6) + W5)
#define SHA256_R6 (W6 = SHA256_S1(W4) + W15 + SHA256_S0(W7) + W6)
#define SHA256_R7 (W7 = SHA256_S1(W5) + W0 + SHA256_S0(W8) + W7)
#define SHA256_R8 (W8 = SHA256_S1(W6) + W1 + SHA256_S0(W9) + W8)
#define SHA256_R9 (W9 = SHA256_S1(W7) + W2 + SHA256_S0(W10) + W9)
#define SHA256_R10 (W10 = SHA256_S1(W8) + W3 + SHA256_S0(W11) + W10)
#define SHA256_R11 (W11 = SHA256_S1(W9) + W4 + SHA256_S0(W12) + W11)
#define SHA256_R12 (W12 = SHA256_S1(W10) + W5 + SHA256_S0(W13) + W12)
#define SHA256_R13 (W13 = SHA256_S1(W11) + W6 + SHA256_S0(W14) + W13)
#define SHA256_R14 (W14 = SHA256_S1(W12) + W7 + SHA256_S0(W15) + W14)
#define SHA256_R15 (W15 = SHA256_S1(W13) + W8 + SHA256_S0(W0) + W15)

#define SHA256_RD14 (SHA256_S1(W12) + W7 + SHA256_S0(W15) + W14)
#define SHA256_RD15 (SHA256_S1(W13) + W8 + SHA256_S0(W0) + W15)
#define ROL32(x, y) rotate(x, y##U)
#define SHR(x, y) (x >> y)
#define SWAP32(a) (as_uint(as_uchar4(a).wzyx))
#define SWAP4(x) as_uint(as_uchar4(x).wzyx)

typedef union {
    unsigned char h1[32];
    unsigned short h2[16];
    uint h4[8];
    ulong h8[4];
} hash_t;

typedef union {
    unsigned char h1[80];
    unsigned short h2[40];
    uint h4[20];
    ulong h8[10];
} header_t;

__kernel void h(__global header_t* header, uint threshold, uint M/* , __global hash_t* hashesOut */, volatile __global uint* out)
{

    uint temp1;
    uint W0 = SWAP32(header->h4[0]);
    uint W1 = SWAP32(header->h4[1]);
    uint W2 = SWAP32(header->h4[2]);
    uint W3 = SWAP32(header->h4[3]);
    uint W4 = SWAP32(header->h4[4]);
    uint W5 = SWAP32(header->h4[5]);
    uint W6 = SWAP32(header->h4[6]);
    uint W7 = SWAP32(header->h4[7]);
    uint W8 = SWAP32(header->h4[8]);
    uint W9 = SWAP32(header->h4[9]);
    uint W10 = SWAP32(header->h4[10]);
    uint W11 = SWAP32(header->h4[11]);
    uint W12 = SWAP32(header->h4[12]);
    uint W13 = SWAP32(header->h4[13]);
    uint W14 = SWAP32(header->h4[14]);
    uint W15 = SWAP32(header->h4[15]);

    uint v0 = 0x6A09E667;
    uint v1 = 0xBB67AE85;
    uint v2 = 0x3C6EF372;
    uint v3 = 0xA54FF53A;
    uint v4 = 0x510E527F;
    uint v5 = 0x9B05688C;
    uint v6 = 0x1F83D9AB;
    uint v7 = 0x5BE0CD19;

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, W0, 0x428A2F98);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, W1, 0x71374491);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, W2, 0xB5C0FBCF);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, W3, 0xE9B5DBA5);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, W4, 0x3956C25B);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, W5, 0x59F111F1);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, W6, 0x923F82A4);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, W7, 0xAB1C5ED5);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, W8, 0xD807AA98);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, W9, 0x12835B01);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, W10, 0x243185BE);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, W11, 0x550C7DC3);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, W12, 0x72BE5D74);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, W13, 0x80DEB1FE);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, W14, 0x9BDC06A7);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, W15, 0xC19BF174);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0xE49B69C1);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0xEFBE4786);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x0FC19DC6);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x240CA1CC);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x2DE92C6F);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x4A7484AA);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x5CB0A9DC);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x76F988DA);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0x983E5152);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0xA831C66D);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0xB00327C8);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0xBF597FC7);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0xC6E00BF3);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xD5A79147);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R14, 0x06CA6351);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R15, 0x14292967);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0x27B70A85);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0x2E1B2138);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x4D2C6DFC);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x53380D13);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x650A7354);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x766A0ABB);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x81C2C92E);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x92722C85);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0xA2BFE8A1);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0xA81A664B);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0xC24B8B70);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0xC76C51A3);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0xD192E819);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xD6990624);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R14, 0xF40E3585);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R15, 0x106AA070);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0x19A4C116);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0x1E376C08);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x2748774C);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x34B0BCB5);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x391C0CB3);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x4ED8AA4A);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x5B9CCA4F);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x682E6FF3);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0x748F82EE);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0x78A5636F);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0x84C87814);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0x8CC70208);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0x90BEFFFA);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xA4506CEB);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_RD14, 0xBEF9A3F7);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_RD15, 0xC67178F2);

    v0 += 0x6A09E667;
    uint s0 = v0;
    v1 += 0xBB67AE85;
    uint s1 = v1;
    v2 += 0x3C6EF372;
    uint s2 = v2;
    v3 += 0xA54FF53A;
    uint s3 = v3;
    v4 += 0x510E527F;
    uint s4 = v4;
    v5 += 0x9B05688C;
    uint s5 = v5;
    v6 += 0x1F83D9AB;
    uint s6 = v6;
    v7 += 0x5BE0CD19;
    uint s7 = v7;

    W0 = SWAP32(header->h4[16]);
    W1 = SWAP32(header->h4[17]);
    W2 = SWAP32(header->h4[18]);
    uint gid = get_global_id(0);
    W3 = gid;
    /*  */
    /* uint gid = ; */
    /* __global header_t *header = &(*hashes[gid]); */
    W4 = 0x80000000;
    W5 = 0;
    W6 = 0;
    W7 = 0;
    W8 = 0;
    W9 = 0;
    W10 = 0;
    W11 = 0;
    W12 = 0;
    W13 = 0;
    W14 = 0;
    W15 = 640;

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, W0, 0x428A2F98);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, W1, 0x71374491);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, W2, 0xB5C0FBCF);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, W3, 0xE9B5DBA5);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, W4, 0x3956C25B);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, W5, 0x59F111F1);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, W6, 0x923F82A4);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, W7, 0xAB1C5ED5);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, W8, 0xD807AA98);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, W9, 0x12835B01);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, W10, 0x243185BE);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, W11, 0x550C7DC3);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, W12, 0x72BE5D74);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, W13, 0x80DEB1FE);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, W14, 0x9BDC06A7);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, W15, 0xC19BF174);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0xE49B69C1);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0xEFBE4786);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x0FC19DC6);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x240CA1CC);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x2DE92C6F);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x4A7484AA);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x5CB0A9DC);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x76F988DA);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0x983E5152);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0xA831C66D);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0xB00327C8);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0xBF597FC7);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0xC6E00BF3);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xD5A79147);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R14, 0x06CA6351);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R15, 0x14292967);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0x27B70A85);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0x2E1B2138);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x4D2C6DFC);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x53380D13);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x650A7354);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x766A0ABB);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x81C2C92E);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x92722C85);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0xA2BFE8A1);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0xA81A664B);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0xC24B8B70);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0xC76C51A3);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0xD192E819);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xD6990624);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R14, 0xF40E3585);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R15, 0x106AA070);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0x19A4C116);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0x1E376C08);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x2748774C);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x34B0BCB5);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x391C0CB3);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x4ED8AA4A);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x5B9CCA4F);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x682E6FF3);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0x748F82EE);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0x78A5636F);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0x84C87814);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0x8CC70208);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0x90BEFFFA);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xA4506CEB);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_RD14, 0xBEF9A3F7);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_RD15, 0xC67178F2);

    v0 += s0;
    v1 += s1;
    v2 += s2;
    v3 += s3;
    v4 += s4;
    v5 += s5;
    v6 += s6;
    v7 += s7;

    W0 = v0;
    W1 = v1;
    W2 = v2;
    W3 = v3;
    W4 = v4;
    W5 = v5;
    W6 = v6;
    W7 = v7;
    W8 = 0x80000000;
    W9 = 0;
    W10 = 0;
    W11 = 0;
    W12 = 0;
    W13 = 0;
    W14 = 0;
    W15 = 256;

    v0 = 0x6A09E667;
    v1 = 0xBB67AE85;
    v2 = 0x3C6EF372;
    v3 = 0xA54FF53A;
    v4 = 0x510E527F;
    v5 = 0x9B05688C;
    v6 = 0x1F83D9AB;
    v7 = 0x5BE0CD19;

    /*  */
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, W0, 0x428A2F98);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, W1, 0x71374491);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, W2, 0xB5C0FBCF);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, W3, 0xE9B5DBA5);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, W4, 0x3956C25B);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, W5, 0x59F111F1);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, W6, 0x923F82A4);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, W7, 0xAB1C5ED5);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, W8, 0xD807AA98);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, W9, 0x12835B01);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, W10, 0x243185BE);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, W11, 0x550C7DC3);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, W12, 0x72BE5D74);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, W13, 0x80DEB1FE);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, W14, 0x9BDC06A7);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, W15, 0xC19BF174);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0xE49B69C1);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0xEFBE4786);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x0FC19DC6);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x240CA1CC);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x2DE92C6F);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x4A7484AA);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x5CB0A9DC);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x76F988DA);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0x983E5152);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0xA831C66D);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0xB00327C8);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0xBF597FC7);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0xC6E00BF3);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xD5A79147);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R14, 0x06CA6351);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R15, 0x14292967);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0x27B70A85);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0x2E1B2138);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x4D2C6DFC);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x53380D13);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x650A7354);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x766A0ABB);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x81C2C92E);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x92722C85);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0xA2BFE8A1);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0xA81A664B);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0xC24B8B70);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0xC76C51A3);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0xD192E819);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xD6990624);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R14, 0xF40E3585);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R15, 0x106AA070);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0x19A4C116);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0x1E376C08);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x2748774C);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x34B0BCB5);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x391C0CB3);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x4ED8AA4A);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x5B9CCA4F);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x682E6FF3);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0x748F82EE);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0x78A5636F);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0x84C87814);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0x8CC70208);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0x90BEFFFA);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xA4506CEB);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_RD14, 0xBEF9A3F7);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_RD15, 0xC67178F2);

    v0 += 0x6A09E667;
    v1 += 0xBB67AE85;
    v2 += 0x3C6EF372;
    v3 += 0xA54FF53A;
    v4 += 0x510E527F;
    v5 += 0x9B05688C;
    v6 += 0x1F83D9AB;
    v7 += 0x5BE0CD19;
    W0 = v0;
    W1 = v1;
    W2 = v2;
    W3 = v3;
    W4 = v4;
    W5 = v5;
    W6 = v6;
    W7 = v7;
    W8 = 0x80000000;
    W9 = 0;
    W10 = 0;
    W11 = 0;
    W12 = 0;
    W13 = 0;
    W14 = 0;
    W15 = 256;

    v0 = 0x6A09E667;
    v1 = 0xBB67AE85;
    v2 = 0x3C6EF372;
    v3 = 0xA54FF53A;
    v4 = 0x510E527F;
    v5 = 0x9B05688C;
    v6 = 0x1F83D9AB;
    v7 = 0x5BE0CD19;

    /*  */
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, W0, 0x428A2F98);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, W1, 0x71374491);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, W2, 0xB5C0FBCF);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, W3, 0xE9B5DBA5);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, W4, 0x3956C25B);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, W5, 0x59F111F1);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, W6, 0x923F82A4);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, W7, 0xAB1C5ED5);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, W8, 0xD807AA98);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, W9, 0x12835B01);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, W10, 0x243185BE);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, W11, 0x550C7DC3);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, W12, 0x72BE5D74);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, W13, 0x80DEB1FE);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, W14, 0x9BDC06A7);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, W15, 0xC19BF174);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0xE49B69C1);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0xEFBE4786);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x0FC19DC6);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x240CA1CC);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x2DE92C6F);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x4A7484AA);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x5CB0A9DC);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x76F988DA);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0x983E5152);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0xA831C66D);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0xB00327C8);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0xBF597FC7);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0xC6E00BF3);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xD5A79147);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R14, 0x06CA6351);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R15, 0x14292967);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0x27B70A85);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0x2E1B2138);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x4D2C6DFC);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x53380D13);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x650A7354);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x766A0ABB);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x81C2C92E);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x92722C85);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0xA2BFE8A1);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0xA81A664B);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0xC24B8B70);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0xC76C51A3);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0xD192E819);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xD6990624);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R14, 0xF40E3585);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R15, 0x106AA070);

    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R0, 0x19A4C116);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R1, 0x1E376C08);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R2, 0x2748774C);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R3, 0x34B0BCB5);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R4, 0x391C0CB3);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R5, 0x4ED8AA4A);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_R6, 0x5B9CCA4F);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_R7, 0x682E6FF3);
    SHA256_P(v0, v1, v2, v3, v4, v5, v6, v7, SHA256_R8, 0x748F82EE);
    SHA256_P(v7, v0, v1, v2, v3, v4, v5, v6, SHA256_R9, 0x78A5636F);
    SHA256_P(v6, v7, v0, v1, v2, v3, v4, v5, SHA256_R10, 0x84C87814);
    SHA256_P(v5, v6, v7, v0, v1, v2, v3, v4, SHA256_R11, 0x8CC70208);
    SHA256_P(v4, v5, v6, v7, v0, v1, v2, v3, SHA256_R12, 0x90BEFFFA);
    SHA256_P(v3, v4, v5, v6, v7, v0, v1, v2, SHA256_R13, 0xA4506CEB);
    SHA256_P(v2, v3, v4, v5, v6, v7, v0, v1, SHA256_RD14, 0xBEF9A3F7);
    SHA256_P(v1, v2, v3, v4, v5, v6, v7, v0, SHA256_RD15, 0xC67178F2);

    v0 += 0x6A09E667;
    v1 += 0xBB67AE85;
    v2 += 0x3C6EF372;
    v3 += 0xA54FF53A;
    v4 += 0x510E527F;
    v5 += 0x9B05688C;
    v6 += 0x1F83D9AB;
    v7 += 0x5BE0CD19;

    const uint c = C_CONSTANT;
    if (v0 >= c && v0 < threshold) {
        uint pos = 2 * atomic_inc(out) + 1;
        if (pos < 2 * M) {
            out[pos + 0] = gid;
            out[pos + 1] = v0;
        }
        /* uint index = M*(float)(v0-c)/(float)(threshold - c); */
        /* if (index < M) { */
        /*     out[index] = v0; */
        /* } */
    }
    /* hashesOut[0].h4[0] = SWAP32(v0); */
    /* hashesOut[0].h4[1] = SWAP32(v1); */
    /* hashesOut[0].h4[2] = SWAP32(v2); */
    /* hashesOut[0].h4[3] = SWAP32(v3); */
    /* hashesOut[0].h4[4] = SWAP32(v4); */
    /* hashesOut[0].h4[5] = SWAP32(v5); */
    /* hashesOut[0].h4[6] = SWAP32(v6); */
    /* hashesOut[0].h4[7] = SWAP32(v7); */
}
