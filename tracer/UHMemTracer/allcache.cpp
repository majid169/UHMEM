/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2017 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*! @file
 *  This file contains an ISA-portable PIN tool for functional simulation of
 *  instruction+data TLB+cache hierarchies
 */


#include "pin.H"
typedef UINT32 CACHE_STATS;


#include "pin_cache.H"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <string>


namespace DL1
{
    // 2nd level unified cache: 2 MB, 64 B lines, direct mapped
    const UINT32 cacheSize = 8*MEGA;
    const UINT32 lineSize = 64;
    const UINT32 associativity = 1;
    const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

    const UINT32 max_sets = cacheSize / (lineSize * associativity);

    typedef CACHE_DIRECT_MAPPED(max_sets, allocation) CACHE;
}
LOCALVAR DL1::CACHE dl1("Large L1 Unified Cache", DL1::cacheSize, DL1::lineSize, DL1::associativity);





#define NUM_INSTR_DESTINATIONS 2
#define NUM_INSTR_SOURCES 4

typedef struct trace_instr_format {
    unsigned long inst;
    unsigned long long int addr;  // instruction pointer (program counter) value
    
} trace_instr_format_t;

/* ================================================================== */
// Global variables 
/* ================================================================== */

UINT64 instrCount = 0;
unsigned long inst = 0;

FILE* out;

bool output_file_closed = false;
bool tracing_on = false;
bool record_me =false;

trace_instr_format_t curr_instr;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool", "o", "champsim.trace", 
        "specify file name for Champsim tracer output");

KNOB<UINT64> KnobSkipInstructions(KNOB_MODE_WRITEONCE, "pintool", "s", "0", 
        "How many instructions to skip before tracing begins");

KNOB<UINT64> KnobTraceInstructions(KNOB_MODE_WRITEONCE, "pintool", "t", "1000000", 
        "How many instructions to trace");

void Fini(int code, VOID * v)
{
    if(!output_file_closed) 
    {
        fclose(out);
        output_file_closed = true;
    }
}






void InsRef(ADDRINT addr)
{
    instrCount++;
    inst++;

    if(instrCount > KnobSkipInstructions.Value()) 
    {
        tracing_on = true;

        if(instrCount > (KnobTraceInstructions.Value()+KnobSkipInstructions.Value()))
            tracing_on = false;
    }

    if(!tracing_on) 
        return;

}

void MemRefMulti(ADDRINT addr, UINT32 size, CACHE_BASE::ACCESS_TYPE accessType)
{
    
    if(instrCount > KnobSkipInstructions.Value()) 
    {
        tracing_on = true;

        if(instrCount > (KnobTraceInstructions.Value()+KnobSkipInstructions.Value()))
            tracing_on = false;
    }

    if(!tracing_on) 
        return;


    const BOOL dl1Hit = dl1.Access(addr, size, accessType);
    if ( ! dl1Hit){
        curr_instr.addr = (unsigned long long int)addr;
        curr_instr.inst = inst;
        inst=0;
        record_me = true;
    }
}

void MemRefSingle(ADDRINT addr, UINT32 size, CACHE_BASE::ACCESS_TYPE accessType)
{
    if(instrCount > KnobSkipInstructions.Value()) 
    {
        tracing_on = true;

        if(instrCount > (KnobTraceInstructions.Value()+KnobSkipInstructions.Value()))
            tracing_on = false;
    }

    if(!tracing_on) 
        return;
    const BOOL dl1Hit = dl1.Access(addr, size, accessType);
    if ( ! dl1Hit){
        curr_instr.addr = (unsigned long long int)addr;
        curr_instr.inst = inst;
        inst=0;
        record_me = true;
    }
}

template <typename T>
std::string ToString(T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}

void EndInstruction()
{
    //printf("%d]\n", (int)instrCount);

    //printf("\n");

    if(instrCount > KnobSkipInstructions.Value())
    {
        tracing_on = true;

        if(instrCount <= (KnobTraceInstructions.Value()+KnobSkipInstructions.Value()) )
        {
            // keep tracing
            if(record_me){
                //fwrite(&curr_instr, sizeof(trace_instr_format_t), 1, out);
                fprintf(out, "%lu %llu\n", curr_instr.inst, curr_instr.addr);
               // out<<std::to_string(curr_instr.addr)<<" "<<std::to_string(curr_instr.inst)<<end;
               // std::cout<<curr_instr.addr<<" "<<curr_instr.inst<<endl;
                record_me = false;   
            }
            
        }
        else
        {
            tracing_on = false;
            // close down the file, we're done tracing
            if(!output_file_closed)
            {
                fclose(out);
                output_file_closed = true;
            }

            exit(0);
        }
    }
}



LOCALFUN VOID Instruction(INS ins, VOID *v)
{
    // all instruction fetches access I-cache
    INS_InsertCall(
        ins, IPOINT_BEFORE, (AFUNPTR)InsRef,
        IARG_INST_PTR,
        IARG_END);

    if (INS_IsMemoryRead(ins) && INS_IsStandardMemop(ins))
    {
        const UINT32 size = INS_MemoryReadSize(ins);
        const AFUNPTR countFun = (size <= 4 ? (AFUNPTR) MemRefSingle : (AFUNPTR) MemRefMulti);

        // only predicated-on memory instructions access D-cache
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, countFun,
            IARG_MEMORYREAD_EA,
            IARG_MEMORYREAD_SIZE,
            IARG_UINT32, CACHE_BASE::ACCESS_TYPE_LOAD,
            IARG_END);
    }

    if (INS_IsMemoryWrite(ins) && INS_IsStandardMemop(ins))
    {
        const UINT32 size = INS_MemoryWriteSize(ins);
        const AFUNPTR countFun = (size <= 4 ? (AFUNPTR) MemRefSingle : (AFUNPTR) MemRefMulti);

        // only predicated-on memory instructions access D-cache
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, countFun,
            IARG_MEMORYWRITE_EA,
            IARG_MEMORYWRITE_SIZE,
            IARG_UINT32, CACHE_BASE::ACCESS_TYPE_STORE,
            IARG_END);
    }
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)EndInstruction, IARG_END);
}

GLOBALFUN int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);

    const char* fileName = KnobOutputFile.Value().c_str();

    out = fopen(fileName, "ab");
    if (!out) 
    {
        cout << "Couldn't open output trace file. Exiting." << endl;
        exit(1);
    }





    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0; // make compiler happy
}
