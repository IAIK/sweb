#ifdef _ERROR_HANDLERS_H_
#error This file may not be included more than once
#else
#define _ERROR_HANDLERS_H_




constexpr const char* errors[32] = {
  "#DE: Divide by Zero",
  "",
  "",
  "",
  "#OF: Overflow (INTO Instruction)",
  "#BR: Bound Range Exceeded",
  "#OP: Invalid OP Code",
  "#NM: FPU Not Avaiable or Ready",
  "#DF: Double Fault",
  "#MF: FPU Segment Overrun",
  "#TS: Invalid Task State Segment (TSS)",
  "#NP: Segment Not Present (WTF ?)",
  "#SS: Stack Segment Fault",
  "#GF: General Protection Fault",
  "",
  "",
  "#MF: Floting Point Error",
  "#AC: Alignment Error (Unaligned Memory Reference)",
  "#MC: Machine Check Error",
  "#XF: SIMD Floting Point Error"
};

#endif
