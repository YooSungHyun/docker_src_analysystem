#ifndef __TRAINHMM__
#define __TRAINHMM__

#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>
#include <time.h>
#include <map>
#include <list>
#include "utils.h"
#include "InputUtil.h"
#include "HMMProblem.h"
#include "StripedArray.h"

struct param param;


static int max_line_length;
static char * line;
static char* readline(FILE *fid);
std::string get_params(HMMProblem *hmm);
std::string run(std::string data);
std::string run(std::string data, std::string PI_0, std::string not_forget, std::string learn, std::string not_slip, std::string guess, std::string metrics);
void set_initial_params(std::string PI_0, std::string not_forget, std::string learn, std::string not_slip, std::string guess, std::string metrics);
std::string get_results(HMMProblem *hmm);

void exit_with_help();
void parse_arguments_step1(int argc, char **argv, char *input_file_name, char *output_file_name, char *predict_file_name, char *console_file_name);
void parse_arguments_step2(int argc, char **argv, FILE *fid_console);
bool read_and_structure_data(std::string data, FILE *fid_console);
NUMBER cross_validate(NUMBER* metrics, const char *filename, const char *model_file_name, clock_t *tm_fit, clock_t *tm_predict, FILE *fid_console);
NUMBER cross_validate_item(NUMBER* metrics, const char *filename, const char *model_file_name, clock_t *tm_fit, clock_t *tm_predict, FILE *fid_console);
NUMBER cross_validate_nstrat(NUMBER* metrics, const char *filename, const char *model_file_name, clock_t *tm_fit, clock_t *tm_predict, FILE *fid_console);

#endif