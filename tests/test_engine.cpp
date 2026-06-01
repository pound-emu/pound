#include "gpu/engine.h"
#include <gtest/gtest.h>

// THIS FILE TESTS THE BARE MINIMUM! THE TESTS WILL BE REWRITTEN IN THE FUTURE.

class GpuEngineTest : public testing::Test
{
protected:
    gpu_engine_t *engine = nullptr;

    void SetUp() override
    {
        engine = gpu_engine_create();
        ASSERT_NE(engine, nullptr);
    }

    void TearDown() override
    {
        gpu_engine_destroy(engine);
    }
};

TEST_F(GpuEngineTest, Alu_IADD3)
{
    sm86_raw_instruction_t raw_shader[2] = {};

    // Inst 0: IADD3 R0, R1, R2, R3
    raw_shader[0].low  = 0x010 | 7 << 12 | 0 << 16 | 1 << 24 | static_cast<uint64_t>(2) << 32;
    raw_shader[0].high = 3;

    // Inst 1: EXIT
    raw_shader[1].low = 0x94D | 7 << 12;

    sm86_decoded_instruction_t decoded_shader[2] = {};
    sm86_decode(&raw_shader[0], &decoded_shader[0]);
    sm86_decode(&raw_shader[1], &decoded_shader[1]);

    sm86_cta_t        *cta     = &engine->cta;
    constexpr uint32_t warp_id = 0;

    for (int i = 0; i < SM86_WARP_SIZE; ++i)
    {
        cta->warps[warp_id].gprs[1][i] = 10 + i; // R1 = 10, 11, 12...
        cta->warps[warp_id].gprs[2][i] = 20;     // R2 = 20
        cta->warps[warp_id].gprs[3][i] = 5;      // R3 = 5
    }

    gpu_engine_launch_cta(engine, 1, decoded_shader);

    for (int i = 0; i < SM86_WARP_SIZE; ++i)
    {
        uint32_t expected = 10 + i + 20 + 5;
        EXPECT_EQ(cta->warps[warp_id].gprs[0][i], expected);
    }
}
