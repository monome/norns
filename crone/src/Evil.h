//
// Created by emb on 11/23/18.
//

// mwwa haha

#ifndef CRONE_EVIL_H
#define CRONE_EVIL_H

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include "faust/gui/APIUI.h"

namespace crone {
class Evil  {
public:
    struct FaustModule {
        std::string name;
        APIUI *ui;
        std::string upname;
        std::string lowname;
        FaustModule(std::string s, APIUI *u): name(s), ui(u), upname(name), lowname(name) {
            boost::to_upper(upname);
            boost::to_lower(lowname);
        }

        // arg: param offset (params gen'd so far)
        // result: num params gen'd
        int genParamEnum(int poff=0) const {
            (void)poff;
            using std::string;
            using std::endl;
            std::ostringstream os;
            os << name << "Params.h";
            string ofname(os.str());
            std::ofstream ofs(ofname, std::ios::out | std::ios::trunc);
            ofs << "#pragma once" << endl << endl;
            ofs << "namespace crone { " << endl;
            ofs << "typedef enum { " << endl;
            int n = ui->getParamsCount();
            for (int i=0; i<n; ++i) {
                string lab(ui->getParamLabel(i));
                replace(lab.begin(), lab.end(), '/','_');
                boost::replace_all(lab, name, "");
                boost::replace_all(lab, "__", "");
                boost::to_upper(lab);
                ofs << lab << " = " << i << "," << endl;
            }
            ofs << "} " << name << "Param;" << endl;
            ofs << "} // namespace crone" << endl;
            ofs.close();
            return n;
        };

        void genOscLambda() const {
            using std::string;
            using std::endl;
            std::ostringstream os;
            os << name << "_param_lambdas.hpp";
            string ofname(os.str());
            std::ofstream ofs(ofname, std::ios::out | std::ios::trunc);
            for (int i=0; i<ui->getParamsCount(); ++i) {
                string lab(ui->getParamLabel(i));
                replace(lab.begin(), lab.end(), '/','_');
                boost::replace_all(lab, name, "");
                boost::replace_all(lab, "__", "");
                boost::to_lower(lab);
                ofs << "addServerMethod(\"/set/param/" << lowname << "/" << lab;
                //ofs << "\",\"f\". [](lo_arg **argv, int argc) {" << endl << "\t";

                ofs << R"evil(", "f", [](lo_arg **argv, int argc) {
        if(argc<1) { return; }
        Commands::post(Commands::Id::SET_PARAM_)evil";
                ofs << upname;
                boost::to_upper(lab);
                ofs << ", "<< name << "Param::" << lab;
                ofs << ", argv[0]->f);" << endl << "});" << endl << endl;
            }
            ofs.close();

        }
    };

    static void DO_EVIL(std::vector<FaustModule> modules) {

        using std::cout;
        using std::endl;
        using std::string;
        using std::vector;
        using std::ofstream;
        using std::replace;

        for(auto const &mod: modules ) {
            APIUI *ui = mod.ui;
            int n = ui->getParamsCount();
            cout << mod.name << ": " << endl;
            for (int i=0; i<n; ++i) {
                cout << "  " << i << ": " << ui->getParamLabel(i)
                     << " (" << ui->getParamAddress(i) << ")"
                     << " = " << ui->getParamValue(i) << endl;
            }

            n = 0;
            n += mod.genParamEnum(n);
            mod.genOscLambda();


        }
    }
};

}

#endif //CRONE_ACCESSGENERATOR_H
