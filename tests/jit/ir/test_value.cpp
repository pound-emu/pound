#include "gtest/gtest.h"
#include "jit/ir/value.h"

namespace pound::jit::ir {

// Test fixture for value_t tests
class ValueTest : public ::testing::Test {
protected:
    value_t val;

    void SetUp() override {
        value_init(&val);
    }

    void TearDown() override {
    }
};

// Test value_init
TEST_F(ValueTest, InitializesToVoid) {
    EXPECT_EQ(val.type, IR_TYPE_VOID);
}

// Test U1 type
TEST_F(ValueTest, U1InitializationAndRetrieval) {
    value_init_from_u1(&val, true);
    EXPECT_EQ(val.type, IR_TYPE_U1);
    EXPECT_TRUE(value_get_u1(&val));

    value_init_from_u1(&val, false);
    EXPECT_EQ(val.type, IR_TYPE_U1);
    EXPECT_FALSE(value_get_u1(&val));
}

// Test U8 type
TEST_F(ValueTest, U8InitializationAndRetrieval) {
    uint8_t test_val = 0xAB;
    value_init_from_u8(&val, test_val);
    EXPECT_EQ(val.type, IR_TYPE_U8);
    EXPECT_EQ(value_get_u8(&val), test_val);

    value_init_from_u8(&val, 0x00);
    EXPECT_EQ(val.type, IR_TYPE_U8);
    EXPECT_EQ(value_get_u8(&val), 0x00);

    value_init_from_u8(&val, 0xFF);
    EXPECT_EQ(val.type, IR_TYPE_U8);
    EXPECT_EQ(value_get_u8(&val), 0xFF);
}

// Test U32 type
TEST_F(ValueTest, U32InitializationAndRetrieval) {
    uint32_t test_val = 0xABCDEF12;
    value_init_from_u32(&val, test_val);
    EXPECT_EQ(val.type, IR_TYPE_U32);
    EXPECT_EQ(value_get_u32(&val), test_val);

    value_init_from_u32(&val, 0x00000000);
    EXPECT_EQ(val.type, IR_TYPE_U32);
    EXPECT_EQ(value_get_u32(&val), 0x00000000);

    value_init_from_u32(&val, 0xFFFFFFFF);
    EXPECT_EQ(val.type, IR_TYPE_U32);
    EXPECT_EQ(value_get_u32(&val), 0xFFFFFFFF);
}

// Test U64 type
TEST_F(ValueTest, U64InitializationAndRetrieval) {
    uint64_t test_val = 0x123456789ABCDEF0ULL;
    value_init_from_u64(&val, test_val);
    EXPECT_EQ(val.type, IR_TYPE_U64);
    EXPECT_EQ(value_get_u64(&val), test_val);

    value_init_from_u64(&val, 0x0000000000000000ULL);
    EXPECT_EQ(val.type, IR_TYPE_U64);
    EXPECT_EQ(value_get_u64(&val), 0x0000000000000000ULL);

    value_init_from_u64(&val, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(val.type, IR_TYPE_U64);
    EXPECT_EQ(value_get_u64(&val), 0xFFFFFFFFFFFFFFFFULL);
}

// Test A32 Register type
TEST_F(ValueTest, A32RegisterInitializationAndRetrieval) {
    value_init_from_a32_register(&val, A32_REGISTER_R0);
    EXPECT_EQ(val.type, IR_TYPE_A32_REGISTER);
    EXPECT_EQ(value_get_a32_register(&val), A32_REGISTER_R0);

    value_init_from_a32_register(&val, A32_REGISTER_SP);
    EXPECT_EQ(val.type, IR_TYPE_A32_REGISTER);
    EXPECT_EQ(value_get_a32_register(&val), A32_REGISTER_SP);

    value_init_from_a32_register(&val, A32_REGISTER_PC);
    EXPECT_EQ(val.type, IR_TYPE_A32_REGISTER);
    EXPECT_EQ(value_get_a32_register(&val), A32_REGISTER_PC);

    value_init_from_a32_register(&val, A32_REGISTER_R10);
    EXPECT_EQ(val.type, IR_TYPE_A32_REGISTER);
    EXPECT_EQ(value_get_a32_register(&val), A32_REGISTER_R10);
}

// --- Death Tests for Type Mismatches ---
// These tests assume that value_get_* functions use PVM_ASSERT
// to check the type and that PVM_ASSERT aborts the program,
// allowing GTest's EXPECT_DEATH to catch it.

TEST(ValueDeathTest, GetU1FromVoid) {
    value_t v;
    value_init(&v);
    EXPECT_DEATH(value_get_u1(&v), "ASSERTION FAILURE");
}

TEST(ValueDeathTest, GetU8FromU32) {
    value_t v;
    value_init_from_u32(&v, 123);
    EXPECT_DEATH(value_get_u8(&v), "ASSERTION FAILURE");
}

TEST(ValueDeathTest, GetU32FromU64) {
    value_t v;
    value_init_from_u64(&v, 123456789);
    EXPECT_DEATH(value_get_u32(&v), "ASSERTION FAILURE");
}

TEST(ValueDeathTest, GetU64FromA32Register) {
    value_t v;
    value_init_from_a32_register(&v, A32_REGISTER_R5);
    EXPECT_DEATH(value_get_u64(&v), "ASSERTION FAILURE");
}

TEST(ValueDeathTest, GetA32RegisterFromU1) {
    value_t v;
    value_init_from_u1(&v, true);
    EXPECT_DEATH(value_get_a32_register(&v), "ASSERTION FAILURE");
}

} // namespace pound::jit::ir
