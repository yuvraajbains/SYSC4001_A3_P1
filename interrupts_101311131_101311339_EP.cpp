/**
 * @file interrupts_101311131_101311339_EP.cpp
 * @author Sasisekhar Govind, Yuvraj Bains, James Bian
 */

#include "interrupts_101311131_101311339.hpp"
#include <sstream>
#include <algorithm>

void sortByPriority(std::vector<PCB> &ready_queue) {
    std::sort(ready_queue.begin(), ready_queue.end(), 
        [](const PCB &a, const PCB &b) {
            return a.PID > b.PID; 
        });
}

std::tuple<std::string> run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   
    std::vector<PCB> wait_queue;    
    std::vector<PCB> job_list;      

    unsigned int current_time = 0;
    PCB running;

    // Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;
    execution_status = print_exec_header();
    std::string memory_status = "--- Memory Usage Log ---\n";

    while(!all_process_terminated(job_list) || !list_processes.empty()) {

        // ----------------- MANAGE WAIT QUEUE (I/O) -----------------
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
                    // Memory full, retry next tick
                    proc_it->arrival_time++; 
                    ++proc_it;
                }
            } else {
                ++proc_it;
            }
        }

        // ----------------- SCHEDULER DISPATCH -----------------
        if (running.state == NOT_ASSIGNED || running.state == TERMINATED || running.state == WAITING) {
            if (!ready_queue.empty()) {
                sortByPriority(ready_queue);
                run_process(running, job_list, ready_queue, current_time);
                execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
            }
        }
        if (running.state == RUNNING) {
            
            running.remaining_time--;
            
            // Check for Termination 
            if (running.remaining_time == 0) {
                terminate_process(running, job_list);
                
                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, TERMINATED);
                
                free_memory(running);
                idle_CPU(running);
            } 
            else {
                
                //If we have done work equal to Frequency, trigger I/O
                int time_spent = running.processing_time - running.remaining_time;
                
                if (running.io_freq > 0 && (time_spent % running.io_freq == 0)) {
                    
                    running.state = WAITING;
                    // Wait starts at the END of this tick
                    running.start_time = current_time + 1; 
                    
                    wait_queue.push_back(running);
                    sync_queue(job_list, running);
                    
                    execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, WAITING);
                    
                    idle_CPU(running);
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
        std::cout << "Usage: ./bin/interrupts_EP.exe <input_file>" << std::endl;
        return -1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << argv[1] << std::endl;
        return -1;
    }

    std::string line;
    std::vector<PCB> list_process;

    while(std::getline(input_file, line)) {
        if (line.empty()) continue; 
        //specific formating was needed
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