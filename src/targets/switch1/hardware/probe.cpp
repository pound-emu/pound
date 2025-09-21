#include "kvm/kvm.h"

#define LOG_MODULE "switch1"
#include "common/logging.h"

namespace pound::kvm
{
static int8_t s1_init(kvm_t* kvm);
static int8_t s1_mmio_read(kvm_t* kvm, uint64_t gpa, uint8_t* data, size_t len);
static int8_t s1_mmio_write(kvm_t* kvm, uint64_t gpa, uint8_t* data, size_t len);
static void s1_destroy(kvm_t* kvm);

const kvm_ops_t s1_ops = {
    .init = s1_init,
    .mmio_read = s1_mmio_read,
    .mmio_write = s1_mmio_write,
    .destroy = s1_destroy,
};


static int8_t s1_init(kvm_t* kvm)
{
    LOG_INFO("Initializing Switch 1 virtual machine");
    /* BOOTSTRAPPING CODE GOES HERE */
    return 0;
}
static int8_t s1_mmio_read(kvm_t* kvm, uint64_t gpa, uint8_t* data, size_t len)
{
    /* TODO(GloriousTacoo:kvm) */
    return 0;
}
static int8_t s1_mmio_write(kvm_t* kvm, uint64_t gpa, uint8_t* data, size_t len)
{
    /* TODO(GloriousTacoo:kvm) */
    return 0;
}
static void  s1_destroy(kvm_t* kvm)
{
    /* TODO(GloriousTacoo:kvm) */
}
}
