#include <stdint.h>
#include "simpleserial.h"


//include DILITHIUM files (reference code)

#include "poly.h"
#include "polyvec.h"
#include "params.h" 
#include "rounding.h"
#include "fips202.h"

#define result_length 4

uint8_t rhoprime[64];

uint8_t set_key(uint8_t cmd, uint8_t scmd, uint8_t len, uint8_t* pt) {
  memcpy(rhoprime, pt, 64);
  return 0x00;
}

#if SS_VER == SS_VER_2_0
uint8_t recv_plain(uint8_t cmd, uint8_t scmd, uint8_t len, uint8_t* pt)
#else
uint8_t recv_plain(uint8_t* pt, uint8_t len)
#endif
{  
    poly y_poly;
    //uint8_t *rhoprime = pt+2;
    //uint16_t nonce = pt[0] | (pt[1] << 8);
    
    //poly_uniform_gamma1(&y_poly, rhoprime, nonce);
    polyz_unpack(&y_poly,pt);
    int32_t result[1];
    //polyz_unpack(&y_poly, outbuf);
    
    /* End of calculations */
    result[0] = y_poly.coeffs[0];
    simpleserial_put('r', 4, (uint8_t*) result);
    return 0x00; //((uint8_t*)nonce)[0];
    //result[1] = y_poly.coeffs[1];
    //result[2] = y_poly.coeffs[2];
    //result[3] = y_poly.coeffs[3];
     /* Sending output array */
    //simpleserial_put('r', 4, (uint8_t*)result);
    //uint8_t result_out = {0x00};
    //simpleserial_put('r', 1, result_out);

  
}

static uint8_t reset_board(int size, uint16_t *data)
{
  (void)(size);
  (void)(data);
  return 0x00;
}

static int init(void)
{

  platform_init();
	init_uart();
	trigger_setup();

  simpleserial_init();
  /*int status2 = simpleserial_addcmd('p', 16, recv_plain);
  if(status2){
    return status2;
  }
  int status3 = simpleserial_addcmd('x', 1024, reset_board);
  if(status3){
    return status3;
  }
  return 0;*/
}

int main(void) {
  int init_status = init();
  #if SS_VER == SS_VER_2_0
    simpleserial_addcmd(0x01, 9, recv_plain);
    simpleserial_addcmd(0x02, 64, set_key);
  #else
    simpleserial_addcmd('p', 4, recv_plain);
  #endif
  while(1){
    simpleserial_get();
  }
  if(init_status){
    simpleserial_put('z', 1, (uint8_t *) &init_status);
    return init_status;
  }

  while(1){
    simpleserial_get();
  }

  return 0;
}

