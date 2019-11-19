#include <string.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <map>
#include <list>
#include "utils.h"
#include "HMMProblem.h"
#include "InputUtil.h"

using namespace std;

#define COLUMNS 4

struct param param;
static char *line = NULL;
NUMBER* metrics;
map<string,NCAT> data_map_group_fwd;
map<NCAT,string> data_map_group_bwd;
map<string,NCAT> map_step;
map<string,NCAT> model_map_skill_fwd;
map<NCAT,string> model_map_skill_bwd;
void exit_with_help();
void parse_arguments(int argc, char **argv, char *input_file_name, char *model_file_name, char *predict_file_name);
void read_predict_data(const char *filename);
void predict(const char *predict_file, HMMProblem *hmm);

int main (int argc, char ** argv) {
    clock_t tm0 = clock();
    printf("predicthmm starting...\n");
    set_param_defaults(&param);
    
    char input_file[1024];
    char model_file[1024];
    char predict_file[1024];
    
    parse_arguments(argc, argv, input_file, model_file, predict_file);

    // read data
    if(param.binaryinput==0) {
        InputUtil::readTxt(input_file, &param);
    } else {
        InputUtil::readBin(input_file, &param);
    }
    
    // read model header
    FILE *fid = fopen(model_file,"r");
    if(fid == NULL)
    {
        fprintf(stderr,"Can't read model file %s\n",model_file);
        exit(1);
    }
    int max_line_length = 1024;
    char *line = Malloc(char,(size_t)max_line_length);
    NDAT line_no = 0;
    struct param param_model;
    set_param_defaults(&param_model);
    bool overwrite = false;
    readSolverInfo(fid, &param_model, &line_no);
    
    // copy partial info from param_model to param
    if(param.nO==0) param.nO = param_model.nO;
    
    // copy number of states from the model
    param.nS = param_model.nS;
    
    // if number of states or observations >2, then no check
    if( param.nO>2 || param.nS>2)
        param.do_not_check_constraints = 1;
    
    //
    // create hmm Object
    //
    HMMProblem *hmm = NULL;
    switch(param.structure)
    {
        case STRUCTURE_SKILL: // Conjugate Gradient Descent
        case STRUCTURE_GROUP: // Conjugate Gradient Descent
            hmm = new HMMProblem(&param);
            break;
    }
    // read model body
    hmm->readModelBody(fid, &param_model, &line_no, overwrite);
      fclose(fid);
    free(line);
    
    if(param.quiet == 0)
        printf("input read, nO=%d, nG=%d, nK=%d, nI=%d\n",param.nO, param.nG, param.nK, param.nI);
    
    clock_t tm = clock();
        metrics = Calloc(NUMBER, (size_t)7);// LL, AIC, BIC, RMSE, RMSEnonull, Acc, Acc_nonull;
    HMMProblem::predict(metrics, predict_file, param.dat_obs, param.dat_group, param.dat_skill, param.dat_skill_stacked, param.dat_skill_rcount, param.dat_skill_rix, &hmm, 1, NULL);
    if(param.quiet == 0)
        printf("predicting is done in %8.6f seconds\n",(NUMBER)(clock()-tm)/CLOCKS_PER_SEC);
        printf("trained model LL=%15.7f (%15.7f), AIC=%8.6f, BIC=%8.6f, RMSE=%8.6f (%8.6f), Acc=%8.6f (%8.6f)\n",
               metrics[0], metrics[1], // ll's
               2*hmm->getNparams() + 2*metrics[0], hmm->getNparams()*safelog(param.N) + 2*metrics[0],
               metrics[2], metrics[3], // rmse's
               metrics[4], metrics[5]); // acc's
    //}
    free(metrics);
    
    destroy_input_data(&param);
    
    delete hmm;
    if(param.quiet == 0)
        printf("overall time running is %8.6f seconds\n",(NUMBER)(clock()-tm0)/CLOCKS_PER_SEC);
    return 0;
}

void exit_with_help() {
    printf(
           "Usage: predicthmm [options] input_file model_file [predicted_response_file]\n"
           "options:\n"
           "-q : quiet mode, without output, 0-no (default), or 1-yes\n"
           "-d : delimiter for multiple skills per observation; 0-single skill per\n"
           "     observation (default), otherwise -- delimiter character, e.g. '-d ~'.\n"
           "-b : treat input file as binary input file (specifications TBA).\n"
           "-p : report model predictions on the train set 0-no (default), 1-yes; 2-yes,\n"
           "     plus output state probability; works with -v and -m parameters.\n"
           "-U : controls how update to the probability distribution of the states is\n"
           "     updated. Takes the following format '-U r|g[,t|g]', where first\n"
           "     character controls how prediction treats known observations, second -- how\n"
           "     prediction treats unknown observations, and third -- whether to output\n"
           "     probabilities of priors. Dealing with known observations 'r' - reveal\n"
           "     actual observations for the update of state probability distribution (makes\n"
           "     sense for modeling how an actual system would work), 'g' - 'guessing' the\n"
           "     observation based on the predicted outcomes (arg max) -- more appropriate\n"
           "     when comparing models (so that no information about observation is never\n"
           "     revealed). Dealing with unknown observations (marked as '.' -- dot): 't' --\n"
           "     use transition matrix only, 'g' -- 'guess' the observation.\n"
           "     Default (if ommitted) is '-U r,t'.\n"
           "     For examle, '-U g,g would require 'guessing' of what the observation was\n"
           "     using model parameters and the running value of the probabilities of state\n"
           "     distributions.\n"
           );
    exit(1);
}

void parse_arguments(int argc, char **argv, char *input_file_name, char *model_file_name, char *predict_file_name) {
    // parse command line options, starting from 1 (0 is path to executable)
    // go in pairs, looking at whether first in pair starts with '-', if not, stop parsing arguments
    int i;
    char * ch;
    for(i=1;i<argc;i++)
    {
        if(argv[i][0] != '-') break; // end of options stop parsing
        if(++i>=argc)
            exit_with_help();
        switch(argv[i-1][1])
        {
            case 'q':
                param.quiet = (NPAR)atoi(argv[i]);
                if(param.quiet!=0 && param.quiet!=1) {
                    fprintf(stderr,"ERROR! Quiet param should be 0 or 1\n");
                    exit_with_help();
                }
                break;
            case  'd':
                param.multiskill = argv[i][0]; // just grab first character (later, maybe several)
                break;
            case 'b':
                param.binaryinput = atoi( strtok(argv[i],"\t\n\r"));
                break;
            case  'p':
                param.predictions = atoi(argv[i]);
                if(param.predictions<0 || param.predictions>2) {
                    fprintf(stderr,"a flag of whether to report predictions for training data (-p) should be 0, 1 or 2\n");
                    exit_with_help();
                }
                break;
            case  'U':
                param.update_known = *strtok(argv[i],",\t\n\r");
                ch = strtok(NULL, ",\t\n\r");
                param.update_unknown = ch[0];
                
                if( (param.update_known!='r' && param.update_known!='g') ||
                   (param.update_unknown!='t' && param.update_unknown!='g') ) {
                    fprintf(stderr,"specification of how probabilities of states should be updated (-U) is incorrect, it sould be r|g[,t|g] \n");
                    exit_with_help();
                }
                break;
            default:
                fprintf(stderr,"unknown option: -%c\n", argv[i-1][1]);
                exit_with_help();
                break;
        }
    }   
    if(param.cv_target_obs>0 && param.metrics>0) { // correct for 0-start coding
        fprintf(stderr,"values for -v and -m cannot be both non-zeros\n");
        exit_with_help();
    }
    
    // next argument should be input (training) file name
    if(i>=argc) // if not
        exit_with_help(); // leave
    
    strcpy(input_file_name, argv[i++]); // copy and advance
    
    if(i>=argc) { // no model file name specified
        fprintf(stderr,"no model file specified\n");
        exit_with_help(); // leave
    }
    else {
        strcpy(model_file_name,argv[i++]); // copy and advance
        if(i>=argc) {// no prediction file name specified
            param.predictions = 0;
        }
        else {
            strcpy(predict_file_name,argv[i]);
        }
    }
}