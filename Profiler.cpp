// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : Profiler
// Date  : 14.04.2003
// Abstract:
//    Profiler
// 
// -----------------------------------------------------------------
// Java native interface
#include "cti.h"
#include "ptypes.h"
#include "standard.h"
#include "extended.h"
#include "console.h"
#include "tracer.h"
#include "profiler.h"
#include "monitor.h"

// ----------------------------------------------------------------
// ----------------------------------------------------------------
// int wmain(int argc, wchar_t *argv[]) {
int main(int argc, char **argv)  {
    int	    i = 0;
    int	    j = 0;
    int	    k = 0; 
    bool    aTestCti = false;
    SAP_UC *aChr;
    SAP_cout << TProperties::getInstance()->getVersion(true) << std::endl;

    #if 0
        for (j = 0; j < 50000; j++) {
	    SHERLOK_FCT_BEGIN(cR("package1"), cR("aClass1"), cR("aMethod1"), cR("()V"))
	        SHERLOK_FCT_BEGIN(cR("package2"), cR("aClass2"), cR("aMethod2"), cR("()V"))
		    aChr = (SAP_UC *)mallocU(1000);
	        SHERLOK_FCT_END
	    SHERLOK_FCT_END
        }

        SLEEP(100000);
    #endif    
    // SAP_cout << TProperties::getInstance()->getVersion(true) << std::endl;
    return 0;
}




