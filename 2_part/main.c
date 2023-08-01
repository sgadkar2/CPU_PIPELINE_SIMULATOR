/*
 * main.c
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"

int
main(int argc, char *argv[])
{
    APEX_CPU *cpu;

    fprintf(stderr, "APEX CPU Pipeline Simulator v%0.1lf\n", VERSION);

   /*if (argc != 2)
    {
        fprintf(stderr, "APEX_Help: Usage %s <input_file>\n", argv[0]);
        exit(1);
    }*/

    if(strcasecmp(argv[2], "Initialize") == 0){
        
        cpu = APEX_cpu_init(argv[1], 1);
        if (!cpu)
        {
           fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
           exit(1);
        }else{
           printf("APEX Simulator initialized successfully\n");
           exit(1);
        }
    }else if(strcasecmp(argv[2],"Simulate") == 0){
        
        cpu = APEX_cpu_init(argv[1], 0);
        cpu->single_step = 0;
        APEX_cpu_simulate(cpu, atoi(argv[3]));
        print_reg_file(cpu);
        printf("================STATE OF DATA MEMORY==================\n");

        for(int i = 0; i < DATA_MEMORY_SIZE; i=i+4){

            printf("MEM[%d] : %d\n", i, cpu->data_memory[i]);            

            /*if(cpu->data_memory[i] > 0){
              
               printf("MEM[%d] : %d\n", i, cpu->data_memory[i]);            
            }*/
        }
        APEX_cpu_stop(cpu);
    }else if(strcasecmp(argv[2],"Single_Step") == 0){
        
        cpu = APEX_cpu_init(argv[1], 0);
        APEX_cpu_single_step(cpu, 0);
        print_reg_file(cpu);
        printf("==========STATE OF DATA MEMORY==============\n");

        //int memCounter = 1;

        for(int i = 0; i < DATA_MEMORY_SIZE; i=i+4){

            printf("MEM[%d] : %d\n", i, cpu->data_memory[i]);            

            /*if(cpu->data_memory[i] > 0){
              
               printf("MEM[%d] : %d\n", i, cpu->data_memory[i]);            
            }*/
        }
        printf("--------------------------------------------\n");
        APEX_cpu_stop(cpu);
    }else if(strcasecmp(argv[2],"ShowMem") == 0){
        
        cpu = APEX_cpu_init(argv[1], 0);
        cpu->single_step = 0;
        APEX_cpu_show_mem(cpu, 0);
        printf("==========STATE OF DATA MEMORY==============\n");
        printf("MEM[%d] : %d\n", atoi(argv[3]), cpu->data_memory[atoi(argv[3])]);
        //printf("--------------------------------------------\n");
        APEX_cpu_stop(cpu);
    }else if(strcasecmp(argv[2],"Display") == 0){
        
        cpu = APEX_cpu_init(argv[1], 0);
        cpu->single_step = 0;
        APEX_cpu_display(cpu, atoi(argv[3]));
        /*printf("--------------------------------------------\n");
        printf("Z Flag : %d\n", cpu->zero_flag);
        printf("--------------------------------------------\n");*/

        //printf("Memory data: \n\n");
        print_reg_file(cpu);
        printf("==========STATE OF DATA MEMORY==============\n");

        //int memCounter = 1;

        for(int i = 0; i < DATA_MEMORY_SIZE; i=i+4){

            printf("MEM[%d] : %d\n", i, cpu->data_memory[i]);            

            /*if(cpu->data_memory[i] > 0){
              
               printf("MEM[%d] : %d\n", i, cpu->data_memory[i]);            
            }*/
        }
        printf("--------------------------------------------\n");
        APEX_cpu_stop(cpu);
    }else{
        fprintf(stderr, "APEX_Help: Usage %s <input_file>\n", argv[0]);
        exit(1);
    }

    /*cpu = APEX_cpu_init(argv[1]);
    if (!cpu)
    {
        fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
        exit(1);
    }*/

    /*APEX_cpu_run(cpu);
    APEX_cpu_stop(cpu);*/
    return 0;
}