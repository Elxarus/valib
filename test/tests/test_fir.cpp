/*
  ImpulseResponse test
  Tests implementations of ImpulseResponse interface:
  * Base tests
    * ZeroIR
    * IdentityIR
    * ImpulseResponseRef
  * ParamIR
*/

#include "fir.h"
#include "fir/param_ir.h"
#include "../suite.h"

///////////////////////////////////////////////////////////////////////////////
// ImpulseResponse base test
///////////////////////////////////////////////////////////////////////////////

TEST(fir_base, "ImpulseResponse base test")

  const int sample_rate = 48000;

  // Base impulse responses creation test
  ZeroIR zero_ir;
  CHECK(zero_ir.get_type(sample_rate) == ir_zero);

  IdentityIR identity_ir;
  CHECK(identity_ir.get_type(sample_rate) == ir_identity);

  // ImpulseResponseRef default constructor test
  ImpulseResponseRef ref_ir;
  CHECK(ref_ir.get_type(sample_rate) == ir_err);

  // ImpulseResponseRef constructor test (ImpulseResponse pointer)
  ImpulseResponseRef ref_ir2(&zero_ir);
  CHECK(ref_ir2.get() == &zero_ir);
  CHECK(ref_ir2.get_type(sample_rate) == ir_zero);

  // ImpulseResponseRef copy constructor test
  ImpulseResponseRef ref_ir3(ref_ir2);
  CHECK(ref_ir3.get() == &zero_ir);
  CHECK(ref_ir3.get_type(sample_rate) == ir_zero);

  // ImpulseResponseRef response change test 1
  int ver = ref_ir.version();
  ref_ir.set(&zero_ir);
  CHECK(ref_ir.get() == &zero_ir);
  CHECK(ver != ref_ir.version());
  CHECK(ref_ir.get_type(sample_rate) == ir_zero);

  // ImpulseResponseRef response change test 2
  ver = ref_ir.version();
  ref_ir.set(&identity_ir);
  CHECK(ref_ir.get() == &identity_ir);
  CHECK(ver != ref_ir.version());
  CHECK(ref_ir.get_type(sample_rate) == ir_identity);

  // ImpulseResponseRef assignment operator test
  ver = ref_ir.version();
  ref_ir = ref_ir2;
  CHECK(ref_ir.get() == &zero_ir);
  CHECK(ver != ref_ir.version());
  CHECK(ver != ref_ir2.version());
  CHECK(ref_ir.get_type(sample_rate) == ir_zero);

TEST_END(fir_base);

///////////////////////////////////////////////////////////////////////////////
// ParamIR test
///////////////////////////////////////////////////////////////////////////////

TEST(param_ir, "ParamIR test")
  int type = 0;
  double f1 = 0.0, f2 = 0.0, df = 0.0, a = 0.0;
  bool norm = true;

  // Default constructor test
  ParamIR ir;
  CHECK(ir.get_type(48000) == ir_err);

  // Setup test
  ir.set(IR_BAND_PASS, 10000, 12000, 500, 100);
  ir.get(&type, &f1, &f2, &df, &a, &norm);
  CHECK(type == IR_BAND_PASS);
  CHECK_DELTA(f1, 10000, 1e-10);
  CHECK_DELTA(f2, 12000, 1e-10);
  CHECK_DELTA(df,   500, 1e-10);
  CHECK_DELTA(a,    100, 1e-10);
  CHECK(norm == false);

  // Full constructor test
  ParamIR ir2(IR_BAND_PASS, 10000, 12000, 500, 100);
  ir2.get(&type, &f1, &f2, &df, &a, &norm);
  CHECK(type == IR_BAND_PASS);
  CHECK_DELTA(f1, 10000, 1e-10);
  CHECK_DELTA(f2, 12000, 1e-10);
  CHECK_DELTA(df,   500, 1e-10);
  CHECK_DELTA(a,    100, 1e-10);
  CHECK(norm == false);

  // Test bounds swapping
  ir.set(IR_BAND_PASS, 12000, 10000, 500, 100);
  ir.get(0, &f1, &f2, 0, 0);

  CHECK(f1 == 10000);
  CHECK(f2 == 12000);

  ir.set(IR_BAND_STOP, 12000, 10000, 500, 100);
  ir.get(0, &f1, &f2, 0, 0);

  CHECK(f1 == 10000);
  CHECK(f2 == 12000);
  
  // Error conditions test
  ir.set(IR_BAND_PASS, 10000, 12000, 500, 100);
  CHECK(ir.get_type(48000) == ir_custom);

  ir.set(IR_BAND_PASS, -10000, 12000, 500, 100);
  CHECK(ir.get_type(48000) == ir_err);

  ir.set(IR_BAND_PASS, 10000, -12000, 500, 100);
  CHECK(ir.get_type(48000) == ir_err);

  ir.set(IR_BAND_PASS, 10000, 12000, -500, 100);
  CHECK(ir.get_type(48000) == ir_err);

  ir.set(IR_BAND_PASS, 10000, 12000, 500, -100);
  CHECK(ir.get_type(48000) == ir_err);

  // Passthrough conditions test
  ir.set(IR_LOW_PASS, 10000, 0, 500, 100);
  CHECK(ir.get_type(21000) == ir_custom);
  CHECK(ir.get_type(19000) == ir_identity);

  ir.set(IR_BAND_STOP, 10000, 12000, 500, 100);
  CHECK(ir.get_type(21000) == ir_custom);
  CHECK(ir.get_type(19000) == ir_identity);
  //
  ir.set(IR_LOW_PASS, 0.49, 0, 0.001, 100, true);
  CHECK(ir.get_type(48000) == ir_custom);

  ir.set(IR_LOW_PASS, 0.51, 0, 0.001, 100, true);
  CHECK(ir.get_type(48000) == ir_identity);
  //
  ir.set(IR_LOW_PASS, 0.49, 0.51, 0.001, 100, true);
  CHECK(ir.get_type(48000) == ir_custom);

  ir.set(IR_LOW_PASS, 0.51, 0.53, 0.001, 100, true);
  CHECK(ir.get_type(48000) == ir_identity);

TEST_END(param_ir);

///////////////////////////////////////////////////////////////////////////////
// Test suite
///////////////////////////////////////////////////////////////////////////////

SUITE(fir, "FIR tests")
  TEST_FACTORY(fir_base),
  TEST_FACTORY(param_ir),
SUITE_END;
