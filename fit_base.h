#ifndef __FIT_BASE_H__
#define __FIT_BASE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define static_assert4(cond, msg) extern char (*msg(void)) [1-2*!(cond)]
#define static_assert3(cond, msg) extern char (*msg(void)) [sizeof(char[1-2*!(cond)])]
#define static_assert2(cond, msg) typedef char msg[(cond) ? 1 : -1]
#define static_assert1(cond, msg) assert(cond)

////

//FITs enabled by default
#ifndef FIT_ENABLED
#define FIT_ENABLED 1
#endif

#define FIT_STATIC_ASSERT(cond, msg) static_assert4(cond, msg)

// the FIT command type
typedef uint32_t FitBase_CmdType;
#define FITCMD_INVALID 0xFFFFFFFFU

// repeat FIT infinitely
#define FIT_REPEAT_INFINITE 0U

// the step init and reset value
#define FIT_STEP_INIT 0u  //used when initializing data structures
#define FIT_STEP_RESET 1u //used when resetting the steps to max_steps

// the FIT Parameters available
typedef struct
{
  uint32_t P1;
  uint32_t P2;
  uint32_t P3;
  uint32_t P4;
} FitBase_CmdArgs;

#define FIT_ARGS(p1, p2, p3, p4) (FitBase_CmdArgs){ .P1 = (p1), .P2 = (p2), .P3 = (p3), .P4 = (p4), }

// to global FIT instance data, only one FIT is ever active a time
typedef struct
{
  FitBase_CmdArgs Args; //the Args for the Command

  uint32_t RepeatCnt; //keeps repeating each test run, a single test can consist of multiple steps, then repeat count only increments once all steps completed
  uint32_t Repeat; //repeat 0 == infinite repeat, see FIT_REPEATCNT_INFINITE, RepeatCnt increments regardless of this setting

  FitBase_CmdType CmdCache; //last executed FIT command, needs to be invalidated prior to setting a new one
  FitBase_CmdType Cmd; //currently active FIT command

  uint32_t Step; //the current FIT step [0..max_step-1], 0 if no steps declared
  uint32_t StepMax; //max steps count

  int32_t  StepInc; //internal, increment of FIT step value
  uint32_t InvStep; //internal, count down of FIT steps, FIT_STEP_INIT means no steps active
} FitBaseTest; //sizeof() = 0x30 = 48 byte

// essential for access from everywhere
extern FitBaseTest FitTest;

//can be used inside the FIT code too (to prematurely stop a FIT) or to switch to a new one (carefull with params and repeat)
#define FIT_DEACTIVATE() do { FitTest.Cmd = FITCMD_INVALID; } while(0)
#define FIT_ACTIVATE(cmd) do { FitTest.CmdCache = FITCMD_INVALID; FitTest.Cmd = cmd; } while(0)

////

// in multi-thread, override
#ifndef FIT_ENTER_CRITICAL
#define FIT_ENTER_CRITICAL() do {} while(0)
#endif

// in multi-thread, override
#ifndef FIT_LEAVE_CRITICAL
#define FIT_LEAVE_CRITICAL() do {} while(0)
#endif

bool FitBase_ActivatedProcess(void);
void FitBase_Process(void);

// main FIT activation Interface
bool FitBase_ActivateTest(FitBase_CmdType cmd, const FitBase_CmdArgs* args, uint32_t Repeat);

// Call once before using FITs
bool FitBase_Init(void);

// needed by FitBase_Init, to check placement
extern const FitBaseTest* FitBase_GetTestArea(void);

////

// Increment Values for the FIT steps
#define FIT_NEXT_STEP  (-1)
#define FIT_MANUAL_STEP (0)
#define FIT_PREV_STEP  (+1)

// internal, initializing on first call of FIT
#define FIT_DOINIT() \
    FitTest.RepeatCnt = 0; \


// some usefull conditions
#define FIT_IS_INACTVE() (FITCMD_INVALID == FitTest.Cmd)
#define FIT_IS_ACTIVE(cmd) (cmd == FitTest.Cmd)
#define FIT_IS_ANY_ACTIVE() (!FIT_IS_INACTVE())

#define FIT_INITIALIZED(cmd) (cmd == FitTest.CmdCache)
#define FIT_STARTED(cmd) (FIT_IS_ACTIVE(cmd) && (!FIT_INITIALIZED(cmd)))
#define FIT_COMPLETED(cmd) (FIT_IS_INACTVE() && FIT_INITIALIZED(cmd))

// some conditions for internal use
#define FIT_ACTIVE() (FIT_IS_ACTIVE(fit_cmd))
#define FIT_INIT_NEED() ((!FIT_INITIALIZED(fit_cmd)))

// this one always goes first, compiler will make fit_cmd a constant, no stack usage
// OTOH, using the global storgae only if FIT is matched
// check for cmd being active and reinit if needed
// assumes no STEP definition, until HAS_STEPS() is used
// if is missing a closing brace on purpose, is completed during BODY construction
#define CMD(cmd) \
    if(FIT_IS_ACTIVE(cmd)) { \
        __attribute__ ((unused)) const FitBase_CmdType fit_cmd = cmd; \
        __attribute__ ((unused)) const bool fit_init = FIT_INIT_NEED(); \
        FitTest.StepMax = FIT_STEP_INIT; \
        FitTest.Step = FIT_STEP_INIT; \
        FitTest.StepInc = FIT_MANUAL_STEP; \


// the step mechanism generates only values [0 ... max_step-1]
#define FIT_STEP(max_steps) (max_steps - FitTest.InvStep)

// has to be executed if the FIT is actually active, and use post increment
// depends on CMD() usage previously, overrides initialization from CMD()
#define HAS_STEPS(max_step, ...) \
        FitTest.StepMax = max_step; \
        if(fit_init) FitTest.InvStep = FitTest.StepMax; \
        FitTest.Step = FIT_STEP(max_step); \
        __attribute__ ((unused)) const uint32_t fit_step_max = FitTest.StepMax; \
        __attribute__ ((unused)) const uint32_t fit_step = FitTest.Step; \
        FitTest.StepInc = FIT_NEXT_STEP; \
        __VA_ARGS__ \


// define test to manage steps inside body. use it in the HAS_STEPS macro or after it
#define SELF_STEPS() \
        (void)fit_step_max; \
        FitTest.StepInc = FIT_MANUAL_STEP; \
        __attribute__ ((unused)) const uint32_t fit_step_inc_override = FIT_MANUAL_STEP; \


// define run steps inside body in reverse order. use it in the HAS_STEPS macro or after it //TODO, init value
#define REV_STEPS() \
        (void)fit_step_max; \
        __attribute__ ((unused)) const bool fit_step_override = true; \
        FitTest.StepInc = FIT_PREV_STEP; \
        __attribute__ ((unused)) const uint32_t fit_step_inc_override = FIT_MANUAL_STEP; \


// conditions usable of steps checking in FIT conditions
#define IS_STEP(step) (fit_step == step)
#define IS_SEQ(step, max_step) (((step) == (fit_step)) && ((max_step) == (FitTest.StepMax)))
#define NO_COND true

// FIT step management, for manual chenges inside FIT body
#define DO_STEP()   (FitTest.InvStep += FitTest.StepInc)
#define NEXT_STEP() (FitTest.InvStep += FIT_NEXT_STEP)
#define PREV_STEP() (FitTest.InvStep += FIT_PREV_STEP)
#define SET_STEP(step) (FitTest.InvStep = FitTest.MaxStep - step)

// define the value for the InvStep for the next round
#define PROC_STEPS(max_step) \
    if(max_step > 0) \
    { \
        if(FitTest.InvStep < FIT_STEP_RESET) { FitTest.InvStep = max_step; } \
        else if(FitTest.InvStep > max_step) { FitTest.InvStep = FIT_STEP_RESET; } \
    } \

////

// cond needs to be handled here (as opposed to leaving it for the better readable used user code
// cause FitBase_Process() must not be executed unless condition is really met
// it handles repeat count and step processing
#define FIT_BODY(head, cond) \
head \
  __attribute__ ((unused)) FitBase_CmdArgs* const fit_args = &FitTest.Args; \
  if(cond) \
  { \
    FitBase_Process(); \

//EXPRESSION WOULD GO HERE//

// contains the closing brace from the CMD() as always first part of the definition
#define FIT_TAIL() \
  } \
}
//  if(false) (void)fit_cmd; \

////

#if FIT_ENABLED

#define FIT(head, cond, expr) \
    FIT_BODY(head, cond) \
      expr; \
    FIT_TAIL()

////

#define FIT_BEGIN(head, cond) \
    FIT_BODY(head, cond)

#define FIT_END(_cmd) \
      FIT_STATIC_ASSERT(fit_cmd == _cmd, sa ## _cmd); \
    FIT_TAIL()

#else //FIT_ENABLED

#define FIT(head, cond, expr)
#define FIT_BEGIN(head, cond)
#define FIT_END(_cmd)

#endif //FIT_ENABLED

////

// data that can be used inside the FIT test scope and also inside the condition space
/*
fit_cmd
fit_init
fit_args.P*
fit_step (if HAS_STEPS is used)
fit_step_max (if HAS_STEPS is used)
*/

////////

// the helper to define the FIT Command Codes
// use compiler to check for multiple definitions of name or code
// if the same code and name is needed for a FIT spread over multiple modules, use FIT_DEFINE_MULTI, with a unique id

#define FIT_DEFINE_MULTI(cmd, id, name) \
extern const FitBase_CmdType FIT_##cmd##id = 0x##cmd; \
extern const FitBase_CmdType FIT_##name##id = FIT_##cmd##id; \
static __attribute__ ((unused)) const FitBase_CmdType FIT_##name = FIT_##cmd##id; \

// main FIT code definition helper
#define FIT_DEFINE(cmd, name) FIT_DEFINE_MULTI(cmd, 0, name)

////

#ifdef __cplusplus
}
#endif

#endif /* __FIT_BASE_H__ */
