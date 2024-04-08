#include "fit_base.h"

//prevent usage of unvalid FIT CODE
FIT_DEFINE(FFFFFFFF, INVALID_CODE)

bool FitBase_ActivateTest(FitBase_CmdType cmd, const FitBase_CmdArgs* args, uint32_t Repeat)
{
    bool ret = false;

    FIT_ENTER_CRITICAL();

    if(FIT_IS_INACTVE())
    {
        FitTest.Repeat = Repeat;
        FitTest.Args = *args;

        FIT_ACTIVATE(cmd);

        ret = true;
    }
    else if(FIT_IS_ACTIVE(cmd))
    {
        //the same FIT is already running
    }
    else
    {
        //some other FIT is already running
    }

    FIT_LEAVE_CRITICAL();
    
    return ret;
}

//return true if FIT is to be kept active
//return false if FIT shall be terminated
bool FitBase_ActivatedProcess(void)
{
    //if we dont use steps (== _INIT) or last step (== _RESET), we may declare a FIT complete and increment the Repeat Count
    if(FitTest.InvStep == FIT_STEP_INIT || FitTest.InvStep == FIT_STEP_RESET)
        FitTest.RepeatCnt++;

    //if condition for infinite repeat met, don't deactivate the fit
    if(FitTest.Repeat == FIT_REPEAT_INFINITE)
        return true;

    if(FitTest.RepeatCnt >= FitTest.Repeat)
        return false;
 
    return true;
}

void FitBase_Process(void)
{
    if(!FIT_INITIALIZED(FitTest.Cmd)) {
        FIT_DOINIT();
    }

    //keep track of last executed for debugging
    FitTest.CmdCache = FitTest.Cmd;

    PROC_STEPS(FitTest.StepMax);

    if(!FitBase_ActivatedProcess())
        FIT_DEACTIVATE();

    //if increment is FIT_MANUAL_STEP = 0, wont do a thing
    DO_STEP();
}

bool FitBase_Init(void)
{
    //check the correct placement of the FitData varibale to be available at a specific memory location
    const FitBaseTest* const fbt = &FitTest;
    if(fbt != FitBase_GetTestArea())
    {
        return false;
    }

    FIT_ACTIVATE(FITCMD_INVALID);
    return true;
}
