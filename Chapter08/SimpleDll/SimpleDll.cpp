// SimpleDll.cpp : Defines the exported functions for the DLL.
//

#include "framework.h"
#include "SimpleDll.h"


// This is an example of an exported variable
SIMPLEDLL_API int nSimpleDll=0;

// This is an example of an exported function.
SIMPLEDLL_API int fnSimpleDll(void)
{
    return 0;
}

// This is the constructor of a class that has been exported.
CSimpleDll::CSimpleDll()
{
    return;
}
