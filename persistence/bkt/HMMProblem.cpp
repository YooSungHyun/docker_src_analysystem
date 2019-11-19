#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include "utils.h"
#include "FitBit.h"
#include <math.h>
#include "HMMProblem.h"
#include <map>

HMMProblem::HMMProblem() {
}

HMMProblem::HMMProblem(struct param *param) {
    NPAR i;
    switch (param->structure) {
        case STRUCTURE_SKILL: // Expectation Maximization (Baum-Welch)
            for(i=0; i<3; i++) this->sizes[i] = param->nK;
            this->n_params = param->nK * 4;
            break;
        case STRUCTURE_GROUP: // Gradient Descent by group
            for(i=0; i<3; i++) this->sizes[i] = param->nG;
            this->n_params = param->nG * 4;
            break;
        default:
            fprintf(stderr,"Structure specified is not supported and should have been caught earlier\n");
            break;
    }
    init(param);
}

void HMMProblem::init(struct param *param) {
    this->p = param;
    this->non01constraints = true;
    this->null_obs_ratio = Calloc(NUMBER, (size_t)this->p->nO);
    this->neg_log_lik = 0;
    this->null_skill_obs = 0;
    this->null_skill_obs_prob = 0;
    if( this->p->solver == METHOD_CGD && this->p->solver_setting == -1)
        this->p->solver_setting = 1; // default Fletcher-Reeves
    
    NPAR nS = this->p->nS, nO = this->p->nO;
    NUMBER *a_PI, ** a_A, ** a_B;
    init3Params(a_PI, a_A, a_B, nS, nO);
    
    //
    // setup params
    //
    this->pi = init2D<NUMBER>((NDAT)this->sizes[0], (NDAT)nS);
    this->A =  init3D<NUMBER>((NDAT)this->sizes[1], (NDAT)nS, (NDAT)nS);
    this->B =  init3D<NUMBER>((NDAT)this->sizes[2], (NDAT)nS, (NDAT)nO);
    NPAR i, j;
    int offset, idx;

    if(param->initfile[0]==0) { // no setup file
        NUMBER sumPI = 0;
        NUMBER sumA[this->p->nS];
        NUMBER sumB[this->p->nS];
        for(i=0; i<this->p->nS; i++) {
            sumA[i] = 0;
            sumB[i] = 0;
        }
        // populate PI
        for(i=0; i<((nS)-1); i++) {
            a_PI[i] = this->p->init_params[i];
            sumPI  += this->p->init_params[i];
        }
        a_PI[nS-1] = 1 - sumPI;
        // populate A
        offset = (int)(nS-1);
        for(i=0; i<nS; i++) {
            for(j=0; j<((nS)-1); j++) {
                idx = (int)(offset + i*((nS)-1) + j);
                a_A[i][j] = this->p->init_params[idx];
                sumA[i]  += this->p->init_params[idx];
            }
            a_A[i][((nS)-1)]  = 1 - sumA[i];
        }
        // populate B
        offset = (int)((nS-1) + nS*(nS-1));
        for(i=0; i<nS; i++) {
            for(j=0; j<((nO)-1); j++) {
                idx = (int)(offset + i*((nO)-1) + j);
                a_B[i][j] = this->p->init_params[idx];
                sumB[i] += this->p->init_params[idx];
            }
            a_B[i][((nO)-1)]  = 1 - sumB[i];
        }
        
        // mass produce PI's, A's, B's
        if( this->p->do_not_check_constraints==0 && !checkPIABConstraints(a_PI, a_A, a_B)) {
            fprintf(stderr,"params do not meet constraints.\n");
            exit(1);
        }

        NCAT x;
        for(x=0; x<this->sizes[0]; x++)
            cpy1D<NUMBER>(a_PI, this->pi[x], (NDAT)nS);
        for(x=0; x<this->sizes[1]; x++)
            cpy2D<NUMBER>(a_A, this->A[x], (NDAT)nS, (NDAT)nS);
        for(x=0; x<this->sizes[2]; x++)
            cpy2D<NUMBER>(a_B, this->B[x], (NDAT)nS, (NDAT)nO);

        // destroy setup params
        free(a_PI);
        free2D<NUMBER>(a_A, (NDAT)nS);
        free2D<NUMBER>(a_B, (NDAT)nS);
    } else {
        // if needs be -- read in init params from a file
        this->readModel(param->initfile, false /* read and upload but not overwrite*/);
    }
    
    // populate boundaries
    // populate lb*/ub*
    // *PI
    init3Params(this->lbPI, this->lbA, this->lbB, nS, nO);
    init3Params(this->ubPI, this->ubA, this->ubB, nS, nO);
    for(i=0; i<nS; i++) {
        lbPI[i] = this->p->param_lo[i];
        ubPI[i] = this->p->param_hi[i];
    }
    // *A
    offset = nS;
    for(i=0; i<nS; i++)
        for(j=0; j<nS; j++) {
            idx = (int)(offset + i*nS + j);
            lbA[i][j] = this->p->param_lo[idx];
            ubA[i][j] = this->p->param_hi[idx];
        }
    // *B
    offset = (int)(nS + nS*nS);
    for(i=0; i<nS; i++)
        for(j=0; j<nO; j++) {
            idx = (int)(offset + i*nO + j);
            lbB[i][j] = this->p->param_lo[idx];
            ubB[i][j] = this->p->param_hi[idx];
        }
}

HMMProblem::~HMMProblem() {
    destroy();
}

void HMMProblem::destroy() {
    // destroy model data
    if(this->null_obs_ratio != NULL) free(this->null_obs_ratio);
    if(this->pi != NULL) free2D<NUMBER>(this->pi, this->sizes[0]);
    if(this->A  != NULL) free3D<NUMBER>(this->A,  this->sizes[1], this->p->nS);
    if(this->B  != NULL) free3D<NUMBER>(this->B,  this->sizes[2], this->p->nS);
    if(this->lbPI!=NULL) free(this->lbPI);
    if(this->ubPI!=NULL) free(this->ubPI);
    if(this->lbA!=NULL) free2D<NUMBER>(this->lbA, this->p->nS);
    if(this->ubA!=NULL) free2D<NUMBER>(this->ubA, this->p->nS);
    if(this->lbB!=NULL) free2D<NUMBER>(this->lbB, this->p->nS);
    if(this->ubB!=NULL) free2D<NUMBER>(this->ubB, this->p->nS);
}// ~HMMProblem

bool HMMProblem::hasNon01Constraints() {
    return this->non01constraints;
}

NUMBER** HMMProblem::getPI() {
    return this->pi;
}

NUMBER*** HMMProblem::getA() {
    return this->A;
}

NUMBER*** HMMProblem::getB() {
    return this->B;
}

NUMBER* HMMProblem::getPI(NCAT x) {
    if( x > (this->sizes[0]-1) ) {
        fprintf(stderr,"While accessing PI, skill index %d exceeded last index of the data %d.\n", x, this->sizes[0]-1);
        exit(1);
    }
    return this->pi[x];
}

NUMBER** HMMProblem::getA(NCAT x) {
    if( x > (this->sizes[1]-1) ) {
        fprintf(stderr,"While accessing A, skill index %d exceeded last index of the data %d.\n", x, this->sizes[1]-1);
        exit(1);
    }
    return this->A[x];
}

NUMBER** HMMProblem::getB(NCAT x) {
    if( x > (this->sizes[2]-1) ) {
        fprintf(stderr,"While accessing B, skill index %d exceeded last index of the data %d.\n", x, this->sizes[2]-1);
        exit(1);
    }
    return this->B[x];
}

NUMBER* HMMProblem::getLbPI() {
    if( !this->non01constraints ) return NULL;
    return this->lbPI;
}

NUMBER** HMMProblem::getLbA() {
    if( !this->non01constraints ) return NULL;
    return this->lbA;
}

NUMBER** HMMProblem::getLbB() {
    if( !this->non01constraints ) return NULL;
    return this->lbB;
}

NUMBER* HMMProblem::getUbPI() {
    if( !this->non01constraints ) return NULL;
    return this->ubPI;
}

NUMBER** HMMProblem::getUbA() {
    if( !this->non01constraints ) return NULL;
    return this->ubA;
}

NUMBER** HMMProblem::getUbB() {
    if( !this->non01constraints ) return NULL;
    return this->ubB;
}

// getters for computing alpha, beta, gamma
NUMBER HMMProblem::getPI(struct data* dt, NPAR i) {
    switch(this->p->structure)
    {
        case STRUCTURE_SKILL:
            return this->pi[dt->k][i];
            break;
        case STRUCTURE_GROUP:
            return this->pi[dt->g][i];
            break;
        default:
            fprintf(stderr,"Solver specified is not supported.\n");
            exit(1);
            break;
    }
}

// getters for computing alpha, beta, gamma
NUMBER HMMProblem::getA (struct data* dt, NPAR i, NPAR j) {
    switch(this->p->structure)
    {
        case STRUCTURE_SKILL:
            return this->A[dt->k][i][j];
            break;
        case STRUCTURE_GROUP:
            return this->A[dt->g][i][j];
            break;
        default:
            fprintf(stderr,"Solver specified is not supported.\n");
            exit(1);
            break;
    }
}

// getters for computing alpha, beta, gamma
NUMBER HMMProblem::getB (struct data* dt, NPAR i, NPAR m) {
    // special attention for "unknonw" observations, i.e. the observation was there but we do not know what it is
    // in this case we simply return 1, effectively resulting in no change in \alpha or \beta vatiables
    if(m<0)
        return 1;
    switch(this->p->structure)
    {
        case STRUCTURE_SKILL:
            return this->B[dt->k][i][m];
            break;
        case STRUCTURE_GROUP:
            return this->B[dt->g][i][m];
            break;
        default:
            fprintf(stderr,"Solver specified is not supported.\n");
            exit(1);
            break;
    }
}

bool HMMProblem::checkPIABConstraints(NUMBER* a_PI, NUMBER** a_A, NUMBER** a_B) {
    NPAR i, j;
    // check values
    NUMBER sum_pi = 0.0;
    NUMBER sum_a_row[this->p->nS];
    NUMBER sum_b_row[this->p->nS];
    for(i=0; i<this->p->nS; i++) {
        sum_a_row[i] = 0.0;
        sum_b_row[i] = 0.0;
    }
    
    for(i=0; i<this->p->nS; i++) {
        if( a_PI[i]>1.0 || a_PI[i]<0.0)
            return false;
        sum_pi += a_PI[i];
        for(j=0; j<this->p->nS; j++) {
            if( a_A[i][j]>1.0 || a_A[i][j]<0.0)
                return false;
            sum_a_row[i] += a_A[i][j];
        }// all states 2
        for(int m=0; m<this->p->nO; m++) {
            if( a_B[i][m]>1.0 || a_B[i][m]<0.0)
                return false;
            sum_b_row[i] += a_B[i][m];
        }// all observations
    }// all states
    if(sum_pi!=1.0)
        return false;
    for(i=0; i<this->p->nS; i++)
        if( sum_a_row[i]!=1.0 || sum_b_row[i]!=1.0)
            return false;
    return true;
}

NUMBER HMMProblem::getSumLogPOPara(NCAT xndat, struct data** x_data) {
    NUMBER result = 0.0;
    for(NCAT x=0; x<xndat; x++) result += (x_data[x]->cnt==0)?x_data[x]->loglik:0;
    return result;
}

void HMMProblem::initAlpha(NCAT xndat, struct data** x_data) {
    NPAR nS = this->p->nS;
//    #pragma omp parallel for schedule(dynamic) if(parallel_now) //PAR
    for(NCAT x=0; x<xndat; x++) {
        NDAT t;
        NPAR i;
        if( x_data[x]->cnt!=0 ) continue; // ... and the thing has not been computed yet (e.g. from group to skill)
        // alpha
        if( x_data[x]->alpha == NULL ) {
            x_data[x]->alpha = Calloc(NUMBER*, (size_t)x_data[x]->n);
            for(t=0; t<x_data[x]->n; t++)
                x_data[x]->alpha[t] = Calloc(NUMBER, (size_t)nS);
            
        } else {
            for(t=0; t<x_data[x]->n; t++)
                for(i=0; i<nS; i++)
                    x_data[x]->alpha[t][i] = 0.0;
        }
        // p_O_param
        x_data[x]->p_O_param = 0.0;
        x_data[x]->loglik = 0.0;
        // c - scaling
        if( x_data[x]->c == NULL ) {
            x_data[x]->c = Calloc(NUMBER, (size_t)x_data[x]->n);
        } else {
            for(t=0; t<x_data[x]->n; t++)
                x_data[x]->c[t] = 0.0;
        }
    } // for all groups in skill
}

void HMMProblem::initXiGamma(NCAT xndat, struct data** x_data) {
    NPAR nS = this->p->nS;
//    #pragma omp parallel for schedule(dynamic) if(parallel_now) //PAR
    for(NCAT x=0; x<xndat; x++) {
        NDAT t;
        NPAR i, j;
        if( x_data[x]->cnt!=0 ) continue; // ... and the thing has not been computed yet (e.g. from group to skill)
        // Xi
        if( x_data[x]->gamma == NULL ) {
            x_data[x]->gamma = Calloc(NUMBER*, (size_t)x_data[x]->n);
            for(t=0; t<x_data[x]->n; t++)
                x_data[x]->gamma[t] = Calloc(NUMBER, (size_t)nS);
            
        } else {
            for(t=0; t<x_data[x]->n; t++)
                for(i=0; i<nS; i++)
                    x_data[x]->gamma[t][i] = 0.0;
        }
        // Gamma
        if( x_data[x]->xi == NULL ) {
            x_data[x]->xi = Calloc(NUMBER**, (size_t)x_data[x]->n);
            for(t=0; t<x_data[x]->n; t++) {
                x_data[x]->xi[t] = Calloc(NUMBER*, (size_t)nS);
                for(i=0; i<nS; i++)
                    x_data[x]->xi[t][i] = Calloc(NUMBER, (size_t)nS);
            }
            
        } else {
            for(t=0; t<x_data[x]->n; t++)
                for(i=0; i<nS; i++)
                    for(j=0; j<nS; j++)
                        x_data[x]->xi[t][i][j] = 0.0;
        }
    } // for all groups in skill
}

void HMMProblem::initBeta(NCAT xndat, struct data** x_data) {
    NPAR nS = this->p->nS;
//    #pragma omp parallel for schedule(dynamic) if(parallel_now) //PAR
    for(NCAT x=0; x<xndat; x++) {
        NDAT t;
        NPAR i;
        if( x_data[x]->cnt!=0 ) continue; // ... and the thing has not been computed yet (e.g. from group to skill)
        // beta
        if( x_data[x]->beta == NULL ) {
            x_data[x]->beta = Calloc(NUMBER*, (size_t)x_data[x]->n);
            for(t=0; t<x_data[x]->n; t++)
                x_data[x]->beta[t] = Calloc(NUMBER, (size_t)nS);
            
        } else {
            for(t=0; t<x_data[x]->n; t++)
                for(i=0; i<nS; i++)
                    x_data[x]->beta[t][i] = 0.0;
        }
    } // for all groups in skill
} // initBeta

NDAT HMMProblem::computeAlphaAndPOParam(NCAT xndat, struct data** x_data) {
    initAlpha(xndat, x_data);
    NPAR nS = this->p->nS;
    NDAT  ndat = 0;
//    #pragma omp parallel for schedule(dynamic) if(parallel_now) reduction(+:ndat) //PAR
    for(NCAT x=0; x<xndat; x++) {
        NDAT t;
        NPAR i, j, o;
        if( x_data[x]->cnt!=0 ) continue; // ... and the thing has not been computed yet (e.g. from group to skill)
        ndat += x_data[x]->n; // reduction'ed
        for(t=0; t<x_data[x]->n; t++) {
            o = this->p->dat_obs[ x_data[x]->ix[t] ];//->get( x_data[x]->ix[t] );

            if(t==0) { // it's alpha(1,i)
                // compute \alpha_1(i) = \pi_i b_i(o_1)
                for(i=0; i<nS; i++) {
                    x_data[x]->alpha[t][i] = getPI(x_data[x],i) * ((o<0)?1:getB(x_data[x],i,o)); // if observatiob unknown use 1
                    if(this->p->scaled==1) x_data[x]->c[t] += x_data[x]->alpha[t][i];
                }
            } else { // it's alpha(t,i)
                // compute \alpha_{t}(i) = b_j(o_{t})\sum_{j=1}^N{\alpha_{t-1}(j) a_{ji}}
                for(i=0; i<nS; i++) {
                    for(j=0; j<nS; j++) {
                        x_data[x]->alpha[t][i] += x_data[x]->alpha[t-1][j] * getA(x_data[x],j,i);
                    }
                    x_data[x]->alpha[t][i] *= ((o<0)?1:getB(x_data[x],i,o)); // if observatiob unknown use 1
                    if(this->p->scaled==1) x_data[x]->c[t] += x_data[x]->alpha[t][i];
                }
            }
            // scale \alpha_{t}(i) - same for t=1 or otherwise
            if(this->p->scaled==1) {
                x_data[x]->c[t] = 1/x_data[x]->c[t];//safe0num();
                for(i=0; i<nS; i++) x_data[x]->alpha[t][i] *= x_data[x]->c[t];
            }
            
            if(this->p->scaled==1)  x_data[x]->loglik += log(x_data[x]->c[t]);
        } // for all observations within skill-group
        if(this->p->scaled==1)  x_data[x]->p_O_param = exp( -x_data[x]->loglik );
        else {
            x_data[x]->p_O_param = 0; // 0 for non-scaled
            for(i=0; i<nS; i++) x_data[x]->p_O_param += x_data[x]->alpha[x_data[x]->n-1][i];
            x_data[x]->loglik = -safelog(x_data[x]->p_O_param);
        }
    } // for all groups in skill
    return ndat;
}

void HMMProblem::computeBeta(NCAT xndat, struct data** x_data) {
    initBeta(xndat, x_data);
    NPAR nS = this->p->nS;
//    #pragma omp parallel for schedule(dynamic) if(parallel_now) //PAR
    for(NCAT x=0; x<xndat; x++) {
        int t;
        NPAR i, j, o;
        if( x_data[x]->cnt!=0 ) continue; // ... and the thing has not been computed yet (e.g. from group to skill)
        for(t=(NDAT)(x_data[x]->n)-1; t>=0; t--) {
            if( t==(x_data[x]->n-1) ) { // last \beta
                // \beta_T(i) = 1
                for(i=0; i<nS; i++)
                    x_data[x]->beta[t][i] = (this->p->scaled==1)?x_data[x]->c[t]:1.0;;
            } else {
                // \beta_t(i) = \sum_{j=1}^N{beta_{t+1}(j) a_{ij} b_j(o_{t+1})}
                o = this->p->dat_obs[ x_data[x]->ix[t+1] ];
                for(i=0; i<nS; i++) {
                    for(j=0; j<nS; j++)
                        x_data[x]->beta[t][i] += x_data[x]->beta[t+1][j] * getA(x_data[x],i,j) * ((o<0)?1:getB(x_data[x],j,o)); // if observatiob unknown use 1
                    // scale
                    if(this->p->scaled==1) x_data[x]->beta[t][i] *= x_data[x]->c[t];
                }
            }
        } // for all observations, starting with last one
    } // for all groups within skill
}

void HMMProblem::computeXiGamma(NCAT xndat, struct data** x_data){
    HMMProblem::initXiGamma(xndat, x_data);
    NPAR nS = this->p->nS;
//    #pragma omp parallel for schedule(dynamic) if(parallel_now) //PAR
    for(NCAT x=0; x<xndat; x++) {
        NDAT t;
        NPAR i, j, o_tp1;
        NUMBER denom;
        if( x_data[x]->cnt!=0 ) continue; // ... and the thing has not been computed yet (e.g. from group to skill)
        
        for(t=0; t<(x_data[x]->n-1); t++) { // -1 is important
            o_tp1 = this->p->dat_obs[ x_data[x]->ix[t+1] ];
            
            denom = 0.0;
            for(i=0; i<nS; i++) {
                for(j=0; j<nS; j++) {
                    denom += x_data[x]->alpha[t][i] * getA(x_data[x],i,j) * x_data[x]->beta[t+1][j] * ((o_tp1<0)?1:getB(x_data[x],j,o_tp1));
                }
            }
            for(i=0; i<nS; i++) {
                for(j=0; j<nS; j++) {
                    x_data[x]->xi[t][i][j] = x_data[x]->alpha[t][i] * getA(x_data[x],i,j) * x_data[x]->beta[t+1][j] * ((o_tp1<0)?1:getB(x_data[x],j,o_tp1)) / ((denom>0)?denom:1); //
                    x_data[x]->gamma[t][i] += x_data[x]->xi[t][i][j];
                }
            }
        
        } // for all observations within skill-group
    } // for all groups in skill
}

void HMMProblem::setGradPI(FitBit *fb){
    if(this->p->block_fitting[0]>0) return;
    NDAT t = 0, ndat = 0;
    NPAR i, o;
    struct data* dt;
    for(NCAT x=0; x<fb->xndat; x++) {
        dt = fb->x_data[x];
        if( dt->cnt!=0 ) continue;
        ndat += dt->n;
        o = this->p->dat_obs[ dt->ix[t] ];
        for(i=0; i<this->p->nS; i++) {
            fb->gradPI[i] -= dt->beta[t][i] * ((o<0)?1:getB(dt,i,o)) / safe0num(dt->p_O_param);
        }
    }
    if( this->p->Cslices>0 ) // penalty
        fb->addL2Penalty(FBV_PI, this->p, (NUMBER)ndat);
}

void HMMProblem::setGradA (FitBit *fb){
    if(this->p->block_fitting[1]>0) return;
    NDAT t, ndat = 0;
    NPAR o, i, j;
    struct data* dt;
    for(NCAT x=0; x<fb->xndat; x++) {
        dt = fb->x_data[x];
        if( dt->cnt!=0 ) continue;
        ndat += dt->n;
        for(t=1; t<dt->n; t++) {
            o = this->p->dat_obs[ dt->ix[t] ];
            for(i=0; i<this->p->nS; i++)
                for(j=0; j<this->p->nS; j++)
                    fb->gradA[i][j] -= dt->beta[t][j] * ((o<0)?1:getB(dt,j,o)) * dt->alpha[t-1][i] / safe0num(dt->p_O_param);
        }
    }
    if( this->p->Cslices>0 ) // penalty
        fb->addL2Penalty(FBV_A, this->p, (NUMBER)ndat);
}

void HMMProblem::setGradB (FitBit *fb){
    if(this->p->block_fitting[2]>0) return;
    NDAT t, ndat = 0;
    NPAR o, o0, i, j;
    struct data* dt;
    for(NCAT x=0; x<fb->xndat; x++) {
        dt = fb->x_data[x];
        if( dt->cnt!=0 ) continue;
        ndat += dt->n;
        for(t=0; t<dt->n; t++) { // Levinson MMFST
            o  = this->p->dat_obs[ dt->ix[t] ];
            o0 = this->p->dat_obs[ dt->ix[0] ];
            if(o<0) // if no observation -- skip
                continue;
            for(j=0; j<this->p->nS; j++)
                if(t==0) {
                    fb->gradB[j][o] -= (o0==o) * getPI(dt,j) * dt->beta[0][j];
                } else {
                    for(i=0; i<this->p->nS; i++)
                        fb->gradB[j][o] -= ( dt->alpha[t-1][i] * getA(dt,i,j) * dt->beta[t][j]) / safe0num(dt->p_O_param); // Levinson MMFST
                }
        }
    }
    if( this->p->Cslices>0 ) // penalty
        fb->addL2Penalty(FBV_B, this->p, (NUMBER)ndat);
}

NDAT HMMProblem::computeGradients(FitBit *fb){
    fb->toZero(FBS_GRAD);
    
    NDAT ndat = computeAlphaAndPOParam(fb->xndat, fb->x_data);
    computeBeta(fb->xndat, fb->x_data);

    if(fb->pi != NULL && this->p->block_fitting[0]==0) setGradPI(fb);
    if(fb->A  != NULL && this->p->block_fitting[1]==0) setGradA(fb);
    if(fb->B  != NULL && this->p->block_fitting[2]==0) setGradB(fb);
    return ndat;
} // computeGradients()

void HMMProblem::toFile(const char *filename) {
    switch(this->p->structure)
    {
        case STRUCTURE_SKILL:
            toFileSkill(filename);
            break;
        case STRUCTURE_GROUP:
            toFileGroup(filename);
            break;
        default:
            fprintf(stderr,"Solver specified is not supported.\n");
            break;
    }
}

void HMMProblem::toFileSkill(const char *filename) {
    FILE *fid = fopen(filename,"w");
    if(fid == NULL) {
        fprintf(stderr,"Can't write output model file %s\n",filename);
        exit(1);
    }
    // write solved id
    writeSolverInfo(fid, this->p);
    
    fprintf(fid,"Null skill ratios\t");
    for(NPAR m=0; m<this->p->nO; m++)
        fprintf(fid," %10.7f%s",this->null_obs_ratio[m],(m==(this->p->nO-1))?"\n":"\t");
    
    NCAT k;
    std::map<NCAT,std::string>::iterator it;
    for(k=0;k<this->p->nK;k++) {
        it = this->p->map_skill_bwd->find(k);
        fprintf(fid,"%d\t%s\n",k,it->second.c_str());
        NPAR i,j,m;
        fprintf(fid,"PI\t");
        for(i=0; i<this->p->nS; i++)
            fprintf(fid,"%12.10f%s",this->pi[k][i],(i==(this->p->nS-1))?"\n":"\t");
        fprintf(fid,"A\t");
        for(i=0; i<this->p->nS; i++)
            for(j=0; j<this->p->nS; j++)
                fprintf(fid,"%12.10f%s",this->A[k][i][j],(i==(this->p->nS-1) && j==(this->p->nS-1))?"\n":"\t");
        fprintf(fid,"B\t");
        for(i=0; i<this->p->nS; i++)
            for(m=0; m<this->p->nO; m++)
                fprintf(fid,"%12.10f%s",this->B[k][i][m],(i==(this->p->nS-1) && m==(this->p->nO-1))?"\n":"\t");
    }
    fclose(fid);
}

void HMMProblem::toFileGroup(const char *filename) {
    FILE *fid = fopen(filename,"w");
    if(fid == NULL) {
        fprintf(stderr,"Can't write output model file %s\n",filename);
        exit(1);
    }
    
    // write solved id
    writeSolverInfo(fid, this->p);
    
    fprintf(fid,"Null skill ratios\t");
    for(NPAR m=0; m<this->p->nO; m++)
        fprintf(fid," %10.7f%s",this->null_obs_ratio[m],(m==(this->p->nO-1))?"\n":"\t");
    NCAT g;
    std::map<NCAT,std::string>::iterator it;
    for(g=0;g<this->p->nG;g++) {
        it = this->p->map_group_bwd->find(g);
        fprintf(fid,"%d\t%s\n",g,it->second.c_str());
        NPAR i,j,m;
        fprintf(fid,"PI\t");
        for(i=0; i<this->p->nS; i++)
            fprintf(fid,"%12.10f%s",this->pi[g][i],(i==(this->p->nS-1))?"\n":"\t");
        fprintf(fid,"A\t");
        for(i=0; i<this->p->nS; i++)
            for(j=0; j<this->p->nS; j++)
                fprintf(fid,"%12.10f%s",this->A[g][i][j],(i==(this->p->nS-1) && j==(this->p->nS-1))?"\n":"\t");
        fprintf(fid,"B\t");
        for(i=0; i<this->p->nS; i++)
            for(m=0; m<this->p->nO; m++)
                fprintf(fid,"%12.10f%s",this->B[g][i][m],(i==(this->p->nS-1) && m==(this->p->nO-1))?"\n":"\t");
    }
    fclose(fid);
}

void HMMProblem::producePCorrect(NUMBER*** group_skill_map, NUMBER* local_pred, NCAT* ks, NCAT nks, struct data* dt) {
    NPAR m, i;
    NCAT k;
    NUMBER *local_pred_inner = init1D<NUMBER>(this->p->nO);
    for(m=0; m<this->p->nO; m++) local_pred[m] = 0.0;
    for(int l=0; l<nks; l++) {
        for(m=0; m<this->p->nO; m++) local_pred_inner[m] = 0.0;
        k = ks[l];
        dt->k = k;
        for(m=0; m<this->p->nO; m++)
            for(i=0; i<this->p->nS; i++)
                local_pred_inner[m] += group_skill_map[dt->g][k][i] * getB(dt,i,m);
        for(m=0; m<this->p->nO; m++)
            local_pred[m] += local_pred_inner[m];
    }
    if(nks>1) {
        for(m=0; m<this->p->nO; m++)
            local_pred[m] /= nks;
    }
    free(local_pred_inner);
}

void HMMProblem::predict(NUMBER* metrics, const char *filename, NPAR* dat_obs, NCAT *dat_group, NCAT *dat_skill, NCAT *dat_skill_stacked, NCAT *dat_skill_rcount, NDAT *dat_skill_rix, HMMProblem **hmms, NPAR nhmms, NPAR *hmm_idx) {
    NDAT t;
    std::string currentKU;
    std::map<NCAT, std::string>::iterator it;
    NCAT g, k;
    NPAR i, j, m, o, isTarget = 0;
    NPAR nS = hmms[0]->p->nS, nO = hmms[0]->p->nO;
    NCAT nK = hmms[0]->p->nK, nG = hmms[0]->p->nG;
    NDAT N  = hmms[0]->p->N;
    NDAT N_null  = hmms[0]->p->N_null;
    char f_multiskill = hmms[0]->p->multiskill;
    char f_update_known = hmms[0]->p->update_known;
    // char f_update_unknown = hmms[0]->p->update_unknown;
    int f_predictions = hmms[0]->p->predictions;
    // char f_metrics_target_obs = hmms[0]->p->metrics_target_obs;
    int f_metrics_target_obs = hmms[0]->p->metrics_target_obs;
    // for(i=1; i<nhmms; i++) {
    //     if( nS != hmms[i]->p->nS || nO != hmms[i]->p->nO || nK != hmms[i]->p->nK ||
    //        nG != hmms[i]->p->nG || hmms[i]->p->N != hmms[i]->p->N || hmms[i]->p->N_null != hmms[i]->p->N_null ||
    //        f_multiskill != hmms[i]->p->multiskill ||
    //        f_update_known != hmms[i]->p->update_known ||
    //        f_update_unknown != hmms[i]->p->update_unknown ||
    //        f_predictions != hmms[i]->p->predictions ||
    //        f_metrics_target_obs != hmms[i]->p->metrics_target_obs) {
    //         fprintf(stderr,"Error! One of count variables (N, N_null, nS, nO, nK, nG) or flags (multiskill, predictions, metrics_target_obs, update_known, update_unknown) does not have the same value across multiple models\n");
    //         exit(1);
    //     }
    // }
    
    NUMBER *local_pred = init1D<NUMBER>(nO); // local prediction
    NUMBER *pLe = init1D<NUMBER>(nS);// p(L|evidence);
    NUMBER pLe_denom; // p(L|evidence) denominator
    NUMBER ***group_skill_map = init3D<NUMBER>(nG, nK, nS);
    
    NUMBER ll = 0.0, ll_no_null = 0.0, rmse = 0.0, rmse_no_null = 0.0, accuracy = 0.0, accuracy_no_null = 0.0;
    NUMBER p;
    
    
    FILE *fid = NULL; // file for storing prediction should that be necessary
    if(f_predictions>0) {
        fid = fopen(filename,"w");
        if(fid == NULL)
        {
            fprintf(stderr,"Can't write output model file %s\n",filename);
            exit(1);
        }
    }
    
    // initialize
    struct data* dt = new data;
    NDAT count = 0;
//    NDAT d = 0;
    HMMProblem *hmm;
    
    for(t=0; t<N; t++) {
        
//        output_this = true;
        o = dat_obs[t];
        g = dat_group[t];
        dt->g = g;
        
        hmm = (nhmms==1)?hmms[0]:hmms[hmm_idx[t]]; // if just one hmm, use 0's, otherwise take the index value
        
        isTarget = hmm->p->metrics_target_obs == o;
        NCAT *ar;
        int n;
        if(f_multiskill==0) {
            k = dat_skill[t];
            ar = &k;
            n = 1;
        } else {
            k = dat_skill_stacked[ dat_skill_rix[t] ];
            ar = &dat_skill_stacked[ dat_skill_rix[t] ];
            n = dat_skill_rcount[t];
        }
        
        // deal with null skill
        if(ar[0]<0) { // if no skill label
            isTarget = hmm->null_skill_obs==o;
            rmse += pow(isTarget - hmm->null_obs_ratio[f_metrics_target_obs],2);
            accuracy += isTarget == (hmm->null_obs_ratio[f_metrics_target_obs]==maxn(hmm->null_obs_ratio,nO) && hmm->null_obs_ratio[f_metrics_target_obs] > 1/nO);
            ll -= isTarget*safelog(hmm->null_skill_obs_prob) + (1-isTarget)*safelog(1 - hmm->null_skill_obs_prob);
            for(m=0; m<nO; m++) {
                fprintf(fid,"%12.10f%s",hmm->null_obs_ratio[m],(m<(nO-1))?"\t":"\n");//PRINTNOW
            }
            continue;
        }
        // check if {g,k}'s were initialized
        for(int l=0; l<n; l++) {
            k = ar[l];

            
            it = hmms[0]->p->map_skill_bwd->find(k);
            currentKU = std::string(it->second.c_str());
            hmms[0]->PLs.insert(std::pair<std::string, float>(currentKU, 0.0));

            if( group_skill_map[g][k][0]==0)
            {
                dt->k = k;
                
                for(i=0; i<nS; i++) {
                    group_skill_map[g][k][i] = hmm->getPI(dt,i);
                    count++;
                }
            }// pLo/pL not set
        }// for all skills at this transaction
        
        // produce prediction and copy to result
        hmm->producePCorrect(group_skill_map, local_pred, ar, n, dt); 
        projectsimplex(local_pred, nO); // addition to make sure there's not side effects
        
        // if necessary guess the obsevaion using Pi and B
        if(f_update_known=='g') {
            NUMBER max_local_pred=0;
            NPAR ix_local_pred=0;
            for(m=0; m<nO; m++) {
                if( local_pred[m]>max_local_pred ) {
                    max_local_pred = local_pred[m];
                    ix_local_pred = m;
                }
            }
            o = ix_local_pred;
        }
        
        // update pL
        for(int l=0; l<n; l++) {
            k = ar[l];
            dt->k = k;
            
            if(o>-1) { // known observations //
                // update p(L)
                pLe_denom = 0.0;
                // 1. pLe =  (L .* B(:,o)) ./ ( L'*B(:,o)+1e-8 );
                for(i=0; i<nS; i++)
                    pLe_denom += group_skill_map[g][k][i] * hmm->getB(dt,i,o);  // TODO: this is local_pred[o]!!!
                for(i=0; i<nS; i++)
                    pLe[i] = group_skill_map[g][k][i] * hmm->getB(dt,i,o) / safe0num(pLe_denom); 
                // 2. L = (pLe'*A)';
                for(i=0; i<nS; i++)
                    group_skill_map[g][k][i]= 0.0; 
                for(j=0; j<nS; j++)
                    for(j=0; j<nS; j++)
                        for(i=0; i<nS; i++)
                            group_skill_map[g][k][j] += pLe[i] * hmm->getA(dt,i,j);//A[i][j]; 
            } else { // unknown observation
                // 2. L = (pL'*A)';
                for(i=0; i<nS; i++)
                    pLe[i] = group_skill_map[g][k][i]; // copy first; 
                for(i=0; i<nS; i++)
                    group_skill_map[g][k][i] = 0.0; // erase old value 
                for(j=0; j<nS; j++)
                    for(i=0; i<nS; i++)
                        group_skill_map[g][k][j] += pLe[i] * hmm->getA(dt,i,j);
            }// observations
            projectsimplex(group_skill_map[g][k], nS); // addition to make sure there's not side effects 
        }
        
        // write prediction out (after update)
        // write prediction out (before pKnown update)
        if(f_predictions>0 /*&& output_this*/) { // write predictions file if it was opened
            for(m=0; m<nO; m++) {
                fprintf(fid,"%12.10f%s",local_pred[m],(m<(nO-1))?"\t": ((f_predictions==1)?"\n":"\t") );// if we print states of KCs, continut
                if(local_pred[m] > hmms[0]->PLs.at(currentKU))
                    hmms[0]->PLs.at(currentKU) = (float) local_pred[m];
            }
            
            if(f_predictions==2) { // if we print out states of KC's as welll
                for(int l=0; l<n; l++) { // all KC here
                    fprintf(fid,"%12.10f%s",group_skill_map[g][ ar[l] ][0], (l==(n-1) && l==(n-1))?"\n":"\t"); // nnon boost // if end of all states: end line//UNBOOST
                }
            }
        }
        
        // printf("target %d, hmm->p->metrics_target_obs %d, o %d, local_pred[f_metrics_target_obs] %d, maxn(local_pred,nO) %d\n", 
        // isTarget, hmm->p->metrics_target_obs, o, local_pred[f_metrics_target_obs], maxn(local_pred,nO));

        rmse += pow(isTarget-local_pred[f_metrics_target_obs],2);
        rmse_no_null += pow(isTarget-local_pred[f_metrics_target_obs],2);
        accuracy += isTarget == (local_pred[f_metrics_target_obs]==maxn(local_pred,nO) && local_pred[f_metrics_target_obs]>1/nO);
        accuracy_no_null += isTarget == (local_pred[f_metrics_target_obs]==maxn(local_pred,nO) && local_pred[f_metrics_target_obs]>1/nO);
        p = safe01num(local_pred[f_metrics_target_obs]);
        ll -= safelog(  p)*   isTarget  +  safelog(1-p)*(1-isTarget);
        ll_no_null -= safelog(  p)*   isTarget  +  safelog(1-p)*(1-isTarget);
    } // for all data
    
    delete(dt);
    free(local_pred);
    free(pLe);
    free3D<NUMBER>(group_skill_map, nG, nK);

    rmse = sqrt(rmse / N);
    rmse_no_null = sqrt(rmse_no_null / (N - N_null));
    if(metrics != NULL) {
        metrics[0] = ll;
        metrics[1] = ll_no_null;
        metrics[2] = rmse;
        metrics[3] = rmse_no_null;
        metrics[4] = accuracy/N;
        metrics[5] = accuracy_no_null/(N-N_null);
    }

    if(f_predictions>0) // close predictions file if it was opened
        fclose(fid);
}


void HMMProblem::predict_nofile(NUMBER* metrics, const char *filename, NPAR* dat_obs, NCAT *dat_group, NCAT *dat_skill, NCAT *dat_skill_stacked, NCAT *dat_skill_rcount, NDAT *dat_skill_rix, HMMProblem **hmms, NPAR nhmms, NPAR *hmm_idx) {
    NDAT t;
    std::string currentKU;
    std::map<NCAT, std::string>::iterator it;
    NCAT g, k;
    NPAR i, j, m, o, isTarget = 0;
    NPAR nS = hmms[0]->p->nS, nO = hmms[0]->p->nO;
    NCAT nK = hmms[0]->p->nK, nG = hmms[0]->p->nG;
    NDAT N  = hmms[0]->p->N;
    NDAT N_null  = hmms[0]->p->N_null;
    char f_multiskill = hmms[0]->p->multiskill;
    char f_update_known = hmms[0]->p->update_known;
    char f_update_unknown = hmms[0]->p->update_unknown;
    int f_predictions = hmms[0]->p->predictions;
    int f_metrics_target_obs = hmms[0]->p->metrics_target_obs;
    for(i=1; i<nhmms; i++) {
        if( nS != hmms[i]->p->nS || nO != hmms[i]->p->nO || nK != hmms[i]->p->nK ||
           nG != hmms[i]->p->nG || hmms[i]->p->N != hmms[i]->p->N || hmms[i]->p->N_null != hmms[i]->p->N_null ||
           f_multiskill != hmms[i]->p->multiskill ||
           f_update_known != hmms[i]->p->update_known ||
           f_update_unknown != hmms[i]->p->update_unknown ||
           f_predictions != hmms[i]->p->predictions ||
           f_metrics_target_obs != hmms[i]->p->metrics_target_obs) {
            // fprintf(stderr,"Error! One of count variables (N, N_null, nS, nO, nK, nG) or flags (multiskill, predictions, metrics_target_obs, update_known, update_unknown) does not have the same value across multiple models\n");
            exit(1);
        }
    }
    
    NUMBER *local_pred = init1D<NUMBER>(nO); // local prediction
    NUMBER *pLe = init1D<NUMBER>(nS);// p(L|evidence);
    NUMBER pLe_denom; // p(L|evidence) denominator
    NUMBER ***group_skill_map = init3D<NUMBER>(nG, nK, nS);
    
    NUMBER ll = 0.0, ll_no_null = 0.0, rmse = 0.0, rmse_no_null = 0.0, accuracy = 0.0, accuracy_no_null = 0.0;
    NUMBER p;
    
    
    // FILE *fid = NULL; // file for storing prediction should that be necessary
    // if(f_predictions>0) {
    //     fid = fopen(filename,"w");
    //     if(fid == NULL)
    //     {
    //         fprintf(stderr,"Can't write output model file %s\n",filename);
    //         exit(1);
    //     }
    // }
    
    // initialize
    struct data* dt = new data;
    NDAT count = 0;
//    NDAT d = 0;
    HMMProblem *hmm;
    
    for(t=0; t<N; t++) {
        
//        output_this = true;
        o = dat_obs[t];
        g = dat_group[t];
        dt->g = g;
        
        hmm = (nhmms==1)?hmms[0]:hmms[hmm_idx[t]]; // if just one hmm, use 0's, otherwise take the index value
        
        isTarget = hmm->p->metrics_target_obs == o;
        NCAT *ar;
        int n;
        if(f_multiskill==0) {
            k = dat_skill[t];
            ar = &k;
            n = 1;
        } else {
            k = dat_skill_stacked[ dat_skill_rix[t] ];
            ar = &dat_skill_stacked[ dat_skill_rix[t] ];
            n = dat_skill_rcount[t];
        }
        
        // deal with null skill
        if(ar[0]<0) { // if no skill label
            isTarget = hmm->null_skill_obs==o;
            rmse += pow(isTarget - hmm->null_obs_ratio[f_metrics_target_obs],2);
            accuracy += isTarget == (hmm->null_obs_ratio[f_metrics_target_obs]==maxn(hmm->null_obs_ratio,nO) && hmm->null_obs_ratio[f_metrics_target_obs] > 1/nO);
            ll -= isTarget*safelog(hmm->null_skill_obs_prob) + (1-isTarget)*safelog(1 - hmm->null_skill_obs_prob);
            for(m=0; m<nO; m++) {
                // fprintf(fid,"%12.10f%s",hmm->null_obs_ratio[m],(m<(nO-1))?"\t":"\n");//PRINTNOW
            }
            continue;
        }
        // check if {g,k}'s were initialized
        for(int l=0; l<n; l++) {
            k = ar[l];


            it = hmms[0]->p->map_skill_bwd->find(k);
            currentKU = std::string(it->second.c_str());
            hmms[0]->PLs.insert(std::pair<std::string, float>(currentKU, 0.0));

            if( group_skill_map[g][k][0]==0)
            {
                dt->k = k;

                for(i=0; i<nS; i++) {
                    group_skill_map[g][k][i] = hmm->getPI(dt,i);
                    count++;
                }
            }// pLo/pL not set
        }// for all skills at this transaction
        
        // produce prediction and copy to result
        hmm->producePCorrect(group_skill_map, local_pred, ar, n, dt); 
        projectsimplex(local_pred, nO); // addition to make sure there's not side effects
        
        // if necessary guess the obsevaion using Pi and B
        if(f_update_known=='g') {
            NUMBER max_local_pred=0;
            NPAR ix_local_pred=0;
            for(m=0; m<nO; m++) {
                if( local_pred[m]>max_local_pred ) {
                    max_local_pred = local_pred[m];
                    ix_local_pred = m;
                }
            }
            o = ix_local_pred;
        }
        
        // update pL
        for(int l=0; l<n; l++) {
            k = ar[l];
            dt->k = k;
            
            if(o>-1) { // known observations //
                // update p(L)
                pLe_denom = 0.0;
                // 1. pLe =  (L .* B(:,o)) ./ ( L'*B(:,o)+1e-8 );
                for(i=0; i<nS; i++)
                    pLe_denom += group_skill_map[g][k][i] * hmm->getB(dt,i,o);  // TODO: this is local_pred[o]!!!
                for(i=0; i<nS; i++)
                    pLe[i] = group_skill_map[g][k][i] * hmm->getB(dt,i,o) / safe0num(pLe_denom); 
                // 2. L = (pLe'*A)';
                for(i=0; i<nS; i++)
                    group_skill_map[g][k][i]= 0.0; 
                for(j=0; j<nS; j++)
                    for(j=0; j<nS; j++)
                        for(i=0; i<nS; i++)
                            group_skill_map[g][k][j] += pLe[i] * hmm->getA(dt,i,j);//A[i][j]; 
            } else { // unknown observation
                // 2. L = (pL'*A)';
                for(i=0; i<nS; i++)
                    pLe[i] = group_skill_map[g][k][i]; // copy first; 
                for(i=0; i<nS; i++)
                    group_skill_map[g][k][i] = 0.0; // erase old value 
                for(j=0; j<nS; j++)
                    for(i=0; i<nS; i++)
                        group_skill_map[g][k][j] += pLe[i] * hmm->getA(dt,i,j);
            }// observations
            projectsimplex(group_skill_map[g][k], nS); // addition to make sure there's not side effects 
        }
        
        // write prediction out (after update)
        // write prediction out (before pKnown update)
        if(f_predictions>0 /*&& output_this*/) { // write predictions file if it was opened
            for(m=0; m<nO; m++) {
                // printf("%12.10f\n", local_pred[m]);
                // fprintf(fid,"%12.10f%s",local_pred[m],(m<(nO-1))?"\t": ((f_predictions==1)?"\n":"\t") );// if we print states of KCs, continut
                if(m<(nO-1)) // always take the even value
                    hmms[0]->PLs.at(currentKU) = (float) local_pred[m];
            }
            
            if(f_predictions==2) { // if we print out states of KC's as welll
                for(int l=0; l<n; l++) { // all KC here
                //     fprintf(fid,"%12.10f%s",group_skill_map[g][ ar[l] ][0], (l==(n-1) && l==(n-1))?"\n":"\t"); // nnon boost // if end of all states: end line//UNBOOST
                }
            }
        }

        // printf("target %d, hmm->p->metrics_target_obs %d, o %d, local_pred[f_metrics_target_obs] %d, maxn(local_pred,nO) %d\n", 
        // isTarget, hmm->p->metrics_target_obs, o, local_pred[f_metrics_target_obs], maxn(local_pred,nO));

        rmse += pow(isTarget-local_pred[f_metrics_target_obs],2);
        rmse_no_null += pow(isTarget-local_pred[f_metrics_target_obs],2);
        accuracy += isTarget == (local_pred[f_metrics_target_obs]==maxn(local_pred,nO) && local_pred[f_metrics_target_obs]>1/nO);
        accuracy_no_null += isTarget == (local_pred[f_metrics_target_obs]==maxn(local_pred,nO) && local_pred[f_metrics_target_obs]>1/nO);
        p = safe01num(local_pred[f_metrics_target_obs]);
        ll -= safelog(  p)*   isTarget  +  safelog(1-p)*(1-isTarget);
        ll_no_null -= safelog(  p)*   isTarget  +  safelog(1-p)*(1-isTarget);
    } // for all data
    
    delete(dt);
    free(local_pred);
    free(pLe);
    free3D<NUMBER>(group_skill_map, nG, nK);

    rmse = sqrt(rmse / N);
    rmse_no_null = sqrt(rmse_no_null / (N - N_null));
    if(metrics != NULL) {
        metrics[0] = ll;
        metrics[1] = ll_no_null;
        metrics[2] = rmse;
        metrics[3] = rmse_no_null;
        metrics[4] = accuracy/N;
        metrics[5] = accuracy_no_null/(N-N_null);
    }

    // if(f_predictions>0) // close predictions file if it was opened
    //     fclose(fid);
}

NUMBER HMMProblem::getLogLik() { // get log likelihood of the fitted model
    return neg_log_lik;
}

NCAT HMMProblem::getNparams() {
    return this->n_params;
}

NUMBER HMMProblem::getNullSkillObs(NPAR m) {
    return this->null_obs_ratio[m];
}

void HMMProblem::fit() {
    NUMBER* loglik_rmse = init1D<NUMBER>(2);
    FitNullSkill(loglik_rmse, false /*do RMSE*/);
    switch(this->p->solver)
    {
        case METHOD_BW: // Conjugate Gradient Descent
            loglik_rmse[0] += BaumWelch();
            break;
        case METHOD_GD: // Gradient Descent
        case METHOD_CGD: // Conjugate Gradient Descent
        case METHOD_GDL: // Gradient Descent, Lagrange
        case METHOD_GBB: // Brzilai Borwein Gradient Method
            loglik_rmse[0] += GradientDescent();
            break;
        default:
            fprintf(stderr,"Solver specified is not supported.\n");
            break;
    }
    this->neg_log_lik = loglik_rmse[0];
    free(loglik_rmse);
}

void HMMProblem::FitNullSkill(NUMBER* loglik_rmse, bool keep_SE) {
    if(this->p->n_null_skill_group==0) {
        this->null_obs_ratio[0] = 1; // set first obs to 1, simplex preserved
        return; // 0 loglik
    }
    NDAT count_all_null_skill = 0;
    struct data *dat; // used as pointer
    // count occurrences
    NCAT g;
    NDAT t;
    NPAR isTarget, o, m;
    for(g=0; g<this->p->n_null_skill_group; g++) {
        dat = &this->p->null_skills[g];
        if(dat->cnt != 0)
            continue; // observe block
        count_all_null_skill += dat->n;
        for(t=0; t<dat->n; t++) {
            o = this->p->dat_obs[ dat->ix[t] ];
            if(((int)o)>=0) // -1 we skip \xff in char notation
                this->null_obs_ratio[ o ]++;
        }
    }
    // produce means
    this->null_skill_obs = 0;
    this->null_skill_obs_prob = 0;
    for(m=0; m<this->p->nO; m++) {
        this->null_obs_ratio[m] /= count_all_null_skill;
        if( this->null_obs_ratio[m] > this->null_skill_obs_prob ) {
            this->null_skill_obs_prob = this->null_obs_ratio[m];
            this->null_skill_obs = m;
        }
    }
    this->null_skill_obs_prob = safe01num(this->null_skill_obs_prob); // safety for logging
    // compute loglik
    NDAT N = 0;
    for(g=0; g<this->p->n_null_skill_group; g++) {
        dat = &this->p->null_skills[g];
        if(dat->cnt != 0)
            continue; // observe block
        for(t=0; t<dat->n; t++) {
            N += dat->n;
            isTarget = this->p->dat_obs[ dat->ix[t] ] == this->null_skill_obs;
            loglik_rmse[0] -= isTarget*safelog(this->null_skill_obs_prob) + (1-isTarget)*safelog(1-this->null_skill_obs_prob);
            loglik_rmse[1] += pow(isTarget - this->null_skill_obs_prob, 2);
        }
    }
    if(!keep_SE) loglik_rmse[1] = sqrt(loglik_rmse[1]/N);
}

void HMMProblem::init3Params(NUMBER* &PI, NUMBER** &A, NUMBER** &B, NPAR nS, NPAR nO) {
    PI = init1D<NUMBER>((NDAT)nS);
    A  = init2D<NUMBER>((NDAT)nS, (NDAT)nS);
    B  = init2D<NUMBER>((NDAT)nS, (NDAT)nO);
}

void HMMProblem::cpy3Params(NUMBER* &soursePI, NUMBER** &sourseA, NUMBER** &sourseB, NUMBER* &targetPI, NUMBER** &targetA, NUMBER** &targetB, NPAR nS, NPAR nO) {
    cpy1D<NUMBER>(soursePI, targetPI, (NDAT)nS);
    cpy2D<NUMBER>(sourseA,  targetA,  (NDAT)nS, (NDAT)nS);
    cpy2D<NUMBER>(sourseB,  targetB,  (NDAT)nS, (NDAT)nO);
}

void HMMProblem::toZero3Params(NUMBER* &PI, NUMBER** &A, NUMBER** &B, NPAR nS, NPAR nO) {
    toZero1D<NUMBER>(PI, (NDAT)nS);
    toZero2D<NUMBER>(A,  (NDAT)nS, (NDAT)nS);
    toZero2D<NUMBER>(B,  (NDAT)nS, (NDAT)nO);
}

void HMMProblem::free3Params(NUMBER* &PI, NUMBER** &A, NUMBER** &B, NPAR nS) {
    free(PI);
    free2D<NUMBER>(A, (NDAT)nS);
    free2D<NUMBER>(B, (NDAT)nS);
    PI = NULL;
    A  = NULL;
    B  = NULL;
}

FitResult HMMProblem::GradientDescentBit(FitBit *fb) {
    FitResult res;
    FitResult *fr = new FitResult;
    fr->iter = 1;
    fr->pO0  = 0.0;
    fr->pO   = 0.0;
    fr->conv = 0; // converged
    fr->ndat = 0;
    NCAT xndat = fb->xndat;
    struct data **x_data = fb->x_data;
    // inital copy parameter values to the t-1 slice
    fb->copy(FBS_PAR, FBS_PARm1);
    while( !fr->conv && fr->iter<=this->p->maxiter ) {
        fr->ndat = computeGradients(fb);
        
        if(fr->iter==1) {
            fr->pO0 = HMMProblem::getSumLogPOPara(xndat, x_data);
            fr->pOmid = fr->pO0;
        }
        // copy parameter values
        // make ste-
        if( this->p->solver==METHOD_GD || (fr->iter==1 && this->p->solver==METHOD_CGD)  || (fr->iter==1 && this->p->solver==METHOD_GBB) )
            fr->pO = doLinearStep(fb); // step for linked skill 0
        else if( this->p->solver==METHOD_CGD )
            fr->pO = doConjugateLinearStep(fb);
        else if( this->p->solver==METHOD_GDL )
            fr->pO = doLagrangeStep(fb);
        else if( this->p->solver==METHOD_GBB )
            fr->pO = doBarzilaiBorweinStep(fb);
        // converge?
        fr->conv = fb->checkConvergence(fr);
        // copy parameter values after we already compared step t-1 with currently computed step t
        fb->copy(FBS_PARm1, FBS_PARm2); // do this for all in order to capture oscillation, e.g. if new param at t is close to param at t-2 (tolerance)
        fb->copy(FBS_PAR, FBS_PARm1);
        // report if converged
        if( !this->p->quiet && ((fr->conv || fr->iter==this->p->maxiter) )) {
        } else {
            // if Conjugate Gradient
            if (this->p->solver==METHOD_CGD) {
                if( fr->iter==1 ) {
                    fb->copy(FBS_GRAD, FBS_DIRm1); // gradient is not direction, it's negative direction, hence, need to negate it
                    fb->negate(FBS_DIRm1);
                }
                else fb->copy(FBS_DIR,  FBS_DIRm1);
                fb->copy(FBS_GRAD, FBS_GRADm1);
            }
            // if Barzilai Borwein Gradient Method
            if (this->p->solver==METHOD_GBB) {
                fb->copy(FBS_GRAD, FBS_GRADm1);
            }
        }
        fr->iter ++;
        fr->pOmid = fr->pO;
    }// single skill loop
    // cleanup
    RecycleFitData(xndat, x_data, this->p); // recycle memory (Alpha, Beta, p_O_param, Xi, Gamma)
    fr->iter--;
    
    res.iter = fr->iter;
    res.pO0  = fr->pO0;
    res.pO   = fr->pO;
    res.conv = fr->conv;
    res.ndat = fr->ndat;
    delete fr;
    return res;
}

NUMBER HMMProblem::GradientDescent() {
    NCAT x, nX;
    if(this->p->structure==STRUCTURE_SKILL)
        nX = this->p->nK;
    else if (this->p->structure==STRUCTURE_GROUP)
        nX = this->p->nG;
    else
        exit(1);
    NUMBER loglik = 0.0;

    //
    // fit all as 1 skill first
    //
    if(this->p->single_skill>0) {
        FitResult fr;
        fr.pO = 0;
        FitBit *fb = new FitBit(this->p->nS, this->p->nO, this->p->nK, this->p->nG, this->p->tol, this->p->tol_mode);
        // link accordingly
        fb->link( this->getPI(0), this->getA(0), this->getB(0), this->p->nSeq, this->p->k_data);// link skill 0 (we'll copy fit parameters to others
        if(this->p->block_fitting[0]!=0) fb->pi = NULL;
        if(this->p->block_fitting[1]!=0) fb->A  = NULL;
        if(this->p->block_fitting[2]!=0) fb->B  = NULL;

        fb->init(FBS_PARm1);
        fb->init(FBS_GRAD);
        if(this->p->solver==METHOD_CGD) {
            fb->init(FBS_DIR);
            fb->init(FBS_DIRm1);
            fb->init(FBS_GRADm1);
        }
        if(this->p->solver==METHOD_GBB) {
            fb->init(FBS_GRADm1);
        }
        fb->init(FBS_PARm2); // do this for all in order to capture oscillation, e.g. if new param at t is close to param at t-2 (tolerance)

        NCAT* original_ks = Calloc(NCAT, (size_t)this->p->nSeq);
        for(x=0; x<this->p->nSeq; x++) { original_ks[x] = this->p->all_data[x].k; this->p->all_data[x].k = 0; } // save original k's
        fr = GradientDescentBit(fb);
        for(x=0; x<this->p->nSeq; x++) { this->p->all_data[x].k = original_ks[x]; } // restore original k's
        free(original_ks);
        if(!this->p->quiet)
            printf("skill one, seq %5d, dat %8d, iter#%3d p(O|param)= %15.7f >> %15.7f, conv=%d\n", this->p->nSeq, fr.ndat, fr.iter,fr.pO0,fr.pO,fr.conv);
        if(this->p->single_skill==2) {
            for(NCAT y=0; y<this->sizes[0]; y++) { // copy the rest
                NUMBER *aPI = this->getPI(y);
                NUMBER **aA = this->getA(y);
                NUMBER **aB = this->getB(y);
                cpy3Params(fb->pi, fb->A, fb->B, aPI, aA, aB, this->p->nS, this->p->nO);
            }
        }// force single skill
        delete fb;
    }
    //
    // Main fit
    //
    if(this->p->single_skill!=2){
//        #pragma omp for schedule(dynamic) reduction(+:loglik) //PAR
        for(x=0; x<nX; x++) { // if not "force single skill" too
            NCAT xndat;
            struct data** x_data;
            if(this->p->structure==STRUCTURE_SKILL) {
                xndat = this->p->k_numg[x];
                x_data = this->p->k_g_data[x];
            } else if(this->p->structure==STRUCTURE_GROUP) {
                xndat = this->p->g_numk[x];
                x_data = this->p->g_k_data[x];
            } else {
                xndat = 0;
                x_data = NULL;
            }
            FitBit *fb = new FitBit(this->p->nS, this->p->nO, this->p->nK, this->p->nG, this->p->tol, this->p->tol_mode);
            fb->link( this->getPI(x), this->getA(x), this->getB(x), xndat, x_data);
            if(this->p->block_fitting[0]!=0) fb->pi = NULL;
            if(this->p->block_fitting[1]!=0) fb->A  = NULL;
            if(this->p->block_fitting[2]!=0) fb->B  = NULL;
            
            FitResult fr;
            fb->init(FBS_PARm1);
            fb->init(FBS_GRAD);
            if(this->p->solver==METHOD_CGD) {
                fb->init(FBS_DIR);
                fb->init(FBS_DIRm1);
                fb->init(FBS_GRADm1);
            }
            if(this->p->solver==METHOD_GBB) {
                fb->init(FBS_GRADm1);
            }
            fb->init(FBS_PARm2); // do this for all in order to capture oscillation, e.g. if new param at t is close to param at t-2 (tolerance)
            
            fr = GradientDescentBit(fb);
            delete fb;
            
            if( ((fr.conv || fr.iter==this->p->maxiter) )) {
                loglik += fr.pO*(fr.pO>0); // reduction'ed
                if(!this->p->quiet)
                    printf("skill %5d, seq %5d, dat %8d, iter#%3d p(O|param)= %15.7f >> %15.7f, conv=%d\n", x, xndat, fr.ndat, fr.iter,fr.pO0,fr.pO,fr.conv);
            }
        } // for all skills
    }// if not force single skill
//    }//#omp //PAR
 
    return loglik;
}

NUMBER HMMProblem::BaumWelch() {
    NCAT k;
    NUMBER loglik = 0;
    //
    // fit all as 1 skill first
    //
    if(this->p->single_skill>0) {
        FitResult fr;
        fr.pO = 0;
        NCAT x;
        FitBit *fb = new FitBit(this->p->nS, this->p->nO, this->p->nK, this->p->nG, this->p->tol, this->p->tol_mode);
        fb->link( this->getPI(0), this->getA(0), this->getB(0), this->p->nSeq, this->p->k_data);// link skill 0 (we'll copy fit parameters to others
        if(this->p->block_fitting[0]!=0) fb->pi = NULL;
        if(this->p->block_fitting[1]!=0) fb->A  = NULL;
        if(this->p->block_fitting[2]!=0) fb->B  = NULL;

        fb->init(FBS_PARm1);
        fb->init(FBS_PARm2);
        fb->init(FBS_GRAD);
        if(this->p->solver==METHOD_CGD) {
            fb->init(FBS_DIR);
            fb->init(FBS_DIRm1);
            fb->init(FBS_GRADm1);
        }

        NCAT* original_ks = Calloc(NCAT, (size_t)this->p->nSeq);
        for(x=0; x<this->p->nSeq; x++) { original_ks[x] = this->p->all_data[x].k; this->p->all_data[x].k = 0; } // save original k's
        fr = BaumWelchBit(fb);
        for(x=0; x<this->p->nSeq; x++) { this->p->all_data[x].k = original_ks[x]; } // restore original k's
        free(original_ks);
        if(!this->p->quiet)
            printf("skill one, seq %4d, dat %8d, iter#%3d p(O|param)= %15.7f >> %15.7f, conv=%d\n",  this->p->nSeq, fr.ndat, fr.iter,fr.pO0,fr.pO,fr.conv);
        if(this->p->single_skill==2) {
            for(NCAT y=0; y<this->sizes[0]; y++) { // copy the rest
                NUMBER *aPI = this->getPI(y);
                NUMBER **aA = this->getA(y);
                NUMBER **aB = this->getB(y);
                cpy3Params(fb->pi, fb->A, fb->B, aPI, aA, aB, this->p->nS, this->p->nO);
            }
        }// force single skill
        delete fb;
    }
    
    //
    // Main fit
    //
    

//        #pragma omp for schedule(dynamic) reduction(+:loglik) //PAR
        for(k=0; k<this->p->nK; k++) {
            FitBit *fb = new FitBit(this->p->nS, this->p->nO, this->p->nK, this->p->nG, this->p->tol, this->p->tol_mode);
            fb->link(this->getPI(k), this->getA(k), this->getB(k), this->p->k_numg[k], this->p->k_g_data[k]);
            if(this->p->block_fitting[0]!=0) fb->pi = NULL;
            if(this->p->block_fitting[1]!=0) fb->A  = NULL;
            if(this->p->block_fitting[2]!=0) fb->B  = NULL;
            
            fb->init(FBS_PARm1);
            fb->init(FBS_PARm2);
            
            FitResult fr;
            fr = BaumWelchBit(fb);
            delete fb;
            
            if( ((fr.conv || fr.iter==this->p->maxiter) )) {
                loglik += fr.pO*(fr.pO>0); // reduction'ed
                if(!this->p->quiet)
                    printf("skill %4d, seq %4d, dat %8d, iter#%3d p(O|param)= %15.7f >> %15.7f, conv=%d\n", k,  this->p->k_numg[k], fr.ndat, fr.iter,fr.pO0,fr.pO,fr.conv);
            }
        } // for all skills
//    }//PAR
    return loglik;
}

FitResult HMMProblem::BaumWelchBit(FitBit *fb) {
    FitResult fr;
    fr.iter = 1;
    fr.pO0  = 0.0;
    fr.pO   = 0.0;
    fr.conv = 0; // converged
    fr.ndat = 0;
    NCAT xndat = fb->xndat;
    struct data **x_data = fb->x_data;
    
    fr.ndat = -1; // no accounting so far
    while( !fr.conv && fr.iter<=this->p->maxiter ) {
        if(fr.iter==1) {
            fr.ndat = computeAlphaAndPOParam(xndat, x_data);
            fr.pO0 = HMMProblem::getSumLogPOPara(xndat, x_data);
            fr.pOmid = fr.pO0;
        }
        fb->copy(FBS_PAR, FBS_PARm1);
        fr.pO = doBaumWelchStep(fb);// PI, A, B);
        
        // check convergence
        fr.conv = fb->checkConvergence(&fr);
        
        if( fr.conv || fr.iter==this->p->maxiter ) {
            fr.pO = HMMProblem::getSumLogPOPara(xndat, x_data);
        }
        fr.iter ++;
        fr.pOmid = fr.pO;
    } // main solver loop
    // recycle memory (Alpha, Beta, p_O_param, Xi, Gamma)
    RecycleFitData(fb->xndat, fb->x_data, this->p);
    fr.iter--;
    return fr;
    
}

NUMBER HMMProblem::doLinearStep(FitBit *fb) {
    NPAR i,j,m;
    NPAR nS = fb->nS, nO = this->p->nO;
    fb->doLog10ScaleGentle(FBS_GRAD);
    
    NCAT xndat = fb->xndat;
    struct data **x_data = fb->x_data;
    fb->init(FBS_PARcopy);
    fb->init(FBS_GRADcopy);
    
    NUMBER e = this->p->ArmijoSeed; // step seed
    bool compliesArmijo = false;
    bool compliesWolfe2 = false; // second wolfe condition is turned on, if satisfied - honored, if not, just the 1st is used
    NUMBER e_Armijo = -1; // e (step size) at which Armijo (Wolfe 1) criterion is satisfied, 'cos both criterions are sometimes not met.
    NUMBER f_xkplus1_Armijo = 0; // f_xkplus1_Armijo at which Armijo (Wolfe 1) criterion is satisfied, 'cos both criterions are sometimes not met.
    NUMBER f_xk = HMMProblem::getSumLogPOPara(xndat, x_data);
    NUMBER f_xkplus1 = 0;
    
    fb->copy(FBS_PAR, FBS_PARcopy); // save original parameters
    fb->copy(FBS_GRAD, FBS_GRADcopy); // save original gradient
    // compute p_k * -p_k
    NUMBER p_k_by_neg_p_k = 0;
    for(i=0; i<nS; i++)
    {
        if(fb->pi != NULL) p_k_by_neg_p_k -= fb->gradPI[i]*fb->gradPI[i];
        if(fb->A  != NULL) for(j=0; j<nS; j++) p_k_by_neg_p_k -= fb->gradA[i][j]*fb->gradA[i][j];
        if(fb->B  != NULL) for(m=0; m<nO; m++) p_k_by_neg_p_k -= fb->gradB[i][m]*fb->gradB[i][m];
    }
    
    int iter = 0; // limit iter steps to 20, via ArmijoMinStep (now 10)
    while( !(compliesArmijo && compliesWolfe2) && e > this->p->ArmijoMinStep) {
        // update
        for(i=0; i<nS; i++) {
            if(fb->pi != NULL) {
                fb->pi[i] = fb->PIcopy[i] - e * fb->gradPIcopy[i];
                if( (fb->pi[i]<0 || fb->pi[i] >1) && (fb->pi[i] > 0) && (fb->pi[i] < 1) ) {
                    fprintf(stderr, "ERROR! pi value is not within [0, 1] range!\n");
                }
            }
            
            if(fb->A  != NULL)
                for(j=0; j<nS; j++) {
                    fb->A[i][j] = fb->Acopy[i][j] - e * fb->gradAcopy[i][j];
                    if( (fb->A[i][j]<0 || fb->A[i][j] >1) && (fb->A[i][j] > 0) && (fb->A[i][j] < 1) ) {
                        fprintf(stderr, "ERROR! A value is not within [0, 1] range!\n");
                    }
                }
            
            if(fb->B  != NULL)
                for(m=0; m<nO; m++) {
                    fb->B[i][m] = fb->Bcopy[i][m] - e * fb->gradBcopy[i][m];
                    if( (fb->B[i][m]<0 || fb->B[i][m] >1) && (fb->B[i][m] > 0) && (fb->B[i][m] < 1) ) {
                        fprintf(stderr, "ERROR! B value is not within [0, 1] range!\n");
                    }
                }
        }
        // project parameters to simplex if needs be
        if(fb->projecttosimplex==1) {
            // scale
            if( !this->hasNon01Constraints() ) {
                if(fb->pi != NULL) projectsimplex(fb->pi, nS);
                for(i=0; i<nS; i++) {
                    if(fb->A  != NULL) projectsimplex(fb->A[i], nS);
                    if(fb->B  != NULL) projectsimplex(fb->B[i], nO);
                }
            } else {
                if(fb->pi != NULL) projectsimplexbounded(fb->pi, this->getLbPI(), this->getUbPI(), nS);
                for(i=0; i<nS; i++) {
                    if(fb->A  != NULL) projectsimplexbounded(fb->A[i], this->getLbA()[i], this->getUbA()[i], nS);
                    if(fb->B  != NULL) projectsimplexbounded(fb->B[i], this->getLbB()[i], this->getUbB()[i], nO);
                }
            }
        }
        
        // recompute alpha and p(O|param)
        computeAlphaAndPOParam(xndat, x_data);
        // compute f(x_{k+1})
        f_xkplus1 = HMMProblem::getSumLogPOPara(xndat, x_data);
        // compute Armijo compliance
        compliesArmijo = (f_xkplus1 <= (f_xk + (this->p->ArmijoC1 * e * p_k_by_neg_p_k)));
        
        // compute Wolfe 2
        NUMBER p_k_by_neg_p_kp1 = 0;
        computeGradients(fb);
        fb->doLog10ScaleGentle(FBS_GRAD);
        for(i=0; i<nS; i++)
        {
            if(fb->pi != NULL) p_k_by_neg_p_kp1 -= fb->gradPIcopy[i]*fb->gradPI[i];
            if(fb->A  != NULL) for(j=0; j<nS; j++) p_k_by_neg_p_kp1 -= fb->gradAcopy[i][j]*fb->gradA[i][j];
            if(fb->B  != NULL) for(m=0; m<nO; m++) p_k_by_neg_p_kp1 -= fb->gradBcopy[i][m]*fb->gradB[i][m];
        }
        compliesWolfe2 = (p_k_by_neg_p_kp1 >= this->p->ArmijoC2 * p_k_by_neg_p_k);
        
        if( compliesArmijo && e_Armijo==-1 ){
            e_Armijo = e; // save the first time Armijo is statisfied, in case we'll roll back to it when Wolfe 2 is finnaly not satisfied
            f_xkplus1_Armijo = f_xkplus1;
        }
        
        e /= (compliesArmijo && compliesWolfe2)?1:this->p->ArmijoReduceFactor;
        iter++;
    } // armijo loop
    if(!compliesArmijo) { // we couldn't step away from current, copy the inital point back
        e = 0;
        fb->copy(FBS_PARcopy, FBS_PAR);
        f_xkplus1 = f_xk;
    } else if(compliesArmijo && !compliesWolfe2) { // we couldn't step away from current, copy the inital point back
        e = e_Armijo; // return the first Armijo-compliant e
        f_xkplus1 = f_xkplus1_Armijo; // return the first Armijo-compliant f_xkplus1
        // create new versions of FBS_PAR using e_Armijo as a step
        fb->copy(FBS_GRADcopy, FBS_GRAD); // return the old gradient
        // update
        for(i=0; i<nS; i++) {
            if(fb->pi != NULL) {
                fb->pi[i] = fb->PIcopy[i] - e * fb->gradPIcopy[i];
                if( (fb->pi[i]<0 || fb->pi[i] >1) && (fb->pi[i] > 0) && (fb->pi[i] < 1) ) {
                    fprintf(stderr, "ERROR! pi value is not within [0, 1] range!\n");
                }
            }
            if(fb->A  != NULL)
                for(j=0; j<nS; j++) {
                    fb->A[i][j] = fb->Acopy[i][j] - e * fb->gradAcopy[i][j];
                    if( (fb->A[i][j]<0 || fb->A[i][j] >1) && (fb->A[i][j] > 0) && (fb->A[i][j] < 1) ) {
                        fprintf(stderr, "ERROR! A value is not within [0, 1] range!\n");
                    }
                }
            if(fb->B  != NULL)
                for(m=0; m<nO; m++) {
                    fb->B[i][m] = fb->Bcopy[i][m] - e * fb->gradBcopy[i][m];
                    if( (fb->B[i][m]<0 || fb->B[i][m] >1) && (fb->B[i][m] > 0) && (fb->B[i][m] < 1) ) {
                        fprintf(stderr, "ERROR! B value is not within [0, 1] range!\n");
                    }
                }
        }
        // project parameters to simplex if needs be
        if(fb->projecttosimplex==1) {
            // scale
            if( !this->hasNon01Constraints() ) {
                if(fb->pi != NULL) projectsimplex(fb->pi, nS);
                for(i=0; i<nS; i++) {
                    if(fb->A  != NULL) projectsimplex(fb->A[i], nS);
                    if(fb->B  != NULL) projectsimplex(fb->B[i], nO);
                }
            } else {
                if(fb->pi != NULL) projectsimplexbounded(fb->pi, this->getLbPI(), this->getUbPI(), nS);
                for(i=0; i<nS; i++) {
                    if(fb->A  != NULL) projectsimplexbounded(fb->A[i], this->getLbA()[i], this->getUbA()[i], nS);
                    if(fb->B  != NULL) projectsimplexbounded(fb->B[i], this->getLbB()[i], this->getUbB()[i], nO);
                }
            }
        }
        // ^^^^^ end of create new versions of FBS_PAR using e_Armijo as a step
    }
    fb->destroy(FBS_PARcopy);
    fb->destroy(FBS_GRADcopy);
    return f_xkplus1;
} // doLinearStep

NUMBER HMMProblem::doLagrangeStep(FitBit *fb) {
    NPAR nS = this->p->nS, nO = this->p->nO;
    NPAR i,j,m;
    
    NUMBER  ll = 0;
    NUMBER * b_PI = init1D<NUMBER>((NDAT)this->p->nS);
    NUMBER   b_PI_den = 0;
    NUMBER ** b_A     = init2D<NUMBER>((NDAT)this->p->nS, (NDAT)this->p->nS);
    NUMBER *  b_A_den = init1D<NUMBER>((NDAT)this->p->nS);
    NUMBER ** b_B     = init2D<NUMBER>((NDAT)this->p->nS, (NDAT)this->p->nO);
    NUMBER *  b_B_den = init1D<NUMBER>((NDAT)this->p->nS);
    // compute sums PI
    
    NUMBER buf = 0;
    // collapse
    for(i=0; i<nS; i++) {
        if(fb->pi != NULL)
            buf = -fb->pi[i]*fb->gradPI[i]; /*negatie since were going against gradient*/
        b_PI_den += buf;
        b_PI[i] += buf;
        if(fb->A != NULL)
            for(j=0; j<nS; j++){
                buf = -fb->A[i][j]*fb->gradA[i][j]; /*negatie since were going against gradient*/
                b_A_den[i] += buf;
                b_A[i][j] += buf;
            }
        if(fb->B != NULL)
            for(m=0; m<nO; m++) {
                buf = -fb->B[i][m]*fb->gradB[i][m]; /*negatie since were going against gradient*/
                b_B_den[i] += buf;
                b_B[i][m] += buf;
            }
    }
    // divide
    for(i=0; i<nS; i++) {
        if(fb->pi != NULL)
            fb->pi[i] = (b_PI_den>0) ? (b_PI[i] / b_PI_den) : fb->pi[i];
        if(fb->A != NULL)
            for(j=0; j<nS; j++)
                fb->A[i][j] = (b_A_den[i]>0) ? (b_A[i][j] / b_A_den[i]) : fb->A[i][j];
        if(fb->B != NULL)
            for(m=0; m<nO; m++)
                fb->B[i][m] = (b_B_den[i]>0) ? (b_B[i][m] / b_B_den[i]) : fb->B[i][m];
    }
    // scale
    if( !this->hasNon01Constraints() ) {
        if(fb->pi != NULL) projectsimplex(fb->pi, nS);
        for(i=0; i<nS; i++) {
            if(fb->A != NULL) projectsimplex(fb->A[i], nS);
            if(fb->B != NULL) projectsimplex(fb->B[i], nO);
        }
    } else {
        if(fb->pi != NULL) projectsimplexbounded(fb->pi, this->getLbPI(), this->getUbPI(), nS);
        for(i=0; i<nS; i++) {
            if(fb->A != NULL) projectsimplexbounded(fb->A[i], this->getLbA()[i], this->getUbA()[i], nS);
            if(fb->B != NULL) projectsimplexbounded(fb->B[i], this->getLbB()[i], this->getUbB()[i], nO);
        }
    }
    // compute LL
    computeAlphaAndPOParam(fb->xndat, fb->x_data);
    ll = HMMProblem::getSumLogPOPara(fb->xndat, fb->x_data);

    free(b_PI);
    free2D<NUMBER>(b_A, nS);
    free(b_A_den);
    free2D<NUMBER>(b_B, nS);
    free(b_B_den);
    
    return ll;
} // doLagrangeStep

NUMBER HMMProblem::doConjugateLinearStep(FitBit *fb) {
    NPAR i=0, j=0, m=0;
    NPAR nS = this->p->nS, nO = this->p->nO;
    // first scale down gradients
    fb->doLog10ScaleGentle(FBS_GRAD);
    
    // compute beta_gradient_direction
    NUMBER beta_grad = 0, beta_grad_num = 0, beta_grad_den = 0;
    
    switch (this->p->solver_setting) {
        case 1: // Fletcher-Reeves
            for(i=0; i<nS; i++)
            {
                if(fb->pi != NULL) {
                    beta_grad_num += fb->gradPI  [i]*fb->gradPI  [i];
                    beta_grad_den += fb->gradPIm1[i]*fb->gradPIm1[i];
                }
                if(fb->A  != NULL)
                    for(j=0; j<nS; j++) {
                        beta_grad_num += fb->gradA  [i][j]*fb->gradA  [i][j];
                        beta_grad_den += fb->gradAm1[i][j]*fb->gradAm1[i][j];
                    }
                if(fb->B  != NULL)
                    for(m=0; m<nO; m++) {
                        beta_grad_num += fb->gradB  [i][m]*fb->gradB  [i][m];
                        beta_grad_den += fb->gradBm1[i][m]*fb->gradBm1[i][m];
                    }
            }
            break;
        case 2: // Polak–Ribiere
            for(i=0; i<nS; i++)
            {
                if(fb->pi != NULL) {
                    beta_grad_num += -fb->gradPI[i]*(-fb->gradPI[i] + fb->gradPIm1[i]);
                    beta_grad_den +=  fb->gradPIm1[i]*fb->gradPIm1[i];
                }
                if(fb->A != NULL)
                    for(j=0; j<nS; j++) {
                        beta_grad_num += -fb->gradA[i][j]*(-fb->gradA[i][j] + fb->gradAm1[i][j]);
                        beta_grad_den +=  fb->gradAm1[i][j]*fb->gradAm1[i][j];
                    }
                if(fb->B  != NULL)
                    for(m=0; m<nO; m++) {
                        beta_grad_num += -fb->gradB[i][m]*(-fb->gradB[i][m] + fb->gradBm1[i][m]);
                        beta_grad_den +=  fb->gradBm1[i][m]*fb->gradBm1[i][m];
                    }
            }
            break;
        case 3: // Hestenes-Stiefel
            for(i=0; i<nS; i++)
            {
                if(fb->pi != NULL) {
                    beta_grad_num +=  fb->gradPI[i]* (-fb->gradPI[i] + fb->gradPIm1[i]); // no -, since neg gradient and - is +
                    beta_grad_den +=  fb->dirPIm1[i]*(-fb->gradPI[i] + fb->gradPIm1[i]);
                }
                if(fb->A  != NULL)
                    for(j=0; j<nS; j++) {
                        beta_grad_num +=  fb->gradA[i][j]* (-fb->gradA[i][j] + fb->gradAm1[i][j]);
                        beta_grad_den +=  fb->dirAm1[i][j]*(-fb->gradA[i][j] + fb->gradAm1[i][j]);
                    }
                if(fb->B  != NULL)
                    for(m=0; m<nO; m++) {
                        beta_grad_num +=  fb->gradB[i][m]* (-fb->gradB[i][m] + fb->gradBm1[i][m]);
                        beta_grad_den +=  fb->dirBm1[i][m]*(-fb->gradB[i][m] + fb->gradBm1[i][m]);
                    }
            }
            break;
        case 4: // Dai-Yuan
            for(i=0; i<nS; i++)
            {
                if(fb->pi != NULL) {
                    beta_grad_num += -fb->gradPI [i]*fb->gradPI  [i];
                    beta_grad_den +=  fb->dirPIm1[i]*(-fb->gradPI[i] + fb->gradPIm1[i]);
                }
                if(fb->A  != NULL)
                    for(j=0; j<nS; j++) {
                        beta_grad_num += -fb->gradA [i][j]*fb->gradA  [i][j];
                        beta_grad_den +=  fb->dirAm1[i][j]*(-fb->gradA[i][j] + fb->gradAm1[i][j]);
                    }
                if(fb->B  != NULL)
                    for(m=0; m<nO; m++) {
                        beta_grad_num += -fb->gradB [i][m]*fb->gradB  [i][m];
                        beta_grad_den +=  fb->dirBm1[i][m]*(-fb->gradB[i][m] + fb->gradBm1[i][m]);
                    }
            }
            break;
        default:
            fprintf(stderr,"Wrong stepping algorithm (%d) specified for Conjugate Gradient Descent\n",this->p->solver_setting);
            break;
    }
    beta_grad = beta_grad_num / safe0num(beta_grad_den);
    beta_grad = (beta_grad>=0)?beta_grad:0;
    // compute new direction
    fb->toZero(FBS_DIR);
    for(i=0; i<nS; i++)
    {
        if(fb->pi != NULL) fb->dirPI[i] = -fb->gradPI[i] + beta_grad * fb->dirPIm1[i];
        if(fb->A  != NULL) for(j=0; j<nS; j++) fb->dirA[i][j] = -fb->gradA[i][j] + beta_grad * fb->dirAm1[i][j];
        if(fb->B  != NULL) for(m=0; m<nO; m++) fb->dirB[i][m] = -fb->gradB[i][m] + beta_grad * fb->dirBm1[i][m];
    }
    // scale direction
    fb->doLog10ScaleGentle(FBS_DIR);
    
    NUMBER e = this->p->ArmijoSeed; // step seed
    bool compliesArmijo = false;
    bool compliesWolfe2 = false; // second wolfe condition is turned on, if satisfied - honored, if not, just the 1st is used
    NUMBER e_Armijo = -1; // e (step size) at which Armijo (Wolfe 1) criterion is satisfied, 'cos both criterions are sometimes not met.
    NUMBER f_xkplus1_Armijo = 0; // f_xkplus1_Armijo at which Armijo (Wolfe 1) criterion is satisfied, 'cos both criterions are sometimes not met.
    NUMBER f_xk = HMMProblem::getSumLogPOPara(fb->xndat, fb->x_data);
    NUMBER f_xkplus1 = 0;
    
    fb->init(FBS_PARcopy);
    fb->init(FBS_GRADcopy);
    
    fb->copy(FBS_PAR, FBS_PARcopy); // copy parameter
    fb->copy(FBS_GRAD, FBS_GRADcopy); // copy initial gradient
    // compute p_k * -p_k >>>> now current gradient by current direction
    NUMBER p_k_by_neg_p_k = 0;
    for(i=0; i<nS; i++)
    {
        if(fb->pi != NULL) p_k_by_neg_p_k += fb->gradPI[i]*fb->dirPI[i];
        if(fb->A  != NULL) for(j=0; j<nS; j++) p_k_by_neg_p_k += fb->gradA[i][j]*fb->dirA[i][j];
        if(fb->B  != NULL) for(m=0; m<nO; m++) p_k_by_neg_p_k += fb->gradB[i][m]*fb->dirB[i][m];
    }
    int iter = 0; // limit iter steps to 20, actually now 10, via ArmijoMinStep
    while( !compliesArmijo && e > this->p->ArmijoMinStep) {
        // update
        for(i=0; i<nS; i++) {
            if(fb->pi != NULL) fb->pi[i] = fb->PIcopy[i] + e * fb->dirPI[i];
            if(fb->A  != NULL)
                for(j=0; j<nS; j++)
                    fb->A[i][j] = fb->Acopy[i][j] + e * fb->dirA[i][j];
            if(fb->B  != NULL)
                for(m=0; m<nO; m++)
                    fb->B[i][m] = fb->Bcopy[i][m] + e * fb->dirB[i][m];
        }
        // scale
        if( !this->hasNon01Constraints() ) {
            if(fb->pi != NULL) projectsimplex(fb->pi, nS);
            for(i=0; i<nS; i++) {
                if(fb->A != NULL) projectsimplex(fb->A[i], nS);
                if(fb->B != NULL) projectsimplex(fb->B[i], nO);
            }
        } else {
            if(fb->pi != NULL) projectsimplexbounded(fb->pi, this->getLbPI(), this->getUbPI(), nS);
            for(i=0; i<nS; i++) {
                if(fb->A != NULL) projectsimplexbounded(fb->A[i], this->getLbA()[i], this->getUbA()[i], nS);
                if(fb->B != NULL) projectsimplexbounded(fb->B[i], this->getLbB()[i], this->getUbB()[i], nO);
            }
        }
        // recompute alpha and p(O|param)
        computeAlphaAndPOParam(fb->xndat, fb->x_data);
        // compute f(x_{k+1})
        f_xkplus1 = HMMProblem::getSumLogPOPara(fb->xndat, fb->x_data);
        // compute Armijo compliance
        compliesArmijo = (f_xkplus1 <= (f_xk + (this->p->ArmijoC1 * e * p_k_by_neg_p_k)));
        // compute Wolfe 2
        NUMBER p_k_by_neg_p_kp1 = 0;
        computeGradients(fb);
        fb->doLog10ScaleGentle(FBS_GRAD);
        for(i=0; i<nS; i++)
        {
            if(fb->pi != NULL) p_k_by_neg_p_kp1 += fb->dirPI[i]*fb->gradPI[i];
            if(fb->A  != NULL) for(j=0; j<nS; j++) p_k_by_neg_p_kp1 += fb->dirA[i][j]*fb->gradA[i][j];
            if(fb->B  != NULL) for(m=0; m<nO; m++) p_k_by_neg_p_kp1 += fb->dirB[i][m]*fb->gradB[i][m];
        }
        compliesWolfe2 = (p_k_by_neg_p_kp1 >= this->p->ArmijoC2 * p_k_by_neg_p_k);
        
        if( compliesArmijo && e_Armijo==-1 ){
            e_Armijo = e; // save the first time Armijo is statisfied, in case we'll roll back to it when Wolfe 2 is finnaly not satisfied
            f_xkplus1_Armijo = f_xkplus1;
        }
        
        e /= (compliesArmijo && compliesWolfe2)?1:this->p->ArmijoReduceFactor;
        iter++;
    } // armijo loop
    
    fb->copy(FBS_GRADcopy, FBS_GRAD); // return the original gradient in its place
    
    if(!compliesArmijo) { // we couldn't step away from current, copy the inital point back
        e = 0;
        fb->copy(FBS_PARcopy, FBS_PAR);
        f_xkplus1 = f_xk;
    } else if(compliesArmijo && !compliesWolfe2) { // we couldn't step away from current, copy the inital point back
        e = e_Armijo;
        f_xkplus1 = f_xkplus1_Armijo;
        // create new versions of FBS_PAR using e_Armijo as a step
        // update
        for(i=0; i<nS; i++) {
            if(fb->pi != NULL) {
                fb->pi[i] = fb->PIcopy[i] + e * fb->dirPI[i];
                if( (fb->pi[i]<0 || fb->pi[i] >1) && (fb->pi[i] > 0) && (fb->pi[i] < 1) ) {
                    fprintf(stderr, "ERROR! pi value is not within [0, 1] range!\n");
                }
            }
            if(fb->A  != NULL)
                for(j=0; j<nS; j++) {
                    fb->A[i][j] = fb->Acopy[i][j] + e * fb->dirA[i][j];
                    if( (fb->A[i][j]<0 || fb->A[i][j] >1) && (fb->A[i][j] > 0) && (fb->A[i][j] < 1) ) {
                        fprintf(stderr, "ERROR! A value is not within [0, 1] range!\n");
                    }
                }
            if(fb->B  != NULL)
                for(m=0; m<nO; m++) {
                    fb->B[i][m] = fb->Bcopy[i][m] + e * fb->dirB[i][m];
                    if( (fb->B[i][m]<0 || fb->B[i][m] >1) && (fb->B[i][m] > 0) && (fb->B[i][m] < 1) ) {
                        fprintf(stderr, "ERROR! B value is not within [0, 1] range!\n");
                    }
                }
        }
        // project parameters to simplex if needs be
        if(fb->projecttosimplex==1) {
            // scale
            if( !this->hasNon01Constraints() ) {
                if(fb->pi != NULL) projectsimplex(fb->pi, nS);
                for(i=0; i<nS; i++) {
                    if(fb->A  != NULL) projectsimplex(fb->A[i], nS);
                    if(fb->B  != NULL) projectsimplex(fb->B[i], nO);
                }
            } else {
                if(fb->pi != NULL) projectsimplexbounded(fb->pi, this->getLbPI(), this->getUbPI(), nS);
                for(i=0; i<nS; i++) {
                    if(fb->A  != NULL) projectsimplexbounded(fb->A[i], this->getLbA()[i], this->getUbA()[i], nS);
                    if(fb->B  != NULL) projectsimplexbounded(fb->B[i], this->getLbB()[i], this->getUbB()[i], nO);
                }
            }
        }
        // ^^^^^ end of create new versions of FBS_PAR using e_Armijo as a step
    }
    fb->destroy(FBS_PARcopy);
    fb->destroy(FBS_GRADcopy);
    return f_xkplus1;
} // doLinearStep


NUMBER HMMProblem::doBarzilaiBorweinStep(FitBit *fb) {
    NPAR i,j,m;
    NPAR nS = this->p->nS, nO = this->p->nO;
    // first scale down gradients
    fb->doLog10ScaleGentle(FBS_GRAD);
    
    // compute s_k_m1
      NUMBER *s_k_m1_PI = init1D<NUMBER>((NDAT)nS);
    NUMBER **s_k_m1_A = init2D<NUMBER>((NDAT)nS, (NDAT)nS);
    NUMBER **s_k_m1_B = init2D<NUMBER>((NDAT)nS, (NDAT)nS);
    for(i=0; i<nS; i++)
    {
        s_k_m1_PI[i] = fb->PIm1[i] - fb->PIm2[i];
        for(j=0; j<nS; j++) s_k_m1_A[i][j] = fb->Am1[i][j] - fb->Am2[i][j];
        for(m=0; m<this->p->nO; m++) s_k_m1_B[i][m] = fb->Bm1[i][m] - fb->Bm2[i][m];
    }
    // compute alpha_step
    NUMBER alpha_step = 0, alpha_step_num = 0, alpha_step_den = 0;
    // Barzilai Borwein: s' * s / ( s' * (g-g_m1) )
    for(i=0; i<nS; i++)
    {
        alpha_step_num += s_k_m1_PI[i]*s_k_m1_PI[i];
        alpha_step_den += s_k_m1_PI[i]*(fb->gradPI[i] - fb->gradPIm1[i]);
        for(j=0; j<nS; j++) {
            alpha_step_num += s_k_m1_A[i][j]*s_k_m1_A[i][j];
            alpha_step_den += s_k_m1_A[i][j]*(fb->gradA[i][j] - fb->gradAm1[i][j]);
        }
        for(m=0; m<nO; m++) {
            alpha_step_num += s_k_m1_B[i][m]*s_k_m1_B[i][m];
            alpha_step_den += s_k_m1_B[i][m]*(fb->gradB[i][m] - fb->gradBm1[i][m]);
        }
    }
    alpha_step = alpha_step_num / safe0num(alpha_step_den);
    
    // step
    for(i=0; i<nS; i++) {
        fb->pi[i] = fb->pi[i] - alpha_step * fb->gradPI[i];
        for(j=0; j<nS; j++)
            fb->A[i][j] = fb->A[i][j] - alpha_step * fb->gradA[i][j];
        for(m=0; m<nO; m++)
            fb->B[i][m] = fb->B[i][m] - alpha_step * fb->gradB[i][m];
    }
    // scale
    if( !this->hasNon01Constraints() ) {
        projectsimplex(fb->pi, nS);
        for(i=0; i<nS; i++) {
            projectsimplex(fb->A[i], nS);
            projectsimplex(fb->B[i], nO);
        }
    } else {
        projectsimplexbounded(fb->pi, this->getLbPI(), this->getUbPI(), nS);
        for(i=0; i<nS; i++) {
            projectsimplexbounded(fb->A[i], this->getLbA()[i], this->getUbA()[i], nS);
            projectsimplexbounded(fb->B[i], this->getLbB()[i], this->getUbB()[i], nO);
        }
    }
    free(s_k_m1_PI);
    free2D<NUMBER>(s_k_m1_B, nS);
    free2D<NUMBER>(s_k_m1_A, nS);

    // recompute alpha and p(O|param)
    computeAlphaAndPOParam(fb->xndat, fb->x_data);
    return HMMProblem::getSumLogPOPara(fb->xndat, fb->x_data);
}

NUMBER HMMProblem::doBaumWelchStep(FitBit *fb) {
    NCAT x;
    NPAR nS = this->p->nS, nO = this->p->nO;
    NPAR i,j,m, o;
    NDAT t;
    NUMBER ll;
    
    NCAT xndat = fb->xndat;
    struct data **x_data = fb->x_data;
    computeAlphaAndPOParam(xndat, x_data);
    computeBeta(xndat, x_data);
    computeXiGamma(xndat, x_data);
    
    NUMBER * b_PI = NULL;
    NUMBER ** b_A_num = NULL;
    NUMBER ** b_A_den = NULL;
    NUMBER ** b_B_num = NULL;
    NUMBER ** b_B_den = NULL;
    if(fb->pi != NULL)
        b_PI = init1D<NUMBER>((NDAT)nS);
    if(fb->A != NULL) {
        b_A_num = init2D<NUMBER>((NDAT)nS, (NDAT)nS);
        b_A_den = init2D<NUMBER>((NDAT)nS, (NDAT)nS);
    }
    if(fb->B != NULL) {
        b_B_num = init2D<NUMBER>((NDAT)nS, (NDAT)nO);
        b_B_den = init2D<NUMBER>((NDAT)nS, (NDAT)nO);
    }

    // compute sums PI

    for(x=0; x<xndat; x++) {
        if( x_data[x]->cnt!=0 ) continue;
        
        if(fb->pi != NULL)
            for(i=0; i<nS; i++)
                b_PI[i] += x_data[x]->gamma[0][i] / xndat;
        
        for(t=0;t<(x_data[x]->n-1);t++) {
            o = this->p->dat_obs[ x_data[x]->ix[t] ];
            for(i=0; i<nS; i++) {
                if(fb->A != NULL)
                    for(j=0; j<nS; j++){
                        b_A_num[i][j] += x_data[x]->xi[t][i][j];
                        b_A_den[i][j] += x_data[x]->gamma[t][i];
                    }
                if(fb->B != NULL)
                    for(m=0; m<nO; m++) {
                        b_B_num[i][m] += (m==o) * x_data[x]->gamma[t][i];
                        b_B_den[i][m] += x_data[x]->gamma[t][i];
                    }
            }
        }
    } // for all groups within a skill
    // set params
    for(i=0; i<nS; i++) {
        if(fb->pi != NULL)
            fb->pi[i] = b_PI[i];
        if(fb->A != NULL)
            for(j=0; j<nS; j++)
                fb->A[i][j] = b_A_num[i][j] / safe0num(b_A_den[i][j]);
        if(fb->B != NULL)
            for(m=0; m<nO; m++)
                fb->B[i][m] = b_B_num[i][m] / safe0num(b_B_den[i][m]);
    }
    // scale
    if( !this->hasNon01Constraints() ) {
        if(fb->pi != NULL)
            projectsimplex(fb->pi, nS);
        for(i=0; i<nS; i++) {
            if(fb->A != NULL)
                projectsimplex(fb->A[i], nS);
            if(fb->B != NULL)
                projectsimplex(fb->B[i], nO);
        }
    } else {
        if(fb->pi != NULL)
            projectsimplexbounded(fb->pi, this->getLbPI(), this->getUbPI(), nS);
        for(i=0; i<nS; i++) {
            if(fb->A != NULL)
                projectsimplexbounded(fb->A[i], this->getLbA()[i], this->getUbA()[i], nS);
            if(fb->B != NULL)
                projectsimplexbounded(fb->B[i], this->getLbB()[i], this->getUbB()[i], nO);
        }
    }
    // compute LL
    computeAlphaAndPOParam(fb->xndat, fb->x_data);
    ll = HMMProblem::getSumLogPOPara(fb->xndat, fb->x_data);
    // free mem
    if(b_PI    != NULL) free(b_PI);
    if(b_A_num != NULL) free2D<NUMBER>(b_A_num, nS);
    if(b_A_den != NULL) free2D<NUMBER>(b_A_den, nS);
    if(b_B_num != NULL) free2D<NUMBER>(b_B_num, nS);
    if(b_B_den != NULL) free2D<NUMBER>(b_B_den, nS);
    return ll;
}

void HMMProblem::readNullObsRatio(FILE *fid, struct param* param, NDAT *line_no) {
    NPAR i;
    //
    // read null skill ratios
    //
    fscanf(fid, "Null skill ratios\t");
    this->null_obs_ratio =Calloc(NUMBER, (size_t)this->p->nO);
    this->null_skill_obs      = 0;
    this->null_skill_obs_prob = 0;
    for(i=0; i<param->nO; i++) {
        if( i==(param->nO-1) ) // end
            fscanf(fid,"%lf\n",&this->null_obs_ratio[i] );
        else
            fscanf(fid,"%lf\t",&this->null_obs_ratio[i] );
        
        if( this->null_obs_ratio[i] > this->null_skill_obs_prob ) {
            this->null_skill_obs_prob = this->null_obs_ratio[i];
            this->null_skill_obs = i;
        }
    }
    (*line_no)++;
}

void HMMProblem::readModel(const char *filename, bool overwrite) {
    FILE *fid = fopen(filename,"r");
    if(fid == NULL)
    {
        fprintf(stderr,"Can't read model file %s\n",filename);
        exit(1);
    }
    int max_line_length = 1024;
    char *line = Malloc(char,(size_t)max_line_length);
    NDAT line_no = 0;
    struct param initparam;
    set_param_defaults(&initparam);
    
    //
    // read solver info
    //
    if(overwrite)
        readSolverInfo(fid, this->p, &line_no);
    else
        readSolverInfo(fid, &initparam, &line_no);
    //
    // read model
    //
    readModelBody(fid, &initparam, &line_no, overwrite);
        
    fclose(fid);
    free(line);
}

void HMMProblem::readModelBody(FILE *fid, struct param* param, NDAT *line_no,  bool overwrite) {
    NPAR i,j,m;
    NCAT k = 0, idxk = 0;
    std::map<std::string,NCAT>::iterator it;
    string s;
    char col[2048];
    //
    readNullObsRatio(fid, param, line_no);
    //
    // init param
    //
    if(overwrite) {
        this->p->map_group_fwd = new map<string,NCAT>();
        this->p->map_group_bwd = new map<NCAT,string>();
        this->p->map_skill_fwd = new map<string,NCAT>();
        this->p->map_skill_bwd = new map<NCAT,string>();
    }
    //
    // read skills
    //
    for(k=0; k<param->nK; k++) {
        // read skill label
        fscanf(fid,"%*s\t%[^\n]\n",col);
        s = string( col );
        (*line_no)++;
        if(overwrite) {
            this->p->map_skill_fwd->insert(pair<string,NCAT>(s, (NCAT)this->p->map_skill_fwd->size()));
            this->p->map_skill_bwd->insert(pair<NCAT,string>((NCAT)this->p->map_skill_bwd->size(), s));
            idxk = k;
        } else {
            it = this->p->map_skill_fwd->find(s);
            if( it==this->p->map_skill_fwd->end() ) { // not found, skip 3 lines and continue
                fscanf(fid, "%*[^\n]\n");
                fscanf(fid, "%*[^\n]\n");
                fscanf(fid, "%*[^\n]\n");
                (*line_no)+=3;
                continue; // skip this iteration
            }
            else
                idxk =it->second;
        }
        // read PI
        fscanf(fid,"PI\t");
        for(i=0; i<(this->p->nS-1); i++) { // read 1 less then necessary
            fscanf(fid,"%[^\t]\t",col);
            this->pi[idxk][i] = atof(col);
        }
        fscanf(fid,"%[^\n]\n",col);// read last one
        this->pi[idxk][i] = atof(col);
        (*line_no)++;
        // read A
        fscanf(fid,"A\t");
        for(i=0; i<this->p->nS; i++)
            for(j=0; j<this->p->nS; j++) {
                if(i==(this->p->nS-1) && j==(this->p->nS-1)) {
                    fscanf(fid,"%[^\n]\n", col); // last one;
                    this->A[idxk][i][j] = atof(col);
                }
                else {
                    fscanf(fid,"%[^\t]\t", col); // not las one
                    this->A[idxk][i][j] = atof(col);
                }
            }
        (*line_no)++;
        // read B
        fscanf(fid,"B\t");
        for(i=0; i<this->p->nS; i++)
            for(m=0; m<this->p->nO; m++) {
                if(i==(this->p->nS-1) && m==(this->p->nO-1)) {
                    fscanf(fid,"%[^\n]\n", col); // last one;
                    this->B[idxk][i][m] = atof(col);
                }
                else {
                    fscanf(fid,"%[^\t]\t", col); // not last one
                    this->B[idxk][i][m] = atof(col);
                }
            }
        (*line_no)++;
    } // for all k
}