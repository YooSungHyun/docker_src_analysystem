//
//  InputUtil.h
//  This is a support class that helps read input text for HMM, convert it
//  to a more compact binary file, and read the binary file as well
//  HMM

#ifndef __HMM__InputUtil__
#define __HMM__InputUtil__

#include "utils.h"

//#define bin_input_file_verstion 1
//#define bin_input_file_verstion 2 // increase number of skills/students to a 4 byte integer
#define bin_input_file_verstion 3 // added Nstacked, changed how multi-skills are stored and added slices (single and multi-coded)

class InputUtil {
public:
    // static bool readTxt(const char *fn, struct param * param); // read txt into param
    static bool readTxt(std::string data, struct param * param); // read txt into param
    static bool readBin(const char *fn, struct param * param); // read bin into param
    static bool toBin(struct param * param, const char *fn);// writes data in param to bin file
    // experimental
    static void writeInputMatrix(const char *filename, struct param* p, NCAT xndat, struct data** x_data);
private:
    static void writeString(FILE *f, string str);
    static string readString(FILE *f);
};
#endif /* defined(__HMM__InputUtil__) */
