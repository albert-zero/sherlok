/**
Definition for C++ sherlok monitor

Copyright (C) 2015  Albert Zedlitz

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#if !defined (CCQ_COV_RUN_H)
    #define CCQ_COV_RUN_H 1
    #define CCQ_COV_MAKE

    /*CCQ_COV_FILE_OFF*/
    /**
        * SB2649 2011-02-11: Adaptions for sherlok instrumentation.
        * SB2671 2011-03-04: Adapt arguments for sherlok macros.
        * \brief In a first step we avoid the inclusion of cti.h as
        * it used jni.h. This header needs to be copyied with the
        * help of macros but due to the removal of mapro the 
        * environment isn't fully available any longer. In 720 
        * this will of course not be a problem. 
        * The macros shall now have the correct number of parameters:
        * FCT_BEGIN with 4 parameters
        * MAIN_BEGIN with 2 parameters
        * SB2688 2011-03-15: Enable sherlok instrumentation by copying everything needed.
        * \brief The internal function ccQprepareSherlokInstrumentation () takes care
        * that the header files needed (cti.h, jni.h and jni_md.h) will be available.
        * Therefore we can include cti.h now and specify the full macro content.
        * TODO Clean up old macro trial versions.
    */
#   include "cti.h"

    /* @(#) $Id: //Douala/DoualaLayer/dev/v8sherlok/ccqcovrun.h#7 $ SAP */
    /**********************************************************************
     *
     * ccQ, sapucchk, sapuccnv
     * - Unicode conversion
     * - Unicode checks
     * - Lint checks
     *
     * Copyright (C) SAP AG, 1997 - 2011
     *
     * This program is free software; you can redistribute it and/or
     * modify it under the terms of the GNU General Public License
     * as published by the Free Software Foundation; either version 2
     * of the License, or (at your option) any later version.
     *
     * This program is distributed in the hope that it will be useful,
     * but WITHOUT ANY WARRANTY; without even the implied warranty of
     * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     * GNU General Public License for more details.
     *
     * You should have received a copy of the GNU General Public License
     * along with this program; if not, write to the Free Software
     * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
     *
     * Contact: SAP AG, Neurottstrasse 16, D-69190 Walldorf, GERMANY
     *
     **********************************************************************
     *
     * ccq coverage runtime header
     * definitions for instrumented source files
     *
     **********************************************************************/
    
    /* 2009-02-13: GUID solution runtime............................*/
    
    /* instrumentation macros */
    #define CCQ_COV_FCT_BEGIN(fctname)                               \
    {                                                                \
        static CCQ_LLONG* pFctCallCounter=0;                         \
        /* if counter has been fetched already, increment counter */ \
        if( pFctCallCounter!=0 ) {                                   \
            (*pFctCallCounter)++;                                      \
        /* else fetch counter */                                     \
        } else  {                                                    \
            pFctCallCounter = ccQCovGetFctCallCounterTry(__FILE__ , __LINE__ , fctname);   \
        }                                                            \
    {
    
    #define CCQ_COV_FCT_BEGIN_GUID(guid, fctname)                    \
    {                                                                \
        ccQCovFctCallIncrement(guid);                                \
    {
    
    
    #define CCQ_COV_FCT_END_GUID }}
    #define CCQ_COV_FCT_END }}
    
    #define CCQ_COV_MAIN_BEGIN {{
    #define CCQ_COV_MAIN_END }}
        
    
    
    /* QUE1447 2008-09-04: Dummy declaration of CCQ_COV_BLK_BEGIN/END ....*/
    #define CCQ_COV_BLK_BEGIN(fctname, filename, lno)                \
    {                                                                \
    {
    #define CCQ_COV_BLK_END }}
    
    /**
     * SB1891 2009-02-24: Add first version of branch coverage related macros.
     * SB1901 2009-03-02: Introduce macro CCQ_COV_ELSE
     * \brief In the first version we use "old" style like the block related
     * macros. After switching to the "guid-solution" there might be 
     * adaptations necessary.
     */
    #define CCQ_COV_CLOSE_PAREN )
    #define CCQ_COV_SEMICOLON ;
    #define CCQ_COV_KEYWORD_ELSE else
    #define CCQ_COV_KEYWORD_ELSE_GEN else
    
    #define CCQ_COV_IF_BEGIN(a, b, c) {{
    #define CCQ_COV_IF_END }}
    
    #define CCQ_COV_ELSE_BEGIN(a, b, c) {{
    #define CCQ_COV_ELSE_END }}
    
    #define CCQ_COV_IF_BLK_BEGIN(a, b, c) {{
    #define CCQ_COV_IF_BLK_END }}
    
    #define CCQ_COV_ELSE_BLK_BEGIN(a, b, c) {{
    #define CCQ_COV_ELSE_BLK_END }}
    
    #define CCQ_COV_WHILE_BEGIN(a, b, c) {{
    #define CCQ_COV_WHILE_END }}
    
    #define CCQ_COV_WHILE_BLK_BEGIN(a, b, c) {{
    #define CCQ_COV_WHILE_BLK_END }}
    
    #define CCQ_COV_DO_BEGIN(a, b, c) {{
    #define CCQ_COV_DO_END }}
    
    #define CCQ_COV_DO_BLK_BEGIN(a, b, c) {{
    #define CCQ_COV_DO_BLK_END }}
    
    #ifdef __cplusplus
        #define CCQ_COV_EXTERN_C extern "C"
        #define CCQ_BEG_EXTERN_C extern "C" {
        #define CCQ_END_EXTERN_C            }
    #else
        #define CCQ_COV_EXTERN_C extern 
        #define CCQ_BEG_EXTERN_C
        #define CCQ_END_EXTERN_C
    #endif
    
    /* SB1011 2007-08-14: Remove inclusion of limits.h and eliminate usage of long type for coverage counter. Use dedicated 64 types on each platform.
     * As we run into trouble for some projects when using coverage analyzer also
     * on UNIX platforms we decided
     * 1) to eliminate limits.h since including it here sometimes changes the
     * behaviour of the follow up inclusions such that e.g. functions are not
     * declared and
     * 2) to use only the dedicated 64bit types offered by each base platform to
     * avoid long type as demanded by 64 bit rules.
     * ACHTUNG: This change corresponds to a similar change in ccqfctinfo.h where
     * the output handling of the coverage analyzer is done (and therefore
     * especially the format is needed).
     *
    */
    /* 64-bit integers */
    /* 2009-02-13: GUID solution runtime ..............................*/ 
    #ifdef _WINDOWS
        typedef __int64 CCQ_LLONG;
        #define CCQ_LLONG_FORMAT "I64d"
    #else
        typedef long long CCQ_LLONG;
        #define CCQ_LLONG_FORMAT "lld"
    #endif
    
    /* JL1911 2009-03-09: exchange guid-2-parts with guid-1-part ........ */
    /* max branch list */
    #define CCQ_COV_MAX_BRN (1<<20) /* must be synchronic to CCQ_COV_MAX_BRN in 
                                       in ccqcov.c */
    /* max output branches */
    #define TWO_BILLION 2000000000 
    
    /* instrumentation code at function begin */
    CCQ_COV_EXTERN_C void ccQCovFctCallIncrement(unsigned guid);
    
    CCQ_COV_EXTERN_C CCQ_LLONG* ccQCovGetFctCallCounterTry( 
                const char* sSrcName, unsigned int sRow, const char* sFctName );
    
    CCQ_COV_EXTERN_C void ccQCovResetFctCountStat(void);
    
    CCQ_COV_EXTERN_C void ccQCovGetFctCounterBySymbolStat(unsigned index, 
                                        char** const inclName,
                                        unsigned * row,
                                        char** const brnName, 
                        int * const count);
    CCQ_COV_EXTERN_C void ccQCovGetFctCounterByIndexStat(unsigned index, 
                         int * const count);
    
    CCQ_COV_EXTERN_C int ccQCovIsShmAvailableStat(void);
    
    
    #if defined (__cplusplus)
    #endif    
#endif /*CCQ_COV_RUN_H*/
