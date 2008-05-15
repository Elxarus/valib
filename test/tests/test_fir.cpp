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
#include <memory>

///////////////////////////////////////////////////////////////////////////////
// ImpulseResponse base test
///////////////////////////////////////////////////////////////////////////////

TEST(fir_base, "Base FIR classes test")

  const int sample_rate = 48000;
  const double gain = 0.5;

  // Base FIR instance classes test

  const ZeroFIRInstance zero_inst(sample_rate);
  const IdentityFIRInstance identity_inst(sample_rate);
  const GainFIRInstance gain_inst(sample_rate, gain);

  CHECK(zero_inst.sample_rate == sample_rate);
  CHECK(identity_inst.sample_rate == sample_rate);
  CHECK(gain_inst.sample_rate == sample_rate);

  CHECK(zero_inst.type == firt_zero);
  CHECK(identity_inst.type == firt_identity);
  CHECK(gain_inst.type == firt_gain);

  CHECK(zero_inst.length == 1);
  CHECK(identity_inst.length == 1);
  CHECK(gain_inst.length == 1);

  CHECK(zero_inst.data[0] == 0.0);
  CHECK(identity_inst.data[0] == 1.0);
  CHECK(gain_inst.data[0] == gain);

  // Base FIR generators test

  FIRZero zero_gen;
  FIRIdentity identity_gen;
  FIRGain gain_gen(gain);

  const FIRInstance *fir_ptr;

  fir_ptr = zero_gen.make(sample_rate);
  CHECK(fir_ptr != 0);
  CHECK(fir_ptr->sample_rate == sample_rate);
  CHECK(fir_ptr->type == firt_zero);
  CHECK(fir_ptr->length == 1);
  CHECK(fir_ptr->data[0] == 0.0);
  safe_delete(fir_ptr);

  fir_ptr = identity_gen.make(sample_rate);
  CHECK(fir_ptr != 0);
  CHECK(fir_ptr->sample_rate == sample_rate);
  CHECK(fir_ptr->type == firt_identity);
  CHECK(fir_ptr->length == 1);
  CHECK(fir_ptr->data[0] == 1.0);
  safe_delete(fir_ptr);

  fir_ptr = gain_gen.make(sample_rate);
  CHECK(fir_ptr != 0);
  CHECK(fir_ptr->sample_rate == sample_rate);
  CHECK(fir_ptr->type == firt_gain);
  CHECK(fir_ptr->length == 1);
  CHECK(fir_ptr->data[0] == gain);
  safe_delete(fir_ptr);

  // Gain generator test

  int ver;

  ver = gain_gen.version();
  gain_gen.set_gain(0.0);
  CHECK(ver != gain_gen.version());
  CHECK(gain_gen.get_gain() == 0.0);

  fir_ptr = gain_gen.make(sample_rate);
  CHECK(fir_ptr->type == firt_zero);
  CHECK(fir_ptr->sample_rate == sample_rate);
  CHECK(fir_ptr->data[0] == 0.0);
  safe_delete(fir_ptr);

  ver = gain_gen.version();
  gain_gen.set_gain(1.0);
  CHECK(ver != gain_gen.version());
  CHECK(gain_gen.get_gain() == 1.0);

  fir_ptr = gain_gen.make(sample_rate);
  CHECK(fir_ptr->type == firt_identity);
  CHECK(fir_ptr->sample_rate == sample_rate);
  CHECK(fir_ptr->data[0] == 1.0);
  safe_delete(fir_ptr);

  ver = gain_gen.version();
  gain_gen.set_gain(gain);
  CHECK(ver != gain_gen.version());
  CHECK(gain_gen.get_gain() == gain);

  fir_ptr = gain_gen.make(sample_rate);
  CHECK(fir_ptr->type == firt_gain);
  CHECK(fir_ptr->sample_rate == sample_rate);
  CHECK(fir_ptr->data[0] == gain);
  safe_delete(fir_ptr);

TEST_END(fir_base);


TEST(fir_ref, "FIRRef class test")

  const int sample_rate = 48000;
  const double gain = 0.5;

  FIRZero zero_gen;
  FIRGain gain_gen(gain);

  int ver;
  const FIRInstance *fir_ptr;

  // Default constructor test

  FIRRef ref;
  ver = ref.version();
  CHECK(ref.get() == 0);
  CHECK(ref.make(sample_rate) == 0);

  // Init constructor test

  FIRRef ref2(&zero_gen);
  ver = ref2.version();
  CHECK(ref2.get() == &zero_gen);

  // Copy constructor test

  FIRRef ref3(ref2);
  ver = ref3.version();
  CHECK(ref3.get() == &zero_gen);

  // Generator change test

  ver = ref.version();
  ref.set(&zero_gen);
  CHECK(ref.version() != ver);
  CHECK(ref.get() == &zero_gen);

  ver = ref.version();
  ref.set(&gain_gen);
  CHECK(ref.version() != ver);
  CHECK(ref.get() == &gain_gen);

  // Assignment test

  ver = ref.version();
  ref = ref2;
  CHECK(ref.version() != ver);
  CHECK(ref.get() == &zero_gen);

  // Generation test

  fir_ptr = ref.make(sample_rate);
  CHECK(fir_ptr != 0);
  CHECK(fir_ptr->type == firt_zero);
  safe_delete(fir_ptr);

  // Release test

  ver = ref.version();
  ref.release();
  CHECK(ref.version() != ver);
  CHECK(ref.get() == 0);

  fir_ptr = ref.make(sample_rate);
  CHECK(fir_ptr == 0);

TEST_END(fir_ref)



TEST(fir_base_old, "ImpulseResponse base test")

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

TEST_END(fir_base_old);

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
  TEST_FACTORY(fir_ref),
  TEST_FACTORY(fir_base_old),
  TEST_FACTORY(param_ir),
SUITE_END;
