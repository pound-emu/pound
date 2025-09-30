#include "pvm/pvm.h"

#define LOG_MODULE "switch1"
#include "common/logging.h"

namespace pound::pvm
{
static int8_t s1_init(pvm_t* pvm);
static int8_t s1_mmio_read(pvm_t* pvm, uint64_t gpa, uint8_t* data, size_t len);
static int8_t s1_mmio_write(pvm_t* pvm, uint64_t gpa, uint8_t* data, size_t len);
static void s1_destroy(pvm_t* pvm);

const pvm_ops_t s1_ops = {
    .init = s1_init,
    .destroy = s1_destroy,
};


static int8_t s1_init(pvm_t* pvm)
{
    LOG_INFO("Initializing Switch 1 virtual machine");
    /* BOOTSTRAPPING CODE GOES HERE */
    return 0;
}

static void  s1_destroy(pvm_t* pvm)
{
    /* TODO(GloriousTacoo:pvm) */
}
}
