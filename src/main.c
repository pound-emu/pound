#define LOG_MODULE "main"
#include "common/logging.h"
#include "common/passert.h"
#include "jit/decoder/arm32.h"

int main()
{
    /* Add r0, r0, #1 */
    pvm_jit_decoder_arm32_decode(0xE2800001); 
    /* Sub r0, r0, #1 */
    pvm_jit_decoder_arm32_decode(0xE2400001);
    pvm_jit_decoder_arm32_decode(0xE12FFF1E);
}
