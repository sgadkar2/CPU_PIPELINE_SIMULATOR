/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

int integerFUCounter;
int mulFUCounter;
int loadStoreFUCounter;

int priorityQueue[MAX_QUEUE_SIZE];
int rear = - 1;
int front = 0;
int itemCount = 0;

void enqueue(int data) {

   if(itemCount !=  MAX_QUEUE_SIZE) {
	
      if(rear == MAX_QUEUE_SIZE-1) {
         rear = -1;            
      }       

      priorityQueue[++rear] = data;
      itemCount++;
   }
}

int dequeue() {
   int data = priorityQueue[front++];
	
   if(front == MAX_QUEUE_SIZE) {
      front = 0;
   }
	
   itemCount--;
   return data;  
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_LDR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1, stage->rs2);
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;
        }

        case OPCODE_LOAD:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_STORE:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }

        case OPCODE_BZ:
        case OPCODE_BNZ:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_HALT:
        case OPCODE_NOP:
        {
            printf("%s", stage->opcode_str);
            break;
        }

        case OPCODE_ADDL:
        case OPCODE_SUBL:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_CMP:
        {
            printf("%s,R%d,R%d", stage->opcode_str, stage->rs1, stage->rs2);
            break;
        }

        case OPCODE_STR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rs3, stage->rs1,
                   stage->rs2);
            break;
        }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

static void
print_empty_content(const char *name, const CPU_Stage *stage)
{
    //printf("%-15s: pc(%d) ", name, stage->pc);
    printf("%-15s: ", name);
    printf("EMPTY");
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
void
print_reg_file(APEX_CPU *cpu)
{
    int i;

    //printf("----------\n%s\n----------\n", "Registers:");
    printf("==========STATE OF ARCHITECTURAL REGISTER FILE==============\n");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d][%-3d] ", i, cpu->reg[i].regs, cpu->reg[i].valid);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d][%-3d] ", i, cpu->reg[i].regs, cpu->reg[i].valid);
    }

    printf("\n");
}

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu, int printMsg)
{

   // storing data in memory location 70
   // cpu->data_memory[70] = 24;
    APEX_Instruction *current_ins;

    if(cpu->is_waiting_decode == 1){
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.rs3 = current_ins->rs3;
        cpu->fetch.imm = current_ins->imm;
        if(printMsg == 1){
          print_stage_content("Fetch", &cpu->fetch);
        }        
        return;
    }

    if (cpu->fetch.has_insn) 
    {
        //cpu->is_waiting_fetch = 1;
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.rs3 = current_ins->rs3;
        cpu->fetch.imm = current_ins->imm;

        /* Update PC for next instruction */
        cpu->pc += 4;

        /* Copy data from fetch latch to decode latch*/
        if(cpu->is_waiting_decode == 0){
            cpu->decode = cpu->fetch;
        }

        if (printMsg == 1)
        {
           print_stage_content("Fetch", &cpu->fetch);
        }
        
        /* Stop fetching new instructions if HALT is fetched */
        if (cpu->fetch.opcode == OPCODE_HALT)
        {
            cpu->fetch.has_insn = FALSE;
        }

    }else{
        if (printMsg == 1)
        {
           print_empty_content("Fetch", &cpu->fetch);
        }
    }
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu, int printMsg)
{
    if (cpu->decode.has_insn)
    {
        cpu->is_waiting_fu = 0;
        cpu->is_waiting_decode = 1;
        int hasDest = 0;
        int validInput = 0;
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
            case OPCODE_MUL:
            {
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    cpu->decode.rs1_value = cpu->reg[cpu->decode.rs1].regs;
                }

                if(cpu->reg[cpu->decode.rs2].valid == 0){
                    cpu->decode.rs2_value = cpu->reg[cpu->decode.rs2].regs;
                }

                //cpu->reg[cpu->decode.rd].valid = 1;

                if(cpu->is_waiting_mulFU == 1){
                    cpu->is_waiting_fu = 1;
                }
                if(cpu->reg[cpu->decode.rs1].valid == 0 && cpu->reg[cpu->decode.rs2].valid == 0){
                    validInput = 1;
                }
                hasDest = 1;
                /*cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];*/
                break;
            }
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            {
            
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    cpu->decode.rs1_value = cpu->reg[cpu->decode.rs1].regs;
                }

                if(cpu->reg[cpu->decode.rs2].valid == 0){
                    cpu->decode.rs2_value = cpu->reg[cpu->decode.rs2].regs;
                }

                //cpu->reg[cpu->decode.rd].valid = 1;
                /*cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];*/
                if(cpu->is_waiting_intFU == 1){
                    cpu->is_waiting_fu = 1;
                }
                if(cpu->reg[cpu->decode.rs1].valid == 0 && cpu->reg[cpu->decode.rs2].valid == 0){
                    validInput = 1;
                }
                hasDest = 1;
                break;
            
            }

            case OPCODE_CMP:
            {
            
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    cpu->decode.rs1_value = cpu->reg[cpu->decode.rs1].regs;
                }

                if(cpu->reg[cpu->decode.rs2].valid == 0){
                    cpu->decode.rs2_value = cpu->reg[cpu->decode.rs2].regs;
                }

                /*cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];*/
                if(cpu->is_waiting_intFU == 1){
                    cpu->is_waiting_fu = 1;
                }
                if(cpu->reg[cpu->decode.rs1].valid == 0 && cpu->reg[cpu->decode.rs2].valid == 0){
                    validInput = 1;
                }
                break;
            
            }

            case OPCODE_STORE:
            {
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    cpu->decode.rs1_value = cpu->reg[cpu->decode.rs1].regs;
                }

                if(cpu->reg[cpu->decode.rs2].valid == 0){
                    cpu->decode.rs2_value = cpu->reg[cpu->decode.rs2].regs;
                }

                if(cpu->reg[cpu->decode.rs1].valid == 0 && cpu->reg[cpu->decode.rs2].valid == 0){
                    validInput = 1;
                }
                /*cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];*/
                if(cpu->is_waiting_loadFU == 1){
                    cpu->is_waiting_fu = 1;
                }
                break;
            }

            case OPCODE_LDR:
            {
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    cpu->decode.rs1_value = cpu->reg[cpu->decode.rs1].regs;
                }

                if(cpu->reg[cpu->decode.rs2].valid == 0){
                    cpu->decode.rs2_value = cpu->reg[cpu->decode.rs2].regs;
                }

                if(cpu->reg[cpu->decode.rs1].valid == 0 && cpu->reg[cpu->decode.rs2].valid == 0){
                    validInput = 1;
                }

                //cpu->reg[cpu->decode.rd].valid = 1;
                /*cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];*/
                if(cpu->is_waiting_loadFU == 1){
                    cpu->is_waiting_fu = 1;
                }
                hasDest = 1;
                break;
            }

            case OPCODE_STR:
            {
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    cpu->decode.rs1_value = cpu->reg[cpu->decode.rs1].regs;
                }

                if(cpu->reg[cpu->decode.rs2].valid == 0){
                    cpu->decode.rs2_value = cpu->reg[cpu->decode.rs2].regs;
                }

                if(cpu->reg[cpu->decode.rs3].valid == 0){
                    cpu->decode.rs3_value = cpu->reg[cpu->decode.rs3].regs;
                }

                if(cpu->reg[cpu->decode.rs1].valid == 0 && cpu->reg[cpu->decode.rs2].valid == 0 && cpu->reg[cpu->decode.rs3].valid == 0){
                    validInput = 1;
                }
                /*cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];*/
                if(cpu->is_waiting_loadFU == 1){
                    cpu->is_waiting_fu = 1;
                }
                break;
            }

            case OPCODE_LOAD:
            {
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    cpu->decode.rs1_value = cpu->reg[cpu->decode.rs1].regs;
                }
                //cpu->reg[cpu->decode.rd].valid = 1;
                if(cpu->is_waiting_loadFU == 1){
                    cpu->is_waiting_fu = 1;
                }
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    validInput = 1;
                }
                hasDest = 1;
                break;
            }

            case OPCODE_MOVC:
            case OPCODE_NOP:
            {
                /* MOVC doesn't have register operands */
                //cpu->reg[cpu->decode.rd].valid = 1; //setting destination register as not valid
                if(cpu->is_waiting_intFU == 1){
                    cpu->is_waiting_fu = 1;
                }
                validInput = 1;
                hasDest = 1;
                break;
            }

            case OPCODE_ADDL:
            case OPCODE_SUBL:
            {
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    cpu->decode.rs1_value = cpu->reg[cpu->decode.rs1].regs;
                }
                //cpu->reg[cpu->decode.rd].valid = 1;
                 if(cpu->is_waiting_intFU == 1){
                    cpu->is_waiting_fu = 1;
                }
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    validInput = 1;
                }
                hasDest = 1;
                break;
            }

            case OPCODE_BZ:
            case OPCODE_BNZ:
            {
                if(cpu->is_waiting_intFU == 1 || cpu->zero_flag_valid == 1){
                    cpu->is_waiting_fu = 1;
                }
                validInput = 1;
                break;
            }

            case OPCODE_HALT:
            {
                if(cpu->is_waiting_intFU == 1){
                    cpu->is_waiting_fu = 1;
                }
                validInput = 1;
                break;
            }
        }

        if (printMsg == 1)
        {
            print_stage_content("Decode/RF", &cpu->decode);
        }

        /*if(cpu->reg[cpu->decode.rs1].valid == 0 && cpu->reg[cpu->decode.rs2].valid == 0 && cpu->reg[cpu->decode.rs3].valid == 0 && cpu->is_waiting_fu == 0){
             //Copy data from decode latch to execute latch
            cpu->reg[cpu->decode.rd].valid = 1;
            cpu->execute = cpu->decode;
            cpu->decode.has_insn = FALSE;
            cpu->is_waiting_decode = 0;
            
        }*/
        if(validInput == 1){

             if(hasDest == 1){
                    cpu->reg[cpu->decode.rd].valid = 1;
                }      
            cpu->execute = cpu->decode;
            cpu->decode.has_insn = FALSE;
            cpu->is_waiting_decode = 0;
        }
        
    }else{
        if (printMsg == 1)
        {
            print_empty_content("Decode/RF", &cpu->decode);
        }
    }
}

static void
APEX_IntegerFU(APEX_CPU *cpu, int printMsg)
{

    if (cpu->integerFU.has_insn)
    {
        if(integerFUCounter == 1){

            cpu->is_waiting_intFU = 1;
            /* Execute logic based on instruction type */
            switch (cpu->integerFU.opcode)
            {
            case OPCODE_ADD:
            {
                cpu->integerFU.result_buffer = cpu->integerFU.rs1_value + cpu->integerFU.rs2_value;

                /* Set the zero flag based on the result buffer */
                /*if (cpu->integerFU.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }*/
                cpu->zero_flag_valid = 1;
                enqueue(1);
                break;
            }

            case OPCODE_BZ:
            {
                if (cpu->zero_flag == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->integerFU.pc + cpu->integerFU.imm;

                    /* Since we are using reverse callbacks for pipeline stages,
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                enqueue(1);
                break;
            }

            case OPCODE_BNZ:
            {
                if (cpu->zero_flag == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->integerFU.pc + cpu->integerFU.imm;

                    /* Since we are using reverse callbacks for pipeline stages,
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                enqueue(1);
                break;
            }

            case OPCODE_MOVC:
            {
                cpu->integerFU.result_buffer = cpu->integerFU.imm;

                /* Set the zero flag based on the result buffer */

                // commenting below code since zero flag not applicable for MOVC
                /*if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }*/
                enqueue(1);
                break;
            }

            case OPCODE_ADDL:
            {
                cpu->integerFU.result_buffer = cpu->integerFU.rs1_value + cpu->integerFU.imm;

                /* Set the zero flag based on the result buffer */
                /*if (cpu->integerFU.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }*/
                cpu->zero_flag_valid = 1;
                enqueue(1);
                break;
            }

            case OPCODE_SUB:
            {
                cpu->integerFU.result_buffer = cpu->integerFU.rs1_value - cpu->integerFU.rs2_value;

                /* Set the zero flag based on the result buffer */
                /*if (cpu->integerFU.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }*/
                cpu->zero_flag_valid = 1;
                enqueue(1);
                break;
            }

            case OPCODE_SUBL:
            {
                cpu->integerFU.result_buffer = cpu->integerFU.rs1_value - cpu->integerFU.imm;

                /* Set the zero flag based on the result buffer */
                /*if (cpu->integerFU.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }*/
                cpu->zero_flag_valid = 1;
                enqueue(1);
                break;
            }

            case OPCODE_AND:
            {
                cpu->integerFU.result_buffer = cpu->integerFU.rs1_value & cpu->integerFU.rs2_value;
                enqueue(1);
                break;
            }

            case OPCODE_OR:
            {
                cpu->integerFU.result_buffer = cpu->integerFU.rs1_value | cpu->integerFU.rs2_value;
                enqueue(1);
                break;
            }

            case OPCODE_XOR:
            {
                cpu->integerFU.result_buffer = cpu->integerFU.rs1_value ^ cpu->integerFU.rs2_value;
                enqueue(1);
                break;
            }

            case OPCODE_NOP:
            {
                enqueue(1);
                break;
            }

            case OPCODE_CMP:
            {
                /*if (cpu->integerFU.rs1_value == cpu->integerFU.rs2_value)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }*/
                cpu->zero_flag_valid = 1;
                enqueue(1);
                break;
            }

            case OPCODE_HALT:
            {
                enqueue(1);
                break;
            }
            }
        }

        if(integerFUCounter >= 1){
            if(priorityQueue[front] == 1){
                /* Copy data from execute latch to memory latch*/
                cpu->writeback = cpu->integerFU;
                cpu->integerFU.has_insn = FALSE;
                integerFUCounter = 1;
                cpu->is_waiting_intFU = 0;
                cpu->zero_flag_valid = 0;
                dequeue();
            }
        }else{
            integerFUCounter++;
        }

        if (printMsg == 1)
        {
            print_stage_content("Integer FU", &cpu->integerFU);
        }

    }else{
        if (printMsg == 1)
        {
           print_empty_content("Integer FU", &cpu->integerFU);
        }
    }
    
}

static void
APEX_MulFU(APEX_CPU *cpu, int printMsg)
{
    if (cpu->multiplierFU.has_insn)
    {
        /* Execute logic based on instruction type */
        if(mulFUCounter == 1){
             cpu->is_waiting_mulFU = 1;
            switch (cpu->multiplierFU.opcode)
            {   
                case OPCODE_MUL:
                {
                    cpu->multiplierFU.result_buffer = cpu->multiplierFU.rs1_value * cpu->multiplierFU.rs2_value;

                    /* Set the zero flag based on the result buffer */
                    /*
                    if (cpu->multiplierFU.result_buffer == 0)
                    {
                       cpu->zero_flag = TRUE;
                    } 
                    else 
                    {
                       cpu->zero_flag = FALSE;
                    }*/
                    cpu->zero_flag_valid = 1;
                    enqueue(2);
                    break;
                }

            }

        }

        if(mulFUCounter >= 3){
            if(priorityQueue[front] == 2){
                /* Copy data from multiplier FU latch to memory latch*/
                cpu->writeback = cpu->multiplierFU;
                cpu->multiplierFU.has_insn = FALSE;
                mulFUCounter = 1;
                cpu->is_waiting_mulFU = 0;
                cpu->zero_flag_valid = 0;
                dequeue();
            }
        }else{
            mulFUCounter++;
        }

        if (printMsg == 1)
        {
            print_stage_content("Multiplier FU", &cpu->multiplierFU);
        }
    }else{
        if (printMsg == 1)
        {
            print_empty_content("Multiplier FU", &cpu->multiplierFU);
        }
    }

}

static void
APEX_loadStoreFU(APEX_CPU *cpu, int printMsg)
{
    if (cpu->loadStoreFU.has_insn)
    {
        /* Execute logic based on instruction type */
        if(loadStoreFUCounter == 1){
            cpu->is_waiting_loadFU = 1;
            switch (cpu->loadStoreFU.opcode)
            {
            case OPCODE_LOAD:
            {
                cpu->loadStoreFU.memory_address = cpu->loadStoreFU.rs1_value + cpu->loadStoreFU.imm;

                /* Read from data memory */
                cpu->loadStoreFU.result_buffer = cpu->data_memory[cpu->loadStoreFU.memory_address];
                enqueue(3);
                break;
            }

            case OPCODE_STORE:
            {
                cpu->loadStoreFU.memory_address = cpu->loadStoreFU.rs2_value + cpu->loadStoreFU.imm;

                /* Write to data memory */
                cpu->data_memory[cpu->loadStoreFU.memory_address] = cpu->loadStoreFU.rs1_value;
                enqueue(3);
                break;
            }

            case OPCODE_LDR:
            {
                cpu->loadStoreFU.memory_address = cpu->loadStoreFU.rs1_value + cpu->loadStoreFU.rs2_value;

                /* Read from data memory */
                cpu->loadStoreFU.result_buffer = cpu->data_memory[cpu->loadStoreFU.memory_address];
                enqueue(3);
                break;
            }

            case OPCODE_STR:
            {
                cpu->loadStoreFU.memory_address = cpu->loadStoreFU.rs1_value + cpu->loadStoreFU.rs2_value;

                /* Write to data memory */
                cpu->data_memory[cpu->loadStoreFU.memory_address] = cpu->loadStoreFU.rs3_value;
                enqueue(3);
                break;
            }
            }
        }

        if(loadStoreFUCounter >= 4){
            if(priorityQueue[front] == 3){
                /* Copy data from execute latch to memory latch*/
                cpu->writeback = cpu->loadStoreFU;
                cpu->loadStoreFU.has_insn = FALSE;
                loadStoreFUCounter = 1;
                cpu->is_waiting_loadFU = 0;
                dequeue();
            }  
        }else{
            loadStoreFUCounter++;
        }

        if (printMsg == 1)
        {
            print_stage_content("Load/Store FU", &cpu->loadStoreFU);
        }
    }else{
        if (printMsg == 1)
        {
            print_empty_content("Load/Store FU", &cpu->loadStoreFU);
        }
    }
    
}

static int
APEX_execute(APEX_CPU *cpu, int printMsg)
{
    if(cpu->execute.has_insn){

        switch(cpu->execute.opcode){

            case OPCODE_MUL:
            {
                cpu->multiplierFU = cpu->execute;
                cpu->execute.has_insn = FALSE;
                break;
            }

            case OPCODE_LOAD:
            case OPCODE_STORE:
            case OPCODE_LDR:
            case OPCODE_STR:
            {
                cpu->loadStoreFU = cpu->execute;
                cpu->execute.has_insn = FALSE;
                break;
            }

            case OPCODE_ADD:
            case OPCODE_BZ:
            case OPCODE_BNZ:
            case OPCODE_MOVC:
            case OPCODE_ADDL:
            case OPCODE_SUB:
            case OPCODE_SUBL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_NOP:
            case OPCODE_CMP:
            case OPCODE_HALT:
            {
                cpu->integerFU = cpu->execute;
                cpu->execute.has_insn = FALSE;
                break;
            }

        }
    }
    APEX_IntegerFU(cpu, printMsg);
    APEX_MulFU(cpu, printMsg);
    APEX_loadStoreFU(cpu, printMsg);
    return 0;
}
/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu, int printMsg)
{
    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->reg[cpu->writeback.rd].regs = cpu->writeback.result_buffer;
                cpu->reg[cpu->writeback.rd].valid = 0;
                //cpu->reg[cpu->writeback.rs3].valid = 0;
                /* Set the zero flag based on the result buffer */
                
                if (cpu->writeback.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->reg[cpu->writeback.rd].regs = cpu->writeback.result_buffer;
                cpu->reg[cpu->writeback.rd].valid = 0;
                //cpu->reg[cpu->writeback.rs3].valid = 0;
                break;
            }

            case OPCODE_LDR:
            {
                cpu->reg[cpu->writeback.rd].regs = cpu->writeback.result_buffer;
                cpu->reg[cpu->writeback.rd].valid = 0;
                //cpu->reg[cpu->writeback.rs3].valid = 0;
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->reg[cpu->writeback.rd].regs = cpu->writeback.result_buffer;
                cpu->reg[cpu->writeback.rd].valid = 0;
                //cpu->reg[cpu->writeback.rs3].valid = 0;
                break;
            }

            case OPCODE_ADDL:
            case OPCODE_SUB: 
            case OPCODE_SUBL:  
            case OPCODE_MUL:
            {
                cpu->reg[cpu->writeback.rd].regs = cpu->writeback.result_buffer;
                cpu->reg[cpu->writeback.rd].valid = 0;
                //cpu->reg[cpu->writeback.rs3].valid = 0;

                /* Set the zero flag based on the result buffer */
                
                if (cpu->writeback.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_AND:
            case OPCODE_OR: 
            case OPCODE_XOR:
            {
                cpu->reg[cpu->writeback.rd].regs = cpu->writeback.result_buffer;
                cpu->reg[cpu->writeback.rd].valid = 0;
                //cpu->reg[cpu->writeback.rs3].valid = 0;
                break;
            }

            case OPCODE_NOP:
            case OPCODE_STORE:
            {
                //cpu->reg[cpu->writeback.rd].valid = 0;
                //cpu->reg[cpu->writeback.rs3].valid = 0;
                break;
            }

            case OPCODE_STR:
            {
                //cpu->reg[cpu->writeback.rd].valid = 0;
                //cpu->reg[cpu->writeback.rs3].valid = 0;
                break;
            }

            case OPCODE_CMP:
            {
                 if (cpu->writeback.rs1_value == cpu->writeback.rs2_value)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }
                cpu->zero_flag_valid = 0;
            }
        }

        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (printMsg == 1)
        {
           print_stage_content("Writeback", &cpu->writeback);
        }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }

    }else{
        if (printMsg == 1)
        {
           print_empty_content("Writeback", &cpu->writeback);
        }
    }
    

    /* Default */
    return 0;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename, int printMsg)
{
    int i;
    APEX_CPU *cpu;
    mulFUCounter = 1;
    loadStoreFUCounter = 1;
    integerFUCounter = 1;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    cpu->clock = 1;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    cpu->zero_flag_valid = 0;
    if (printMsg == 1)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s\t %-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2", "rs3",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s\t %-9d %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].rs3, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_run(APEX_CPU *cpu, int totalCycles)
{
    char user_prompt_val;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu, 1))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        //APEX_memory(cpu);
        APEX_execute(cpu, 1);
        APEX_decode(cpu, 1);
        APEX_fetch(cpu, 1); 

        print_reg_file(cpu);

        /*printf("--------------------------------------------\n");
        printf("Z Flag : %d\n", cpu->zero_flag);
        printf("--------------------------------------------\n");*/

        //printing data in memory location 20
        /*printf("--------------------------------------------\n");
        printf("Memory : %d\n", cpu->data_memory[20]);
        printf("--------------------------------------------\n");*/

        cpu->clock++;

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }else{
            if(cpu->clock == totalCycles){
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                exit(1);
            }
        }

        //cpu->clock++;
    }
}

void
APEX_cpu_simulate(APEX_CPU *cpu, int totalCycles)
{
    char user_prompt_val;

    while (TRUE)
    {
        /*if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }*/

        if (APEX_writeback(cpu, 0))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        APEX_execute(cpu, 0);
        APEX_decode(cpu, 0);
        APEX_fetch(cpu, 0);

        //print_reg_file(cpu);

        cpu->clock++;

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }else{
            if(cpu->clock == totalCycles){
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                return;
            }
        }
    }
}

void
APEX_cpu_display(APEX_CPU *cpu, int totalCycles)
{
    char user_prompt_val;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu, 1))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        APEX_execute(cpu, 1);
        APEX_decode(cpu, 1);
        APEX_fetch(cpu, 1);

        printf("--------------------------------------------\n");
        printf("Z Flag : %d\n", cpu->zero_flag);
        printf("--------------------------------------------\n");

        //print_reg_file(cpu);

        cpu->clock++;

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }else{
            if(cpu->clock == totalCycles){
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                return;
            }
        }
    }
}

void
APEX_cpu_single_step(APEX_CPU *cpu, int totalCycles)
{
    char user_prompt_val;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu, 1))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        APEX_execute(cpu, 1);
        APEX_decode(cpu, 1);
        APEX_fetch(cpu, 1);

        printf("--------------------------------------------\n");
        printf("Z Flag : %d\n", cpu->zero_flag);
        printf("--------------------------------------------\n");

        //print_reg_file(cpu);

        cpu->clock++;

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }else{
            if(cpu->clock == totalCycles){
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                return;
            }
        }
    }
}

void
APEX_cpu_show_mem(APEX_CPU *cpu, int totalCycles)
{
    char user_prompt_val;

    while (TRUE)
    {
        /*if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }*/

        if (APEX_writeback(cpu, 0))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        APEX_execute(cpu, 0);
        APEX_decode(cpu, 0);
        APEX_fetch(cpu, 0);

        /*printf("--------------------------------------------\n");
        printf("Z Flag : %d\n", cpu->zero_flag);
        printf("--------------------------------------------\n");*/

        //print_reg_file(cpu);

        cpu->clock++;

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }else{
            if(cpu->clock == totalCycles){
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                return;
            }
        }
    }
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}