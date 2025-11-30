/**
 * @file interrupts_101311131_101311339_RR.cpp
 * @author Sasisekhar Govind, Yuvraj Bains, James Bian
 */

#include "interrupts_101311131_101311339.hpp"
#include <sstream>

std::tuple<std::string> run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   
    std::vector<PCB> wait_queue;    
    std::vector<PCB> job_list;      

    unsigned int current_time = 0;
    unsigned int time_slice_counter = 0; 
    const unsigned int TIME_QUANTUM = 100;

    PCB running;
    idle_CPU(running);

    std::string execution_status = print_exec_header();
    std::string memory_status = "--- Memory Usage Log ---\n";

    // Loop until all processes are terminated
    while(!all_process_terminated(job_list) || !list_processes.empty()) {

        auto it = wait_queue.begin();
        while (it != wait_queue.end()) {
            if (current_time - it->start_time >= it->io_duration) {
                PCB p = *it;
                p.state = READY;
                p.start_time = -1; 
                ready_queue.push_back(p);
                sync_queue(job_list, p);
                execution_status += print_exec_status(current_time, p.PID, WAITING, READY);
                it = wait_queue.erase(it);
            } else {
                ++it;
            }
        }

        auto proc_it = list_processes.begin();
        while (proc_it != list_processes.end()) {
            if (proc_it->arrival_time == current_time) {
                if (assign_memory(*proc_it)) {
                    PCB p = *proc_it;
                    p.state = READY;
                    ready_queue.push_back(p);
                    job_list.push_back(p);
                    execution_status += print_exec_status(current_time, p.PID, NEW, READY);
                    memory_status += get_memory_status(current_time);
                    proc_it = list_processes.erase(proc_it);
                } else {
                    proc_it->arrival_time++; 
                    ++proc_it;
                }
            } else {
                ++proc_it;
            }
        }
        
        // Check for Time Quantum Expiry
        if (running.state == RUNNING) {
            if (time_slice_counter >= TIME_QUANTUM) {
                running.state = READY;
                ready_queue.push_back(running); // Move to back of FIFO
                sync_queue(job_list, running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                idle_CPU(running);
                time_slice_counter = 0;
            }
        }

        // Dispatch if CPU Idle
        if ((running.state == NOT_ASSIGNED || running.state == TERMINATED || running.state == WAITING || running.state == READY) && !ready_queue.empty()) {
            // Pick from Front (FIFO)
            PCB next_proc = ready_queue.front();
            ready_queue.erase(ready_queue.begin());
            
            running = next_proc;
            running.start_time = current_time;
            running.state = RUNNING;
            sync_queue(job_list, running);
            time_slice_counter = 0;
            
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
        }

        if (running.state == RUNNING) {
            running.remaining_time--;
            time_slice_counter++;

            if (running.remaining_time == 0) {
                terminate_process(running, job_list);
                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, TERMINATED);
                free_memory(running);
                idle_CPU(running);
                time_slice_counter = 0;
            } 
            else {
                //Check I/O AFTER work is done
                int time_spent = running.processing_time - running.remaining_time;
                if (running.io_freq > 0 && (time_spent % running.io_freq == 0)) {
                    running.state = WAITING;
                    running.start_time = current_time + 1; 
                    
                    wait_queue.push_back(running);
                    sync_queue(job_list, running);
                    
                    execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, WAITING);
                    idle_CPU(running);
                    time_slice_counter = 0;
                }
            }
        }
        current_time++;
    }
    
    execution_status += print_exec_footer();
    write_output(memory_status, "output_files/memory_status.txt");
    return std::make_tuple(execution_status);
}

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "Usage: ./bin/interrupts_RR.exe <input_file>" << std::endl;
        return -1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << argv[1] << std::endl;
        return -1;
    }

    std::string line;
    std::vector<PCB> list_process;

    // ROBUST PARSING LOGIC
    while(std::getline(input_file, line)) {
        if (line.empty()) continue;
        
        for (char &c : line) if (c == ',') c = ' '; 
        
        std::stringstream ss(line);
        std::vector<std::string> tokens;
        std::string temp;
        while (ss >> temp) tokens.push_back(temp);

        if (tokens.size() >= 6) {
            auto new_process = add_process(tokens);
            list_process.push_back(new_process);
        }
    }
    input_file.close();

    auto [exec] = run_simulation(list_process);
    
    write_output(exec, "output_files/execution.txt"); 

    return 0;
}