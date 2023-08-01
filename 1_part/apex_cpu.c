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
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
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
            //cpu->is_waiting_fetch = 0;
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
        cpu->is_waiting_decode = 1;
        int hasDest = 0;
        int validInput = 0;
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
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
                 hasDest = 1;
                break;
            }

            case OPCODE_CMP:
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

                //cpu->reg[cpu->decode.rd].valid = 1;
                /*cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];*/
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
                break;
            }

            case OPCODE_LOAD:
            {
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    cpu->decode.rs1_value = cpu->reg[cpu->decode.rs1].regs;
                    validInput = 1;
                }

                
                //cpu->reg[cpu->decode.rd].valid = 1;
                hasDest = 1;
                break;
            }

            case OPCODE_MOVC:
            {
                /* MOVC doesn't have register operands */
                //cpu->reg[cpu->decode.rd].valid = 1; //setting destination register as not valid
                validInput = 1;
                hasDest = 1;
                break;
            }

            case OPCODE_NOP:
            case OPCODE_BNZ:
            case OPCODE_BZ:
            case OPCODE_HALT:
            {
                /* MOVC doesn't have register operands */
                //cpu->reg[cpu->decode.rd].valid = 1; //setting destination register as not valid
                validInput = 1;
                break;
            }

            case OPCODE_ADDL:
            case OPCODE_SUBL:
            {
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    cpu->decode.rs1_value = cpu->reg[cpu->decode.rs1].regs;
                }
                if(cpu->reg[cpu->decode.rs1].valid == 0){
                    validInput = 1;
                }
                hasDest = 1;
                //cpu->reg[cpu->decode.rd].valid = 1;
                break;
            }
        }

        /*if(cpu->reg[cpu->decode.rs1].valid == 0 && cpu->reg[cpu->decode.rs2].valid == 0 && cpu->reg[cpu->decode.rs3].valid == 0){
            Copy data from decode latch to execute latch
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
        
        if (printMsg == 1)
        {
            print_stage_content("Decode/RF", &cpu->decode);
        }
    }else{
        if (printMsg == 1)
        {
            print_empty_content("Decode/RF", &cpu->decode);
        }
    }
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu, int printMsg)
{
    if (cpu->execute.has_insn)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value + cpu->execute.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
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
                cpu->execute.memory_address
                    = cpu->execute.rs1_value + cpu->execute.imm;
                break;
            }

            case OPCODE_STORE:
            {
                cpu->execute.memory_address
                    = cpu->execute.rs2_value + cpu->execute.imm;
                break;
            }

            case OPCODE_BZ:
            {
                if (cpu->zero_flag == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BNZ:
            {
                if (cpu->zero_flag == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->execute.result_buffer = cpu->execute.imm;

                /* Set the zero flag based on the result buffer */

                //commenting below code since zero flag not applicable for MOVC
                /*if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }*/
                break;
            }

            case OPCODE_ADDL:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value + cpu->execute.imm;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_SUB:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value - cpu->execute.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_SUBL:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value - cpu->execute.imm;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_MUL:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value * cpu->execute.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
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
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value & cpu->execute.rs2_value;

                break;
            }

            case OPCODE_OR:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value | cpu->execute.rs2_value;

                break;
            }

            case OPCODE_XOR:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value ^ cpu->execute.rs2_value;

                break;
            }

            case OPCODE_NOP:
            {
                break;
            }

            case OPCODE_CMP:
            {
                if (cpu->execute.rs1_value == cpu->execute.rs2_value)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_LDR:
            {
                cpu->execute.memory_address
                    = cpu->execute.rs1_value + cpu->execute.rs2_value;
                break;
            }

            case OPCODE_STR:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.rs2_value;
                break;
            }
        }

        /* Copy data from execute latch to memory latch*/
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (printMsg == 1)
        {
            print_stage_content("Execute", &cpu->execute);
        }
    }else{
        if (printMsg == 1)
        {
            print_empty_content("Execute", &cpu->execute);
        }
    }
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu, int printMsg)
{
    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_ADDL:
            case OPCODE_SUB:
            case OPCODE_SUBL:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_NOP:
            case OPCODE_CMP:
            {
                /* No work for ADD */
                break;
            }

            case OPCODE_LOAD:
            {
                /* Read from data memory */
                cpu->memory.result_buffer
                    = cpu->data_memory[cpu->memory.memory_address];
                break;
            }

            case OPCODE_STORE:
            {
                /* Write to data memory */
                cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs1_value;
                break;
            }

            case OPCODE_LDR:
            {
                /* Read from data memory */
                cpu->memory.result_buffer
                    = cpu->data_memory[cpu->memory.memory_address];
                break;
            }

            case OPCODE_STR:
            {
                /* Write to data memory */
                cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs3_value;
                break;
            }

        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

        if (printMsg == 1)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }else{
         if (printMsg == 1)
        {
            print_empty_content("Memory", &cpu->memory);
        }
    }
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
            case OPCODE_CMP:
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

        APEX_memory(cpu, 1);
        APEX_execute(cpu, 1);
        APEX_decode(cpu, 1);
        APEX_fetch(cpu, 1);

        print_reg_file(cpu);

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

        APEX_memory(cpu, 0);
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

        APEX_memory(cpu, 1);
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

        APEX_memory(cpu, 1);
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

        APEX_memory(cpu, 0);
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