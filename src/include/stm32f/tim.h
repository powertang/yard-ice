
/* TIM1&TIM8 break and dead-time register */
#define TIMX_BDTR 0x44


/* Bit 15 - Main output enable */
#define MOE (1 << 15)
/* This bit is cleared asynchronously by hardware as soon as the break 
   input is active. It is set by software or automatically depending on the 
   AOE bit. It is acting only on the channels which are configured in output.
   0: OC and OCN outputs are disabled or forced to idle state.
   1: OC and OCN outputs are enabled if their respective enable bits are 
   set (CCxE, CCxNE in TIMx_CCER register).
   See OC/OCN enable description for more details (Section 13.4.9: TIM1&TIM8
   capture/compare enable register (TIMx_CCER) on page 344). */

/* Bit 14 - Automatic output enable */
#define AOE (1 << 14)
/* 0: MOE can be set only by software
   1: MOE can be set by software or automatically at the next update event 
   (if the break input is not be active)
   Note: This bit can not be modified as long as LOCK level 1 has been 
   programmed (LOCK bits in TIMx_BDTR register). */

/* Bit 13 - Break polarity */
#define BKP (1 << 13)
/* 0: Break input BRK is active low
   1: Break input BRK is active high
   Note: This bit can not be modified as long as LOCK level 1 has been 
   programmed (LOCK bits in TIMx_BDTR register).
   Note: Any write operation to this bit takes a delay of 1 APB clock 
   cycle to become effective. */

/* Bit 12 - Break enable */
#define BKE (1 << 12)
/* 0: Break inputs (BRK and CCS clock failure event) disabled 
   1: Break inputs (BRK and CCS clock failure event) enabled
   Note: This bit cannot be modified when LOCK level 1 has been 
   programmed (LOCK bits in TIMx_BDTR register).
   Note: Any write operation to this bit takes a delay of 1 APB clock 
   cycle to become effective. */

/* Bit 11 - Off-state selection for Run mode */
#define OSSR (1 << 11)
/* This bit is used when MOE=1 on channels having a complementary output 
   which are configured as outputs. OSSR is not implemented if no 
   complementary output is implemented in the timer.
   See OC/OCN enable description for more details (Section 13.4.9: 
   TIM1&TIM8 capture/compare enable register (TIMx_CCER) on page 344).
   0: When inactive, OC/OCN outputs are disabled (OC/OCN enable output 
   signal=0).
   1: When inactive, OC/OCN outputs are enabled with their inactive level 
   as soon as CCxE=1 or CCxNE=1. Then, OC/OCN enable output signal=1
   Note: This bit can not be modified as soon as the LOCK level 2 has 
   been programmed (LOCK bits in TIMx_BDTR register). */

/* Bit 10 - Off-state selection for Idle mode */
#define OSSI (1 << 10)
/* This bit is used when MOE=0 on channels configured as outputs.
   See OC/OCN enable description for more details (Section 13.4.9: TIM1&TIM8
   capture/compare enable register (TIMx_CCER) on page 344).
   0: When inactive, OC/OCN outputs are disabled (OC/OCN enable 
   output signal=0).
   1: When inactive, OC/OCN outputs are forced first with their idle level 
   as soon as CCxE=1 or CCxNE=1. OC/OCN enable output signal=1)
   Note: This bit can not be modified as soon as the LOCK level 2 has been 
   programmed (LOCK bits in TIMx_BDTR register). */

/* Bits [9..8] - Lock configuration */
#define LOCK_MSK (((1 << (1 + 1)) - 1) << 8)
#define LOCK_SET(VAL) (((VAL) << 8) & LOCK_MSK)
#define LOCK_GET(REG) (((REG) & LOCK_MSK) >> 8)
/* These bits offer a write protection against software errors.
   00: LOCK OFF - No bit is write protected.
   01: LOCK Level 1 = DTG bits in TIMx_BDTR register, OISx and OISxN bits 
   in TIMx_CR2 register and BKE/BKP/AOE bits in TIMx_BDTR register can no 
   longer be written.
   10: LOCK Level 2 = LOCK Level 1 + CC Polarity bits (CCxP/CCxNP bits 
   in TIMx_CCER register, as long as the related channel is configured in 
   output through the CCxS bits) as well as OSSR and OSSI bits can 
   no longer be written.
   11: LOCK Level 3 = LOCK Level 2 + CC Control bits (OCxM and OCxPE bits in
   TIMx_CCMRx registers, as long as the related channel is configured in 
   output through the CCxS bits) can no longer be written.
   Note: The LOCK bits can be written only once after the reset. Once the 
   TIMx_BDTR register has been written, their content is frozen 
   until the next reset. */

/* Bits [7..0] - Dead-time generator setup */
#define DTG_MSK (((1 << (7 + 1)) - 1) << 0)
#define DTG_SET(VAL) (((VAL) << 0) & DTG_MSK)
#define DTG_GET(REG) (((REG) & DTG_MSK) >> 0)
/* This bit-field defines the duration of the dead-time inserted between 
   the complementary outputs. DT correspond to this duration.
   DTG[7:5]=0xx => DT=DTG[7:0]x tdtg with tdtg=tDTS.
   DTG[7:5]=10x => DT=(64+DTG[5:0])xtdtg with Tdtg=2xtDTS.
   DTG[7:5]=110 => DT=(32+DTG[4:0])xtdtg with Tdtg=8xtDTS.
   DTG[7:5]=111 => DT=(32+DTG[4:0])xtdtg with Tdtg=16xtDTS.
   Example if TDTS=125ns (8MHz), dead-time possible values are:
   0 to 15875 ns by 125 ns steps,
   16 us to 31750 ns by 250 ns steps,
   32 us to 63us by 1 us steps,
   64 us to 126 us by 2 us steps
   Note: This bit-field can not be modified as long as LOCK level 1, 2 or 
   3 has been programmed (LOCK bits in TIMx_BDTR register). */

/* TIM1&TIM8 repetition counter register */
#define TIMX_RCR 0x30

/* [15..8] Reserved, must be kept at reset value. */

/* Bits [7..0] - Repetition counter value */
#define REP_MSK (((1 << (7 + 1)) - 1) << 0)
#define REP_SET(VAL) (((VAL) << 0) & REP_MSK)
#define REP_GET(REG) (((REG) & REP_MSK) >> 0)
/* These bits allow the user to set-up the update rate of the compare 
   registers (i.e. periodic transfers from preload to active registers) 
   when preload registers are enable, as well as the update interrupt 
   generation rate, if this interrupt is enable.
   Each time the REP_CNT related downcounter reaches zero, an update event 
   is generated and it restarts counting from REP value. As REP_CNT is 
   reloaded with REP value only at the repetition update event U_RC, any 
   write to the TIMx_RCR register is not taken in account until the next 
   repetition update event.
   It means in PWM mode (REP+1) corresponds to:
   – the number of PWM periods in edge-aligned mode
   – the number of half PWM period in center-aligned mode. */

