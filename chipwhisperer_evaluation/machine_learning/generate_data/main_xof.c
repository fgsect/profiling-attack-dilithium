#include <stdint.h>
#include <stdio.h>
//include DILITHIUM files (reference code)
#include "params.h"
#include "poly.h"
#include "rounding.h"
#include "fips202.h"
#include "symmetric.h"



void randombytes(uint8_t *out, size_t outlen, uint64_t seed)
{
  unsigned int i;
  uint8_t buf[8];
  //static uint64_t ctr = 0;
  uint64_t ctr = seed;

  for (i = 0; i < 8; ++i){
    buf[i] = ctr >> 8 * i;
    //printf("%d\n", ctr >> 8);
 } 
    
  ctr++;
  shake128(out, outlen, buf, 8);
}
//Generate random bytes
//Save value + first coefficient


int main()
{
  /*uint8_t rhoprime[64];
  randombytes(rhoprime, 64, 137281);
  vini_hexdump(rhoprime, 64, "rhoprime" );
  return 0;*/

  for (uint32_t i = 0; i < UINT32_MAX; ++i)
  {
    uint8_t rhoprime[64];
    uint16_t nonce;
    randombytes(rhoprime, 64, (uint64_t)i);
    nonce = 0;
    poly y_poly;
    #define POLY_UNIFORM_GAMMA1_NBLOCKS ((POLYZ_PACKEDBYTES + STREAM256_BLOCKBYTES - 1)/STREAM256_BLOCKBYTES)

    uint8_t buf[POLY_UNIFORM_GAMMA1_NBLOCKS*STREAM256_BLOCKBYTES];
    stream256_state state;

    stream256_init(&state, rhoprime, nonce);
    stream256_squeezeblocks(buf, POLY_UNIFORM_GAMMA1_NBLOCKS, &state);
    polyz_unpack(&y_poly, buf);
    printf("%u %d %d %d %d\n", i, y_poly.coeffs[0], y_poly.coeffs[1], y_poly.coeffs[2], y_poly.coeffs[3]);
    //printf("%d %d\n", i, y_poly.coeffs[0]);
    /*printf("%d %d\n", i, y_poly.coeffs[1]);
    printf("%d %d\n", i, y_poly.coeffs[1]);
    printf("%d %d\n", i, y_poly.coeffs[2]);*/
    /*uint8_t outbuf[1000];
    poly y_poly;
    randombytes(outbuf, 1000, (uint64_t)i);
    polyz_unpack(&y_poly, outbuf);
    printf("%d %d\n", i, y_poly.coeffs[0]);*/
    /* if (y_poly.coeffs[0] > (1 << 17))
      {
        printf("%d\n", i);
      }
    */
    /*for (int j = 0; j < 4; ++j)
    {
     
    }*/
  }
}
