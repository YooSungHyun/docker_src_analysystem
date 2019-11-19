#include "trainhmm.h"

using namespace std;

static char* readline(FILE *fid) {
    int length = 0;
    
    if(fgets(line,max_line_length,fid) == NULL)
        return NULL;
    
    while(strrchr(line,'\n') == NULL && strrchr(line,'\r') == NULL) // do take both line endings
    {
        max_line_length *= 2;
        line = (char *) realloc(line, (size_t)max_line_length);
        length = (int) strlen(line);
        if(fgets(line+length,max_line_length-length,fid) == NULL)
            break;
    }
    return line;
}

std::string get_results(HMMProblem *hmm) {
    std::string bkt_res = get_params(hmm);

    std::ostringstream stream;
    stream.precision(10);

    stream << "{\n";
    stream <<  "\"metrics\" : {";
    if(param.metrics==1) {
        stream <<  "\"ll\":"   << param.metric_results[0] << ", ";
        stream <<  "\"rmse\":" << param.metric_results[2] << ", ";
        stream <<  "\"acc\":"  << param.metric_results[4];
    }
    stream << "},\n";
    stream << "\"bkt results\" : " << bkt_res << "}";

    return stream.str();

}

std::string get_params(HMMProblem *hmm) {
    NCAT k;
    std::map<NCAT, std::string>::iterator it;
    std::ostringstream stream;
    stream.precision(10);


    // 마스터=0, 취약=1, 추천=2
    stream << "[\n";
    for(k=0;k<param.nK;k++) {
        it = param.map_skill_bwd->find(k);
        stream << "\t{\n\t\"ku_id\" : \""      <<  it->second.c_str() << "\",\n";
        stream << "\t\t\"" << "mstr"           << "\" : " << hmm->PLs.at(it->second.c_str()) << ",\n";
        stream << "\t\t\"" << "init_ku_status" << "\" : " << hmm->getPI()[k][0]   << ",\n";
        stream << "\t\t\"" << "ku_acq"         << "\" : " << hmm->getA()[k][1][0] << ",\n";
        stream << "\t\t\"" << "guess"          << "\" : " << hmm->getB()[k][1][0] << ",\n";
        // stream << "\t\t\"" << "1-guess"          << "\" : " << hmm->getB()[k][1][1] << ",\n";
        stream << "\t\t\"" << "slip"           << "\" : " << hmm->getB()[k][0][1] << "\n";
        // stream << "\t\t\"" << "1-slip"           << "\" : " << hmm->getB()[k][0][0] << "\n";

        if((k+1) == param.nK)
            stream << "\t}";
        else
            stream << "\t},\n";
    }
    stream << "]";

    return stream.str();
}

int main(int argc, char ** argv) {
    // std::string data = "2\tstudent1\tstep1\tskillA\n2\tstudent1\tstep1\tskillA\n2\tstudent1\tstep3\tskillB\n2\tstudent1\tstep4\tskillB\n2\tstudent1\tstep5\tskillC\n2\tstudent1\tstep6\tskillC\n2\tstudent1\tstep7\tskillD\n2\tstudent1\tstep8\tskillD\n2\tstudent1\tstep9\tskillE\n2\tstudent1\tstep10\tskillE\n2\tstudent1\tstep11\tskillF\n2\tstudent1\tstep12\tskillF\n2\tstudent1\tstep13\tskillG\n2\tstudent1\tstep14\tskillG\n2\tstudent1\tstep15\tskillH\n2\tstudent1\tstep16\tskillH\n2\tstudent1\tstep17\tskillI\n2\tstudent1\tstep18\tskillI\n2\tstudent1\tstep19\tskillJ\n2\tstudent1\tstep20\tskillJ\n2\tstudent1\tstep9\tskillK\n2\tstudent1\tstep10\tskillK\n2\tstudent1\tstep11\tskillL\n2\tstudent1\tstep12\tskillL\n";
    // std::string data = "2\tstudent1\tstep1\tskillA\n2\tstudent1\tstep1\tskillA\n2\tstudent1\tstep3\tskillB\n2\tstudent1\tstep4\tskillB\n2\tstudent1\tstep5\tskillC\n2\tstudent1\tstep6\tskillC\n2\tstudent1\tstep7\tskillD\n2\tstudent1\tstep8\tskillD\n2\tstudent1\tstep9\tskillE\n2\tstudent1\tstep10\tskillE\n2\tstudent1\tstep11\tskillF\n2\tstudent1\tstep12\tskillF\n2\tstudent1\tstep13\tskillG\n2\tstudent1\tstep14\tskillG\n2\tstudent1\tstep15\tskillH\n2\tstudent1\tstep16\tskillH\n2\tstudent1\tstep17\tskillI\n2\tstudent1\tstep18\tskillI\n2\tstudent1\tstep19\tskillJ\n2\tstudent1\tstep20\tskillJ\n2\tstudent1\tstep9\tskillK\n2\tstudent1\tstep10\tskillK\n2\tstudent1\tstep11\tskillL\n2\tstudent1\tstep12\tskillL\n2\tstudent1\tstep13\tskillM\n2\tstudent1\tstep14\tskillM\n";
    // std::string data = "2\tsunman0424\tWC201803021051904\tC026\n2\tsunman0424\tWC201803021051904\tC026\n2\tsunman0424\tWC201803021051904\tC026\n2\tsunman0424\tWC201803021051904\tC026\n1\tsunman0424\tWC201803021051904\tC026\n1\tsunman0424\tWC201803021051904\tC026\n1\tsunman0424\tWC201803021051904\tC026\n1\tsunman0424\tWC201803021051904\tC026\n1\tsunman0424\tWC2018030210519040\tC026\n1\tsunman0424\tWC2018030210519041\tC026\n2\tsunman0424\tWC2018030210519042\tC026\n1\tsunman0424\tWC2018030210519043\tC026\n2\tsunman0424\tWC201803021051905\tC026\n1\tsunman0424\tWC201803021051908\tC026\n";
    // std::string data = "1\tjeaweon08\tWC201803051053006\tD067\n1\tjeff0626\tWC201803021051901\tC024\n1\tjeff0626\tWC201803021051902\tC025\n1\tjeff0626\tWC201803021051903\tC025\n1\tjeff0626\tWC201803021051904\tC026\n1\tjeff0626\tWC201803021051905\tC026\n1\tjeff0626\tWC201803021051908\tC026\n1\tjeff0626\tWC201803021051906\tC027\n1\tjeff0626\tWC201803021051907\tC027\n1\tjeff0626\tWC201803021051909\tC027\n1\tjeff0626\tWC201803021051910\tC027\n1\tjeff0626\tWC201802271048101\tC031\n1\tjeff0626\tWC201802271048102\tC031\n1\tjeff0626\tWC201802271048103\tC031\n1\tjeff0626\tWC201802271048104\tC032\n1\tjeff0626\tWC201802271048105\tC032\n1\tjeff0626\tWC201802271048106\tC032\n1\tjeff0626\tWC201802271048107\tC035\n1\tjeff0626\tWC201802271048108\tC035\n1\tjeff0626\tWC201802271048109\tC035\n1\tjeff0626\tWC201802271048110\tC035\n";

    //std::string data = "2\tST\tQA\tKUA\n2\tST\tQB\tKUA\n2\tST\tQC\tKUA\n2\tST\tQD\tKUA\n2\tST\tQE\tKUA\n2\tST\tQF\tKUA\n2\tST\tQG\tKUA\n1\tST\tQH\tKUA\n1\tST\tQI\tKUA\n1\tST\tQJ\tKUA\n1\tST\tQK\tKUA\n2\tST\tQL\tKUA\n2\tST\tQM\tKUA\n1\tST\tQN\tKUA\n1\tST\tQO\tKUA\n1\tST\tQP\tKUA\n2\tST\tQQ\tKUA\n1\tST\tQR\tKUA\n1\tST\tQS\tKUA\n2\tST\tQT\tKUA\n1\tST\tQU\tKUA\n1\tST\tQV\tKUA\n1\tST\tQW\tKUA\n1\tST\tQX\tKUA\n2\tST\tQY\tKUA\n1\tST\tQZ\tKUA\n1\tST\tQa\tKUA\n1\tST\tQb\tKUA\n1\tST\tQc\tKUA\n1\tST\tQd\tKUA\n1\tST\tQe\tKUA\n1\tST\tQf\tKUA\n1\tST\tQg\tKUA\n2\tST\tQh\tKUA\n1\tST\tQi\tKUA\n2\tST\tQj\tKUA\n1\tST\tQk\tKUA\n1\tST\tQl\tKUA\n1\tST\tQm\tKUA\n2\tST\tQn\tKUA\n1\tST\tQo\tKUA\n1\tST\tQp\tKUA\n1\tST\tQq\tKUA\n";
    std::string data = "2\tST\tQA\tKUA\n2\tST\tQB\tKUA\n2\tST\tQC\tKUA\n2\tST\tQD\tKUA\n2\tST\tQE\tKUA\n2\tST\tQF\tKUA\n2\tST\tQG\tKUA\n1\tST\tQH\tKUA\n2\tST\tQI\tKUA\n2\tST\tQJ\tKUA\n2\tST\tQK\tKUA\n2\tST\tQL\tKUA\n2\tST\tQM\tKUA\n1\tST\tQN\tKUA\n2\tST\tQO\tKUA\n2\tST\tQP\tKUA\n2\tST\tQQ\tKUA\n1\tST\tQR\tKUA\n2\tST\tQS\tKUA\n2\tST\tQT\tKUA\n2\tST\tQU\tKUA\n2\tST\tQV\tKUA\n2\tST\tQW\tKUA\n1\tST\tQX\tKUA\n2\tST\tQY\tKUA\n2\tST\tQZ\tKUA\n1\tST\tQa\tKUA\n2\tST\tQb\tKUA\n2\tST\tQc\tKUA\n2\tST\tQd\tKUA\n2\tST\tQe\tKUA\n2\tST\tQf\tKUA\n2\tST\tQg\tKUA\n1\tST\tQh\tKUA\n2\tST\tQi\tKUA\n2\tST\tQj\tKUA\n1\tST\tQk\tKUA\n2\tST\tQl\tKUA\n2\tST\tQm\tKUA\n2\tST\tQn\tKUA\n1\tST\tQo\tKUA\n2\tST\tQp\tKUA\n2\tST\tQq\tKUA\n";

    // std::cout << run(data) << std::endl;
    std::cout << run(data) << std::endl;
}

std::string run(std::string data, std::string PI_0, std::string not_forget, std::string learn, std::string not_slip, std::string guess, std::string metrics) {
    set_initial_params(PI_0, not_forget, learn, not_slip, guess, metrics);
    return run(data);
}

std::string run(std::string data) {
    
    char output_file[] = "";
    char predict_file[] = "";
    char colsole_file[] = "";


    std::string res = "";

    clock_t tm_all = clock();//overall time //SEQ

    set_param_defaults(&param);

    FILE *fid_console = NULL;
    if(param.duplicate_console==1)
        fid_console = fopen(colsole_file,"w");
    
    if(!param.quiet) {
        printf("trainhmm starting...\n");
        if(param.duplicate_console==1) fprintf(fid_console, "trainhmm starting...\n");
    }

    clock_t tm_read = clock();//overall time //SEQ
    int read_ok = read_and_structure_data(data, fid_console);
    tm_read = (clock_t)(clock()-tm_read);//SEQ
    
    if( ! read_ok )
        return "";
    
    
    
    if(!param.quiet) {
        printf("input read, nO=%d, nG=%d, nK=%d, nI=%d\n",param.nO, param.nG, param.nK, param.nI);
        if(param.duplicate_console==1) fprintf(fid_console, "input read, nO=%d, nG=%d, nK=%d, nI=%d\n",param.nO, param.nG, param.nK, param.nI);
    }

    // erase blocking labels
    zeroLabels(&param);

    clock_t tm_fit = 0; //SEQ
    clock_t tm_predict = 0; //SEQ
    
    if(param.cv_folds==0) { // not cross-validation
        // create problem
        HMMProblem *hmm = NULL;
        switch(param.structure)
        {
            case STRUCTURE_SKILL: // Conjugate Gradient Descent
            case STRUCTURE_GROUP: // Conjugate Gradient Descent
                hmm = new HMMProblem(&param);
                break;
        }


        tm_fit = clock(); //SEQ
        hmm->fit();
        tm_fit = clock()-tm_fit;//SEQ
        
        // write model
        // hmm->toFile(output_file);
        
        if(param.metrics>0 || param.predictions>0) {
            NUMBER* metrics = Calloc(NUMBER, (size_t)7); // LL, AIC, BIC, RMSE, RMSEnonull, Acc, Acc_nonull;
            // takes care of predictions and metrics, writes predictions if param.predictions==1
            
            tm_predict = clock(); //SEQ
            
            HMMProblem::predict_nofile(param.metric_results, predict_file, param.dat_obs, param.dat_group, param.dat_skill, param.dat_skill_stacked, param.dat_skill_rcount, param.dat_skill_rix, &hmm, 1, NULL);
            
            tm_predict = clock()-tm_predict;//SEQ
            if( param.metrics>0) {
                // printf("trained model LL=%15.7f (%15.7f), AIC=%8.6f, BIC=%8.6f, RMSE=%8.6f (%8.6f), Acc=%8.6f (%8.6f)\n",
                //        metrics[0], metrics[1], // ll's
                //        2*hmm->getNparams() + 2*metrics[0], hmm->getNparams()*safelog(param.N) + 2*metrics[0],
                //        metrics[2], metrics[3], // rmse's
                //        metrics[4], metrics[5]); // acc's
            }
            res = get_results(hmm);
            free(metrics);
        } // if predict or metrics
        delete hmm;
    } else { // cross-validation
        NUMBER* metrics = Calloc(NUMBER, (size_t)7); // AIC, BIC, RMSE, RMSE no null
        NUMBER n_par = 0;
        switch (param.cv_strat) {
            case CV_GROUP:
                n_par = cross_validate(metrics, predict_file, output_file, &tm_fit, &tm_predict, fid_console);//SEQ
//                cross_validate(metrics, predict_file, output_file, &_tm_fit, &_tm_predict, fid_console);//PAR
                break;
            case CV_ITEM:
                n_par = cross_validate_item(metrics, predict_file, output_file, &tm_fit, &tm_predict, fid_console);//SEQ
//                cross_validate_item(metrics, predict_file, output_file, &_tm_fit, &_tm_predict, fid_console);//PAR
                break;
            case CV_NSTR:
                n_par = cross_validate_nstrat(metrics, predict_file, output_file, &tm_fit, &tm_predict, fid_console);//SEQ
//                cross_validate_nstrat(metrics, predict_file, output_file, &_tm_fit, &_tm_predict, fid_console);//PAR
                break;
            default:
                
                break;
        }

        printf("%d-fold cross-validation: LL=%15.7f (%15.7f), AIC=%8.6f, BIC=%8.6f, RMSE=%8.6f (%8.6f), Acc=%8.6f (%8.6f)\n",
               param.cv_folds,
               metrics[0], metrics[1], // ll's
               2*n_par + 2*metrics[0], n_par*safelog(param.N) + 2*metrics[0],
               metrics[2], metrics[3], // rmse's
               metrics[4], metrics[5]); // acc's
        if(param.duplicate_console==1) fprintf(fid_console, "%d-fold cross-validation: LL=%15.7f (%15.7f), AIC=%8.6f, BIC=%8.6f, RMSE=%8.6f (%8.6f), Acc=%8.6f (%8.6f)\n",
                                               param.cv_folds,
                                               metrics[0], metrics[1], // ll's
                                               2*n_par + 2*metrics[0], n_par*safelog(param.N) + 2*metrics[0],
                                               metrics[2], metrics[3], // rmse's
                                               metrics[4], metrics[5]); // acc's
        free(metrics);
    }
    // free data
    destroy_input_data(&param);
    
    // printf("timing: overall %f seconds, read %f, fit %f, predict %f\n",(NUMBER)((clock()-tm_all)/CLOCKS_PER_SEC), (NUMBER)tm_read/CLOCKS_PER_SEC,  (NUMBER)tm_fit/CLOCKS_PER_SEC,  (NUMBER)tm_predict/CLOCKS_PER_SEC);//SEQ
    if(param.duplicate_console==1) fprintf(fid_console, "timing: overall %f seconds, read %f, fit %f, predict %f\n",(NUMBER)((clock()-tm_all)/CLOCKS_PER_SEC), (NUMBER)tm_read/CLOCKS_PER_SEC,  (NUMBER)tm_fit/CLOCKS_PER_SEC,  (NUMBER)tm_predict/CLOCKS_PER_SEC);//SEQ
    
    if(param.duplicate_console==1)
        fclose(fid_console);
    return res;
}


void set_initial_params(std::string PI_0, std::string not_forget, std::string learn, std::string not_slip, std::string guess, std::string metrics) {
    param.override_init_params = true;
    param.init_params = Calloc(NUMBER, (size_t)5);

    sscanf(PI_0.c_str(),       "%10lf", &param.init_params[0]); // P[0]
    sscanf(not_forget.c_str(), "%10lf", &param.init_params[1]); // p(not forget)
    sscanf(learn.c_str(),      "%10lf", &param.init_params[2]); // p(learn)
    sscanf(not_slip.c_str(),   "%10lf", &param.init_params[3]); // p(not slip)
    sscanf(guess.c_str(),      "%10lf", &param.init_params[4]); // p(guess) 

    param.metrics = atoi(metrics.c_str());
}

void exit_with_help() {
    printf(
           "Usage: trainhmm [options] input_file [[output_file] predicted_response_file]\n"
           "options:\n"
           "-s : structure.solver[.solver setting], structures: 1-by skill, 2-by user;\n"
           "     solvers: 1-Baum-Welch, 2-Gradient Descent, 3-Conjugate Gradient Descent;\n"
           "     Conjugate Gradient Descent has 3 settings: 1-Polak-Ribiere,\n"
           "     2-Fletcher–Reeves, 3-Hestenes-Stiefel, 4-Dai-Yuan.\n"
           "     For example '-s 1.3.1' would be by skill structure (classical) with\n"
           "     Conjugate Gradient Descent and Hestenes-Stiefel formula, '-s 2.1' would be\n"
           "     by student structure fit using Baum-Welch method.\n"
           "-e : tolerance of termination criterion (0.01 for parameter change default);\n"
           "     could be compuconvergeted by the change in log-likelihood per datapoint, e.g.\n"
           "     '-e 0.00001,l'.\n"
           "-i : maximum iterations (200 by default)\n"
           "-q : quiet mode, without output, 0-no (default), or 1-yes\n"
           "-n : number of hidden states, should be 2 or more (default 2)\n"
           "-0 : initial parameters comma-separated for priors, transition, and emission\n"
           "     probabilities skipping the last value from each vector (matrix row) since\n"
           "     they sum up to 1; default 0.5,1.0,0.4,0.8,0.2\n"
           "-l : lower boundaries for parameters, comma-separated for priors, transition,\n"
           "     and emission probabilities (without skips); default 0,0,1,0,0,0,0,0,0,0\n"
           "-u : upper boundaries for params, comma-separated for priors, transition,\n"
           "     and emission probabilities (without skips); default 0,0,1,0,0,0,0,0,0,0\n"
           "-c : specification of the C weight and cetroids for L2 penalty, empty (default).\n"
           "     For standard BKT - 4 comma-separated numbers: C weight of the penalty and\n"
           "     centroids, for PI, A, and B matrices respectively.\n"
           "     For example, '-c 1.0,0.5,0.5,0.0'."
           "-f : fit as one skill, 0-no (default), 1 - fit as one skill and use params as\n"
           "     starting point for multi-skill, 2 - force one skill\n"
           "-m : report model fitting metrics (AIC, BIC, RMSE) 0-no (default), 1-yes. To \n"
           "     specify observation for which metrics to be reported, list it after ','.\n"
           "     For example '-m 0', '-m 1' (by default, observation 1 is assumed), '-m 1,2'\n"
           "     (compute metrics for observation 2). Incompatible with-v option.\n"
           "-v : cross-validation folds, stratification, and target state to validate\n"
           "     against, default 0 (no cross-validation),\n"
           "     examples '-v 5,i,2' - 5 fold, item-stratified c.-v., predict state 2,\n"
           "     '-v 10' - 10-fold subject-stratified c.-v. predict state 1 by default,\n"
           "     alternatively '-v 10,g,1', and finally '-v 5,n,2,' - 5-fold unstratified\n"
           "     c.-v. predicting state 1.\n"
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
           "-d : delimiter for multiple skills per observation; 0-single skill per\n"
           "     observation (default), otherwise -- delimiter character, e.g. '-d ~'.\n"
           "-b : treat input file as binary input file (specifications TBA).\n"
           "-B : block re-estimation of prior, transitions, or emissions parameters\n"
           "     respectively (defailt is '-B 0,0,0'), to block re-estimation of transition\n"
           "     probabilities specify '-B 0,1,0'.\n"
           "-P : use parallel processing, defaul - 0 (no parallel processing), 1 - fit\n"
           "     separate skills/students separately, 2 - fit separate sequences within\n"
           "     skill/student separately.\n"
           "-o : in addition to printing to console, print output to the file specified\n"
           "     default is empty.\n"
           );
    exit(1);
}

void parse_arguments_step1(int argc, char **argv, char *input_file_name, char *output_file_name, char *predict_file_name, char *console_file_name) {
    // parse command line options, starting from 1 (0 is path to executable)
    // go in pairs, looking at whether first in pair starts with '-', if not, stop parsing arguments

    // at this time we do not know nO (the number of observations) yet
    int i;
    int n;
    char *ch, *ch2;
    for(i=1;i<argc;i++)
    {
        if(argv[i][0] != '-') break; // end of options stop parsing
        if(++i>=argc)
            exit_with_help();
        switch(argv[i-1][1])
        {
            case 'e':
                param.tol = atof( strtok(argv[i],",\t\n\r") );
                ch = strtok(NULL,",\t\n\r"); // could be NULL
                if(ch != NULL)
                    param.tol_mode = ch[0];
                if(param.tol<0) {
                    fprintf(stderr,"ERROR! Fitting tolerance cannot be negative\n");
                    exit_with_help();
                }
                if(param.tol>10) {
                    fprintf(stderr,"ERROR! Fitting tolerance cannot be >10\n");
                    exit_with_help();
                }
                if(param.tol_mode!='p' && param.tol_mode!='l') {
                    fprintf(stderr,"ERROR! Tolerance mode '%c' is not allowed\n",param.tol_mode);
                    exit_with_help();
                }
                break;
            case 'i':
                param.maxiter = atoi(argv[i]);
                if(param.maxiter<10) {
                    fprintf(stderr,"ERROR! Maximum iterations should be at least 10\n");
                    exit_with_help();
                }
                break;
            case 'q':
                param.quiet = (NPAR)atoi(argv[i]);
                if(param.quiet!=0 && param.quiet!=1) {
                    fprintf(stderr,"ERROR! Quiet param should be 0 or 1\n");
                    exit_with_help();
                }
                break;
            case 'n':
                param.nS = (NPAR)atoi(argv[i]);
                if(param.nS<2) {
                    fprintf(stderr,"ERROR! Number of hidden states should be at least 2\n");
                    exit_with_help();
                }
                if(param.nS != 2) {
                    param.stat_specd_gt2 = true;
                }
                break;
            case 'S':
                param.scaled = (NPAR)atoi(argv[i]);
                if(param.scaled < 0 || param.scaled > 1) {
                    fprintf(stderr,"ERROR! Scaling flag should be either 0 (off) or 1 (in)\n");
                    exit_with_help();
                }
                break;
            case 's':
                param.structure = (NPAR)atoi( strtok(argv[i],".\t\n\r") );
                ch = strtok(NULL,".\t\n\r"); // could be NULL (default GD solver)
                if(ch != NULL)
                    param.solver = (NPAR)atoi(ch);
                ch = strtok(NULL,"\t\n\r"); // could be NULL (default GD solver)
                if(ch != NULL)
                    param.solver_setting = (NPAR)atoi(ch);
                if( param.structure != STRUCTURE_SKILL && param.structure != STRUCTURE_GROUP  ) {
                    fprintf(stderr, "Model Structure specified (%d) is out of range of allowed values\n",param.structure);
                    exit_with_help();
                }
                if( param.solver != METHOD_BW  && param.solver != METHOD_GD &&
                   param.solver != METHOD_CGD && param.solver != METHOD_GDL &&
                   param.solver != METHOD_GBB) {
                    fprintf(stderr, "Method specified (%d) is out of range of allowed values\n",param.solver);
                    exit_with_help();
                }
                if( param.solver == METHOD_BW && ( param.solver != STRUCTURE_SKILL && param.solver != STRUCTURE_GROUP ) ) {
                    fprintf(stderr, "Baum-Welch solver does not support model structure specified (%d)\n",param.solver);
                    exit_with_help();
                }
                if( param.solver == METHOD_CGD  &&
                   ( param.solver_setting != 1 && param.solver_setting != 2 &&
                    param.solver_setting != 3 && param.solver_setting !=4 )
                   ) {
                    fprintf(stderr, "Conjugate Gradient Descent setting specified (%d) is out of range of allowed values\n",param.solver_setting);
                    exit_with_help();
                }
                break;
            case 'f':
                param.single_skill = (NPAR)atoi(argv[i]);
                break;
            case 'm':
                param.metrics = atoi( strtok(argv[i],",\t\n\r"));
                ch = strtok(NULL, "\t\n\r");
                if(ch!=NULL)
                    param.metrics_target_obs = atoi(ch)-1;
                if(param.metrics<0 || param.metrics>1) {
                    fprintf(stderr,"value for -m should be either 0 or 1.\n");
                    exit_with_help();
                }
                if(param.metrics_target_obs<0) {
                    fprintf(stderr,"target observation to compute metrics against cannot be '%d'\n",param.metrics_target_obs+1);
                    exit_with_help();
                }
                break;
            case 'b':
                param.binaryinput = atoi( strtok(argv[i],"\t\n\r"));
                break;
            case 'v':
                param.cv_folds   = (NPAR)atoi( strtok(argv[i],",\t\n\r"));
                ch2 = strtok(NULL, ",\t\n\r");
                if(ch2!=NULL)
                    param.cv_strat = ch2[0];
                ch = strtok(NULL, ",\t\n\r");
                if(ch!=NULL)
                    param.cv_target_obs = (NPAR)(atoi(ch)-1);
                ch = strtok(NULL, ",\t\n\r");
                if(ch!=NULL)
                    strcpy(param.cv_folds_file, ch);
                ch = strtok(NULL, ",\t\n\r");
                if(ch!=NULL)
                    param.cv_inout_flag = ch[0];
                
                if(param.cv_folds<2) {
                    fprintf(stderr,"number of cross-validation folds should be at least 2\n");

                    exit_with_help();
                }
                if(param.cv_folds>10) {
                    fprintf(stderr,"please keep number of cross-validation folds less than or equal to 10\n");
                    exit_with_help();
                }
                if(param.cv_strat != CV_GROUP && param.cv_strat != CV_ITEM && param.cv_strat != CV_NSTR){
                    fprintf(stderr,"cross-validation stratification parameter '%c' is illegal\n",param.cv_strat);
                    exit_with_help();
                }
                if(param.cv_target_obs<0) {
                    fprintf(stderr,"target observation to be cross-validated against cannot be '%d'\n",param.cv_target_obs+1);
                    exit_with_help();
                }
                if( param.cv_inout_flag!='i' && param.cv_inout_flag!='o') {
                    fprintf(stderr,"cross-validation folds input/output flag should be ither 'o' (out) or 'i' (in), while it is '%c'\n",param.cv_inout_flag);
                    exit_with_help();
                }
                
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
            case  'd':
                param.multiskill = argv[i][0]; // just grab first character (later, maybe several)
                break;
            case  'P':
                n = atoi(argv[i]);
                if(n!=0 && n!=1 && n!=2) {
                    fprintf(stderr,"parallel processing flag (-P) should be 0 or 1\n");
                    exit_with_help();
                }
                param.parallel = (NPAR)n;
                break;
            case 'c': {
                    StripedArray<NUMBER> * tmp_array = new StripedArray<NUMBER>();
                    ch = strtok(argv[i],",\t\n\r");
                    while( ch != NULL ) {
                        tmp_array->add( atof(ch) );
                        ch = strtok(NULL,",\t\n\r");
                    }
                    if( (tmp_array->getSize() % 4) != 0 ) {
                        fprintf(stderr,"The number of regularization parameters should be a multiple of 4 and it is %d\n",tmp_array->getSize());
                        exit_with_help();
                    }
                    param.Cslices = (NPAR) tmp_array->getSize() / 4;
                    param.Cw = Calloc(NUMBER, (size_t)param.Cslices);
                    param.Ccenters = Calloc(NUMBER, (size_t)(param.Cslices * 3) );
                    int c1 = 0, c2 = 0, i = 0;
                    for(int l=0; l<(int)tmp_array->getSize() / 4; l++) {
                        param.Cw[c1++] = tmp_array->get((NDAT)i++);
                        for(int j=0; j<3; j++)
                            param.Ccenters[c2++] = tmp_array->get((NDAT)i++);
                    }
                    delete tmp_array;
                }
                break;
            case 'o':
                param.duplicate_console = 1;
                strcpy(console_file_name,argv[i]);
                break;
            case '0':
                param.init_reset = true;
                break;
            case 'l': // just to keep it a valid option
                break;
            case 'u': // just to keep it a valid option
                break;
            case 'B': // just to keep it a valid option
                break;
            default:
                fprintf(stderr,"unknown option: -%c\n", argv[i-1][1]);
                exit_with_help();
                break;
        }
    }
    // post-process checks
    // -v and -m collision
    if(param.cv_folds>0 && param.metrics>0) { // correct for 0-start coding
        fprintf(stderr,"values for -v and -m cannot be both non-zeros\n");
        exit_with_help();
    }
    // scaling
    if(param.scaled == 1 && param.solver != METHOD_BW) {
        param.scaled = 0;
        printf("Scaling can only be enabled for Baum-Welch method. Setting it to off\n");
    }
    // specifying >2 states via -n and mandatory specification of -0 (initial parameters)
    if(param.nS > 2 && !param.init_reset) {
        fprintf(stderr,"when >2 latent states specified via '-n', initial values of parameters have to be explicitly set via '-0'!\n");
        exit_with_help();
    }
    
    // next argument should be input file name
    if(i>=argc) // if not
        exit_with_help(); // leave
    
    strcpy(input_file_name, argv[i++]); // copy and advance
    
    if(i>=argc) { // no output file name specified
        strcpy(output_file_name,"output.hmm");
        strcpy(predict_file_name,"predict_hmm.txt"); // the predict file too
    }
    else {
        strcpy(output_file_name,argv[i++]); // copy and advance
        if(i>=argc) // no prediction file name specified
            strcpy(predict_file_name,"predict_hmm.txt"); // the predict file too
        else
            strcpy(predict_file_name,argv[i]);
    }
}

void parse_arguments_step2(int argc, char **argv, FILE *fid_console) {
    // parse command line options, starting from 1 (0 is path to executable)
    // go in pairs, looking at whether first in pair starts with '-', if not, stop parsing arguments
    
    // at this time we do know nO (the number of observations)
    int i;
    int n;
    char *ch;
    for(i=1;i<argc;i++)
    {
        if(argv[i][0] != '-') break; // end of options stop parsing
        if(++i>=argc)
            exit_with_help();
        switch(argv[i-1][1])
        {
            case '0': // init_params
                int len;
                len = (int)strlen( argv[i] );
                // count delimiters
                n = 1; // start with 1
                for(int j=0;j<len;j++) {
                    n += (argv[i][j]==',')?1:0;
                    if( (argv[i][j] >= 'a' && argv[i][j] <= 'z') || (argv[i][j] >= 'A' && argv[i][j] <= 'Z') ) {
                        strcpy(param.initfile, argv[i]);
                        break;
                    }
                }
                if(param.initfile[0]==0) { // init parameters parameters
                    // init params
                    if(param.init_params!=NULL) free(param.init_params);
                    param.init_params = Calloc(NUMBER, (size_t)n);
                    // read params and write to params
                    param.init_params[0] = atof( strtok(argv[i],",\t\n\r") );
                    for(int j=1; j<n; j++) {
                        param.init_params[j] = atof( strtok(NULL,",\t\n\r") );
                    }
                }
                break;
            case 'l': // lower boundaries
                len = (int)strlen( argv[i] );
                // count delimiters
                n = 1; // start with 1
                for(int j=0;j<len;j++)
                    n += (NPAR)((argv[i][j]==',')?1:0);
                // init params
                if(param.param_lo!=NULL) free(param.param_lo);
                param.param_lo = Calloc(NUMBER, (size_t)n);
                // read params and write to params
                param.param_lo[0] = atof( strtok(argv[i],",\t\n\r") );
                for(int j=1; j<n; j++) {
                    param.param_lo[j] = atof( strtok(NULL,",\t\n\r") );
                }
                param.lo_lims_specd = true;
                break;
            case 'u': // upper boundaries
                len = (int)strlen( argv[i] );
                // count delimiters
                n = 1; // start with 1
                for(int j=0;j<len;j++)
                    n += (argv[i][j]==',')?1:0;
                // init params
                if(param.param_hi!=NULL) free(param.param_hi);
                param.param_hi = Calloc(NUMBER, (size_t)n);
                // read params and write to params
                param.param_hi[0] = atof( strtok(argv[i],",\t\n\r") );
                for(int j=1; j<n; j++) {
                    param.param_hi[j] = atof( strtok(NULL,",\t\n\r") );
                }
                param.hi_lims_specd = true;
                break;
            case 'B': // block fitting
                // first
                param.block_fitting[0] = (NPAR)atoi( strtok(argv[i],",\t\n\r") );
                if(param.block_fitting[0]!=0 && param.block_fitting[0]!=1) {
                    fprintf(stderr,"Values of blocking the fitting flags shuld only be 0 or 1.\n");
                    if(param.duplicate_console==1) fprintf(fid_console,"Values of blocking the fitting flags shuld only be 0 or 1.\n");
                    exit_with_help();
                }
                // second
                ch = strtok(NULL,",\t\n\r"); // could be NULL (default GD solver)
                if(ch != NULL) {
                    param.block_fitting[1] = (NPAR)atoi(ch);
                    if(param.block_fitting[1]!=0 && param.block_fitting[1]!=1) {
                        fprintf(stderr,"Values of blocking the fitting flags shuld only be 0 or 1.\n");
                        if(param.duplicate_console==1) fprintf(fid_console,"Values of blocking the fitting flags shuld only be 0 or 1.\n");
                        exit_with_help();
                    }
                }
                else {
                    fprintf(stderr,"There should be 3 blockig the fitting flags specified.\n");
                    if(param.duplicate_console==1) fprintf(fid_console,"There should be 3 blockig the fitting flags specified.\n");
                    exit_with_help();
                }
                // third
                ch = strtok(NULL,",\t\n\r"); // could be NULL (default GD solver)
                if(ch != NULL) {
                    param.block_fitting[2] = (NPAR)atoi(ch);
                    if(param.block_fitting[2]!=0 && param.block_fitting[2]!=1) {
                        fprintf(stderr,"Values of blocking the fitting flags shuld only be 0 or 1.\n");
                        if(param.duplicate_console==1) fprintf(fid_console,"Values of blocking the fitting flags shuld only be 0 or 1.\n");
                        exit_with_help();
                    }
                }
                else {
                    fprintf(stderr,"There should be 3 blockig the fitting flags specified.\n");
                    if(param.duplicate_console==1) fprintf(fid_console,"There should be 3 blockig the fitting flags specified.\n");
                    exit_with_help();
                }
                break;
        } // end switch
    }// end for
    // post parse actions
    
    if(!param.lo_lims_specd && (param.nS!=2 || param.nO!=2) ) { // if not specified, and it's not 2-state 2-obs case, set to 0
        if(param.param_lo!=NULL) free(param.param_lo);
        param.param_lo = Calloc(NUMBER, (size_t)( param.nS*(1+param.nS+param.nO) ) );
    }
 
    if(!param.hi_lims_specd && (param.nS!=2 || param.nO!=2) ) {  // if not specified, and it's not 2-state 2-obs case, set to 1
        if(param.param_hi!=NULL) free(param.param_hi);  // if not specified, set to 1
        param.param_hi = Calloc(NUMBER, (size_t)( param.nS*(1+param.nS+param.nO) ) );
        for(int j=0; j<( param.nS*(1+param.nS+param.nO) ); j++)
            param.param_hi[j] = (NUMBER)1.0;
    }
    
    // post-argument checks - TODO - enable
    if( param.cv_target_obs>(param.nO-1)) {
        fprintf(stderr,"target observation to be cross-validated against cannot be '%d'\n",param.cv_target_obs+1);
        if(param.duplicate_console==1) fprintf(fid_console,"target observation to be cross-validated against cannot be '%d'\n",param.cv_target_obs+1);
        exit_with_help();
    }
    if(param.metrics_target_obs>(param.nO-1)) {
        fprintf(stderr,"target observation to compute metrics against cannot be '%d'\n",param.metrics_target_obs+1);
        if(param.duplicate_console==1) fprintf(fid_console,"target observation to compute metrics against cannot be '%d'\n",param.metrics_target_obs+1);
        exit_with_help();
    }
    
}

bool read_and_structure_data(std::string data, FILE *fid_console) {
    bool readok = true;
    if(param.binaryinput==0)
        readok = InputUtil::readTxt(data, &param);
    else
        readok = InputUtil::readBin(0, &param); // NOT USED
    if(! readok )
        return false;
    
    //    2. distribute data into nK skill bins
    //        create
    //          skill_group_map[nK][nG] - explicit 'sparse' map of skills and groups, here 1 means done
    //            k_numg[nK]        - number of groups per skill                 RETAIN
    
    NDAT t = 0;
    NDAT t_stacked = 0;
    NCAT g, k;
//    NPAR o;
    NPAR **skill_group_map = init2D<NPAR>(param.nK, param.nG); // binary map of skills to groups
    param.k_numg = Calloc(NCAT, (size_t)param.nK);
    param.g_numk = Calloc(NCAT, (size_t)param.nG);
    NDAT *count_null_skill_group = Calloc(NDAT, (size_t)param.nG); // count null skill occurences per group
    NCAT *index_null_skill_group = Calloc(NCAT, (size_t)param.nG); // index of group in compressed array
    
    // Pass A
    for(t=0; t<param.N; t++) {
        if(param.multiskill==0)
            k = param.dat_skill[t];//[t];
        else
            k = param.dat_skill_stacked[ param.dat_skill_rix[t] ]; // first skill of multi-skill

        g = param.dat_group[t];//[t];
        // null skill : just count
        if( k < 0 ) {
            if(count_null_skill_group[g]==0) param.n_null_skill_group++;
            count_null_skill_group[g]++;
            continue;
        }
        NCAT *ar;
        int n;
        if(param.multiskill==0) {
            k = param.dat_skill[t];
            ar = &k;
            n = 1;
        } else {
            k = param.dat_skill_stacked[ param.dat_skill_rix[t] ];
            ar = &param.dat_skill_stacked[ param.dat_skill_rix[t] ];
            n = param.dat_skill_rcount[t];
        }
        for(int l=0; l<n; l++) {
            k = ar[l];
            if( skill_group_map[k][g] == 0 ) {
                skill_group_map[k][g] = 1;
                param.k_numg[k]++;
                param.g_numk[g]++;
            }
        }
    }
    for(k=0; k<param.nK; k++) param.nSeq += param.k_numg[k];
    param.all_data = Calloc(struct data, (size_t)param.nSeq);
    
    // Section B
    param.k_g_data = Malloc(struct data **, (size_t)param.nK);
    param.k_data = Malloc(struct data *, (size_t)param.nSeq);
    param.g_k_data = Calloc(struct data **, (size_t)param.nG);
    param.g_data = Malloc(struct data *, (size_t)param.nSeq);
    param.null_skills = Calloc(struct data, (size_t)param.n_null_skill_group);
    // index compressed array of null-skill-BY-group
    NCAT idx = 0;
    for(g=0; g<param.nG; g++)
        if( count_null_skill_group[g] >0 ) index_null_skill_group[g] = idx++;
    
    // Pass C
    NDAT *k_countg = Calloc(NDAT, (size_t)param.nK); // track current group in skill
    NDAT *g_countk = Calloc(NDAT, (size_t)param.nG); // track current skill in group
    // set k_countg and g_countk pointers to relative positions
    NDAT sumk=0, sumg=0;
    for(k=0; k<param.nK; k++) {
        k_countg[k] = sumk;
        param.k_g_data[k] = &param.k_data[sumk];
        sumk += param.k_numg[k];
    }
    for(g=0; g<param.nG; g++) {
        g_countk[g] = sumg;
        param.g_k_data[g] = &param.g_data[sumg];
        sumg += param.g_numk[g];
    }
    
    NDAT n_all_data = 0;
    for(t=0; t<param.N; t++) {
        NCAT *ar;
        int n;
        if(param.multiskill==0) {
            k = param.dat_skill[t];
            ar = &k;
            n = 1;
        } else {
            k = param.dat_skill_stacked[ param.dat_skill_rix[t] ];
            ar = &param.dat_skill_stacked[ param.dat_skill_rix[t] ];
            n = param.dat_skill_rcount[t];
        }
        for(int l=0; l<n; l++) {
            k = ar[l];
            g = param.dat_group[t];//[t];
            // now allocate space for the data
            if( k < 0 ) {
                NCAT gidx = index_null_skill_group[g];
                if( param.null_skills[gidx].ix != NULL) // was obs // check if we allocated it already
                    continue;
                param.null_skills[gidx].n = count_null_skill_group[g];
                param.null_skills[gidx].g = g;
                param.null_skills[gidx].k = -1;
                param.null_skills[gidx].cnt = 0;
                param.null_skills[gidx].ix = Calloc(NDAT, (size_t)count_null_skill_group[g]);
                if(param.multiskill!=0)
                    param.null_skills[gidx].ix_stacked = Calloc(NDAT, (size_t)count_null_skill_group[g]);
                // no time for null skills is necessary
                param.null_skills[gidx].alpha = NULL;
                param.null_skills[gidx].beta = NULL;
                param.null_skills[gidx].gamma = NULL;
                param.null_skills[gidx].xi = NULL;
                param.null_skills[gidx].c = NULL;
                param.null_skills[gidx].p_O_param = 0.0;
                continue;
            }
            if( skill_group_map[k][g]==0) {
                fprintf(stderr, "ERROR! position [%d,%d] in skill_group_map should have been 1\n",k,g);
                if(param.duplicate_console==1) fprintf(fid_console,"ERROR! position [%d,%d] in skill_group_map should have been 1\n",k,g);
            } else if( skill_group_map[k][g]==1 ) { // insert new sequence and grab new data
                // link
                param.k_data[ k_countg[k] ] = &param.all_data[n_all_data]; // in linear array
                param.g_data[ g_countk[g] ] = &param.all_data[n_all_data]; // in linear array
                param.all_data[n_all_data].n = 1; // init data << VV
                param.all_data[n_all_data].k = k; // init k
                param.all_data[n_all_data].g = g; // init g
                param.all_data[n_all_data].cnt = 0;
                param.all_data[n_all_data].ix = NULL;
                param.all_data[n_all_data].ix_stacked = NULL;
                param.all_data[n_all_data].alpha = NULL;
                param.all_data[n_all_data].beta = NULL;
                param.all_data[n_all_data].gamma = NULL;
                param.all_data[n_all_data].xi = NULL;
                param.all_data[n_all_data].c = NULL;
                param.all_data[n_all_data].p_O_param = 0.0;
                param.all_data[n_all_data].loglik = 0.0;
                k_countg[k]++; // count
                g_countk[g]++; // count
                skill_group_map[k][g] = 2; // mark
                n_all_data++;
            } else if( skill_group_map[k][g]== 2) { // update data count, LINEAR SEARCH :(
                int gidx;
                for(gidx=(k_countg[k]-1); gidx>=0 && param.k_data[gidx]->g!=g; gidx--)
                    ;
                if( param.k_data[gidx]->g==g)
                    param.k_data[gidx]->n++;
                else {
                    fprintf(stderr, "ERROR! position of group %d in skill %d not found\n",g,k);
                    if(param.duplicate_console==1) fprintf(fid_console,"ERROR! position of group %d in skill %d not found\n",g,k);
                }
            }
        }
    }
    // recycle
    free(k_countg);
    free(g_countk);
    free(count_null_skill_group);
    
    //    3 fill data
    //        pass A
    //            fill k_g_data.data (g_k_data is already linked)
    //                using skill_group_map as marker, 3 - data grabbed
    k_countg = Calloc(NDAT, (size_t)param.nK); // track current group in skill
    g_countk = Calloc(NDAT, (size_t)param.nG); // track current skill in group
    for(t=0; t<param.N; t++) {
        g = param.dat_group[t];
        NCAT *ar;
        int n;
        if(param.multiskill==0) {
            k = param.dat_skill[t];
            ar = &k;
            n = 1;
        } else {
            k = param.dat_skill_stacked[ param.dat_skill_rix[t] ];
            ar = &param.dat_skill_stacked[ param.dat_skill_rix[t] ];
            n = param.dat_skill_rcount[t];
        }
        for(int l=0; l<n; l++) {
            k = ar[l];
            if( k < 0 ) {
                NCAT gidx = index_null_skill_group[g];
                param.null_skills[gidx].ix[ param.null_skills[gidx].cnt++ ] = t; // use .cnt as counter
                if(param.multiskill!=0)
                    param.null_skills[gidx].ix_stacked[ param.null_skills[gidx].cnt-1 ] = t_stacked; // use prior value version of cnt that was increased for ix
                continue;
            }
            if( skill_group_map[k][g]<2) {
                fprintf(stderr, "ERROR! position [%d,%d] in skill_group_map should have been 2\n",k,g);
                if(param.duplicate_console==1) fprintf(fid_console, "ERROR! position [%d,%d] in skill_group_map should have been 2\n",k,g);
            } else if( skill_group_map[k][g]==2 ) { // grab data and insert first dat point
                param.k_g_data[k][ k_countg[k] ]->ix = Calloc(NDAT, (size_t)param.k_g_data[k][ k_countg[k] ]->n); // grab
                param.k_g_data[k][ k_countg[k] ]->ix[0] = t; // insert
                if(param.multiskill!=0) {
                    param.k_g_data[k][ k_countg[k] ]->ix_stacked = Calloc(NDAT, (size_t)param.k_g_data[k][ k_countg[k] ]->n); // grab
                    param.k_g_data[k][ k_countg[k] ]->ix_stacked[0] = t_stacked; // first stacked index
                }
                param.k_g_data[k][ k_countg[k] ]->cnt++; // increase data counter
                k_countg[k]++; // count unique groups forward
                g_countk[g]++; // count unique skills forward
                skill_group_map[k][g] = 3; // mark
            } else if( skill_group_map[k][g]== 3) { // insert next data point and inc counter, LINEAR SEARCH :(
                NCAT gidx;
                for(gidx=(k_countg[k]-(NCAT)1); gidx>=0 && param.k_g_data[k][gidx]->g!=g; gidx--)
                    ; // skip
                if( param.k_g_data[k][gidx]->g==g ) {
                    NDAT pos = param.k_g_data[k][ gidx ]->cnt; // copy position
                    param.k_g_data[k][ gidx ]->ix[pos] = t; // insert
                    if(param.multiskill!=0)
                        param.k_g_data[k][ gidx ]->ix_stacked[pos] = t_stacked; // insert
                    param.k_g_data[k][ gidx ]->cnt++; // increase data counter
                }
                else {
                    fprintf(stderr, "ERROR! position of group %d in skill %d not found\n",g,k);
                    if(param.duplicate_console==1) fprintf(fid_console, "ERROR! position of group %d in skill %d not found\n",g,k);
                }
            }
            t_stacked++;
        }// all skills on the row
    } // all t
    // recycle
    free(k_countg);
    free(g_countk);
    free(index_null_skill_group);
    free2D<NPAR>(skill_group_map, param.nK);
    // reset `cnt'
    for(g=0; g<param.nG; g++) // for all groups
        for(k=0; k<param.g_numk[g]; k++) // for all skills in it
            param.g_k_data[g][k]->cnt = 0;
    for(NCAT x=0; x<param.n_null_skill_group; x++)
        param.null_skills[x].cnt = 0;

    return true;
}

NUMBER cross_validate(NUMBER* metrics, const char *filename, const char *model_file_name, clock_t *tm_fit, clock_t *tm_predict, FILE *fid_console) {//SEQ
    clock_t tm0;//SEQ
    char *ch;
    NPAR f;
    NCAT g,k;
    FILE *fid = NULL; // file for storing prediction should that be necessary
    FILE *fid_folds = NULL; // file for reading/writing folds
    if(param.predictions>0) {  // if we have to write the predictions file
        fid = fopen(filename,"w");
        if(fid == NULL)
        {
            fprintf(stderr, "Can't write output model file %s\n",filename);
            if(param.duplicate_console==1) fprintf(fid_console, "Can't write output model file %s\n",filename);
            exit(1);
        }
    }
    // produce folds
    NPAR *folds = Calloc(NPAR, (size_t)param.nG);
    srand ( (unsigned int)time(NULL) );
    // folds file
    if(param.cv_folds_file[0] > 0) { // file is specified
        if(param.cv_inout_flag=='i') {
            fid_folds = fopen(param.cv_folds_file,"r");
            if(fid_folds == NULL)
            {
                fprintf(stderr, "Can't open folds file for reading %s\n",filename);
                if(param.duplicate_console==1) fprintf(fid_console, "Can't open folds file for reading %s\n",filename);
                exit(1);
            }
            max_line_length = 1024;
            line = (char *)malloc((size_t)max_line_length);
        } else if(param.cv_inout_flag=='o') {
            fid_folds = fopen(param.cv_folds_file,"w");
            if(fid_folds == NULL)
            {
                fprintf(stderr, "Can't open folds file for writing %s\n",filename);
                if(param.duplicate_console==1) fprintf(fid_console, "Can't open folds file for writing %s\n",filename);
                exit(1);
            }
        }
    }
    for(g=0; g<param.nG; g++) {
        if( param.cv_folds_file[0]==0 || (param.cv_folds_file[0] > 0 && param.cv_inout_flag=='o') ) { // output or default
            folds[g] = (NPAR)(rand() % param.cv_folds);
            if(param.cv_folds_file[0] > 0 )
                fprintf(fid_folds,"%i\n",folds[g]); // write out
        } else if( (param.cv_folds_file[0] > 0 && param.cv_inout_flag=='i') ) {
            readline(fid_folds);//
            ch = strtok(line,"\t\n\r");
            if(ch == NULL) {
                fprintf(stderr, "Error reading input folds file (potentialy wrong number of rows)\n");
                if(param.duplicate_console==1) fprintf(fid_console, "Error reading input folds file (potentialy wrong number of rows)\n");
                exit(1);
            }
            folds[g] = (NPAR)(atoi(ch));
        }
    }
    if(param.cv_folds_file[0] > 0) { // file is specified
        fclose(fid_folds);
        if(param.cv_inout_flag=='i')
            free(line);
    }
    param.metrics_target_obs = param.cv_target_obs;

    
    // create and fit multiple problems
    HMMProblem* hmms[param.cv_folds];
    int q = param.quiet;
    param.quiet = 1;
    for(f=0; f<param.cv_folds; f++) {
        switch(param.structure)
        {
            case STRUCTURE_SKILL: // Conjugate Gradient Descent
            case STRUCTURE_GROUP: // Conjugate Gradient Descent
                hmms[f] = new HMMProblem(&param);
                break;
       }
        // block respective data - do not fit the data belonging to the fold
        for(g=0; g<param.nG; g++) // for all groups
            if(folds[g]==f) { // if in current fold
                for(k=0; k<param.g_numk[g]; k++) // for all skills in it
                    param.g_k_data[g][k]->cnt = 1; // block it
            }
        // block nulls
        for(NCAT x=0; x<param.n_null_skill_group; x++) {
            if( param.null_skills[x].g == f)
                param.null_skills[x].cnt = 1;
        }

        // now compute
        tm0 = clock(); //SEQ
        hmms[f]->fit();
        *(tm_fit) += (clock_t)(clock()- tm0);//SEQ
        
        // write model
        char fname[1024];
        sprintf(fname,"%s_%i",model_file_name,f);
        hmms[f]->toFile(fname);
        
        // UN-block respective data
        for(g=0; g<param.nG; g++) // for all groups
            if(folds[g]==f) { // if in current fold
                for(k=0; k<param.g_numk[g]; k++) // for all skills in it
                    param.g_k_data[g][k]->cnt = 0; // UN-block it
            }
        // UN-block nulls
        for(NCAT x=0; x<param.n_null_skill_group; x++) {
            if( param.null_skills[x].g == f)
                param.null_skills[x].cnt = 0;
        }
        if(q == 0) {
            printf("fold %d is done\n",f+1);
            if(param.duplicate_console==1) fprintf(fid_console,"fold %d is done\n",f+1);
        }
    }
    param.quiet = (NPAR)q;
    
    tm0 = clock();//SEQ
    
    // new prediction
    //        create a general fold-identifying array
    NPAR *dat_fold = Calloc(NPAR, param.N);
    for(NDAT t=0; t<param.N; t++) dat_fold[t] = folds[ param.dat_group[t] ];
    //        predict
    HMMProblem::predict(metrics, filename, param.dat_obs, param.dat_group, param.dat_skill, param.dat_skill_stacked, param.dat_skill_rcount, param.dat_skill_rix, hmms, param.cv_folds/*nhmms*/, dat_fold);
    free(dat_fold);
    
    *(tm_predict) += (clock_t)(clock()- tm0);//SEQ
    
    
    // delete problems
    NUMBER n_par = 0;
    for(f=0; f<param.cv_folds; f++) {
        n_par += hmms[f]->getNparams();
        delete hmms[f];
    }
    n_par /= param.cv_folds;
    
    free(folds);
    
    return (n_par);
}

NUMBER cross_validate_item(NUMBER* metrics, const char *filename, const char *model_file_name, clock_t *tm_fit, clock_t *tm_predict, FILE *fid_console) {//SEQ
    NPAR f;
    NCAT I; // item
    NDAT t;
    clock_t tm0;//SEQ
    char *ch;
    FILE *fid = NULL; // file for storing prediction should that be necessary
    FILE *fid_folds = NULL; // file for reading/writing folds
    if(param.predictions>0) {  // if we have to write the predictions file
        fid = fopen(filename,"w");
        if(fid == NULL)
        {
            fprintf(stderr, "Can't write output model file %s\n",filename);
            if(param.duplicate_console==1) fprintf(fid_console, "Can't write output model file %s\n",filename);
            exit(1);
        }
    }
    // produce folds
    NPAR *folds = Calloc(NPAR, (size_t)param.nI);
    NDAT *fold_counts = Calloc(NDAT, (size_t)param.cv_folds);
    srand ( (unsigned int)time(NULL) ); // randomize

    // folds file
    if(param.cv_folds_file[0] > 0) { // file is specified
        if(param.cv_inout_flag=='i') {
            fid_folds = fopen(param.cv_folds_file,"r");
            if(fid_folds == NULL)
            {
                fprintf(stderr, "Can't open folds file for reading %s\n",filename);
                if(param.duplicate_console==1) fprintf(fid_console, "Can't open folds file for reading %s\n",filename);
                exit(1);
            }
            max_line_length = 1024;
            line = (char *)malloc((size_t)max_line_length);
        } else if(param.cv_inout_flag=='o') {
            fid_folds = fopen(param.cv_folds_file,"w");
            if(fid_folds == NULL)
            {
                fprintf(stderr, "Can't open folds file for writing %s\n",filename);
                if(param.duplicate_console==1) fprintf(fid_console, "Can't open folds file for writing %s\n",filename);
                exit(1);
            }
        }
    }
    for(I=0; I<param.nI; I++) {
        if( param.cv_folds_file[0]==0 || (param.cv_folds_file[0] > 0 && param.cv_inout_flag=='o') ) { // output or default
            folds[I] = (NPAR)(rand() % param.cv_folds); // produce folds
            if(param.cv_folds_file[0] > 0 )
                fprintf(fid_folds,"%i\n",folds[I]); // write out
        } else if( (param.cv_folds_file[0] > 0 && param.cv_inout_flag=='i') ) {
            readline(fid_folds);//
            ch = strtok(line,"\t\n\r");
            if(ch == NULL) {
                fprintf(stderr, "Error reading input folds file (potentialy wrong number of rows)\n");
                if(param.duplicate_console==1) fprintf(fid_console, "Error reading input folds file (potentialy wrong number of rows)\n");
                exit(1);
            }
            folds[I] = (NPAR)(atoi(ch));
        }
    }
    if(param.cv_folds_file[0] > 0) { // file is specified
        fclose(fid_folds);
        if(param.cv_inout_flag=='i')
            free(line);
    }
    
    // count number of items in each fold
    for(t=0; t<param.N; t++) fold_counts[ folds[param.dat_item[t]] ]++;
    // create and fit multiple problems
    HMMProblem* hmms[param.cv_folds];
    int q = param.quiet;
    param.quiet = 1;
    for(f=0; f<param.cv_folds; f++) {
        switch(param.structure)
        {
            case STRUCTURE_SKILL: // Conjugate Gradient Descent
            case STRUCTURE_GROUP: // Conjugate Gradient Descent
                hmms[f] = new HMMProblem(&param);
                break;
        }
        // block respective data - do not fit the data belonging to the fold
        NPAR *saved_obs = Calloc(NPAR, (size_t)fold_counts[f]);
        NDAT count_saved = 0;
        for(t=0; t<param.N; t++) {
            if( folds[ param.dat_item[t] ] == f ) {
                saved_obs[count_saved++] = param.dat_obs[t];
                param.dat_obs[t] = -1;
            }
        }
        // now compute
        tm0 = clock(); //SEQ
        
        hmms[f]->fit();
        *(tm_fit) += (clock_t)(clock()- tm0);//SEQ
        
        // write model
        char fname[1024];
        sprintf(fname,"%s_%i",model_file_name,f);
        hmms[f]->toFile(fname);
        
        // UN-block respective data
        count_saved = 0;
        for(t=0; t<param.N; t++)
            if( folds[ param.dat_item[t]] == f )
                param.dat_obs[t]=saved_obs[count_saved++];
        free(saved_obs);
        if(q == 0) {
            printf("fold %d is done\n",f+1);
            if(param.duplicate_console==1) fprintf(fid_console, "fold %d is done\n",f+1);
        }
    }
    free(fold_counts);
    param.quiet = (NPAR)q;

    tm0 = clock();//SEQ
    
    // new prediction
    //        create a general fold-identifying array
    NPAR *dat_fold = Calloc(NPAR, param.N);
    for(NDAT t=0; t<param.N; t++) dat_fold[t] = folds[ param.dat_item[t] ];
    //        predict
    HMMProblem::predict(metrics, filename, param.dat_obs, param.dat_group, param.dat_skill, param.dat_skill_stacked, param.dat_skill_rcount, param.dat_skill_rix, hmms, param.cv_folds/*nhmms*/, dat_fold);
    free(dat_fold);
    
    *(tm_predict) += (clock_t)(clock()- tm0);//SEQ
    
    // delete problems
    NCAT n_par = 0;
    for(f=0; f<param.cv_folds; f++) {
        n_par += hmms[f]->getNparams();
        delete hmms[f];
    }
    n_par /= f;
    free(folds);

    return n_par;
}

NUMBER cross_validate_nstrat(NUMBER* metrics, const char *filename, const char *model_file_name, clock_t *tm_fit, clock_t *tm_predict, FILE *fid_console) {//SEQ
    NPAR f;
    NCAT U; // unstratified
    NDAT t;
    clock_t tm0;//SEQ
    char *ch;
    FILE *fid = NULL; // file for storing prediction should that be necessary
    FILE *fid_folds = NULL; // file for reading/writing folds
    if(param.predictions>0) {  // if we have to write the predictions file
        fid = fopen(filename,"w");
        if(fid == NULL)
        {
            fprintf(stderr, "Can't write output model file %s\n",filename);
            if(param.duplicate_console==1) fprintf(fid_console, "Can't write output model file %s\n",filename);
            exit(1);
        }
    }
    // produce folds
    NPAR *folds = Calloc(NPAR, (size_t)param.N);
    NDAT *fold_counts = Calloc(NDAT, (size_t)param.cv_folds);
    
    srand ( (unsigned int)time(NULL) ); // randomize
    
    // folds file
    if(param.cv_folds_file[0] > 0) { // file is specified
        if(param.cv_inout_flag=='i') {
            fid_folds = fopen(param.cv_folds_file,"r");
            if(fid_folds == NULL)
            {
                fprintf(stderr, "Can't open folds file for reading %s\n",filename);
                if(param.duplicate_console==1) fprintf(fid_console, "Can't open folds file for reading %s\n",filename);
                exit(1);
            }
            max_line_length = 1024;
            line = (char *)malloc((size_t)max_line_length);
        } else if(param.cv_inout_flag=='o') {
            fid_folds = fopen(param.cv_folds_file,"w");
            if(fid_folds == NULL)
            {
                fprintf(stderr, "Can't open folds file for writing %s\n",filename);
                if(param.duplicate_console==1) fprintf(fid_console, "Can't open folds file for writing %s\n",filename);
                exit(1);
            }
        }
    }
    for(U=0; U<param.N; U++) {
        if( param.cv_folds_file[0]==0 || (param.cv_folds_file[0] > 0 && param.cv_inout_flag=='o') ) { // output or default
            folds[U] = (NPAR)(rand() % param.cv_folds); // produce folds
            if(param.cv_folds_file[0] > 0 )
                fprintf(fid_folds,"%i\n",folds[U]); // write out
        } else if( (param.cv_folds_file[0] > 0 && param.cv_inout_flag=='i') ) {
            readline(fid_folds);//
            ch = strtok(line,"\t\n\r");
            if(ch == NULL) {
                fprintf(stderr, "Error reading input folds file (potentialy wrong number of rows)\n");
                if(param.duplicate_console==1) fprintf(fid_console, "Error reading input folds file (potentialy wrong number of rows)\n");
                exit(1);
            }
            folds[U] = (NPAR)(atoi(ch));
        }
    }
    if(param.cv_folds_file[0] > 0) { // file is specified
        fclose(fid_folds);
        if(param.cv_inout_flag=='i')
            free(line);
    }
    
    
    // count number of items in each fold
    for(t=0; t<param.N; t++)  fold_counts[ folds[param.dat_item[t]] ]++;
    // create and fit multiple problems
    HMMProblem* hmms[param.cv_folds];
    int q = param.quiet;
    param.quiet = 1;
    for(f=0; f<param.cv_folds; f++) {
        switch(param.structure)
        {
            case STRUCTURE_SKILL: // Conjugate Gradient Descent
            case STRUCTURE_GROUP: // Conjugate Gradient Descent
                hmms[f] = new HMMProblem(&param);
                break;
        }
        // block respective data - do not fit the data belonging to the fold
        NPAR *saved_obs = Calloc(NPAR, (size_t)fold_counts[f]);
        NDAT count_saved = 0;
        for(t=0; t<param.N; t++) {
            if( folds[ param.dat_item[t]] == f ) {
                saved_obs[count_saved++] = param.dat_obs[t];
                param.dat_obs[t]=-1;
            }
        }
        // now compute
        tm0 = clock(); //SEQ
        hmms[f]->fit();
        *(tm_fit) += (clock_t)(clock()- tm0);//SEQ
        
        // write model
        char fname[1024];
        sprintf(fname,"%s_%i",model_file_name,f);
        hmms[f]->toFile(fname);
        
        // UN-block respective data
        count_saved = 0;
        for(t=0; t<param.N; t++)
            if( folds[ param.dat_item[t]] == f )
                param.dat_obs[t]=saved_obs[count_saved++];
        free(saved_obs);
        if(q == 0) {
            printf("fold %d is done\n",f+1);
            if(param.duplicate_console==1) fprintf(fid_console, "fold %d is done\n",f+1);
        }
    }
    free(fold_counts);
    param.quiet = (NPAR)q;
    
    tm0 = clock();//SEQ
    
    // new prediction
    //        predict
    HMMProblem::predict(metrics, filename, param.dat_obs, param.dat_group, param.dat_skill, param.dat_skill_stacked, param.dat_skill_rcount, param.dat_skill_rix, hmms, param.cv_folds/*nhmms*/, folds);
    
    *(tm_predict) += (clock_t)(clock()- tm0);//SEQ
    
    // delete problems
    NCAT n_par = 0;
    for(f=0; f<param.cv_folds; f++) {
        n_par += hmms[f]->getNparams();
        delete hmms[f];
    }
    n_par /= f;
    free(folds);
    
    return n_par;
}