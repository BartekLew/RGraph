#include <iostream>
#include <vector>
#include <iterator>
#include <regex>
#include <optional>
#include <fmt/core.h>
#include <boost/process.hpp>
#include <sys/stat.h>

using namespace std;
using namespace boost::process;

typedef vector<string> strings;
typedef strings::iterator istrings;

class UshR {
    public:
    UshR() {
        this->proc = child("ush R");
        this->input = get_input(this->proc.id());
    }

    ~UshR() {
        this->input.close();
    }

    ofstream& cmd() { return this->input; };

    void wait() { this->proc.wait(); }

    void source(string file) { 
        this->input << "source('" << file << "');" << endl;
    }

    void let_csv(string var, string fname) {
        this->input << var << " <- read.csv('" << fname << "');" << endl;
    }
     
    private:
    ofstream get_input(int pid) {
        string ushin = fmt::format("/tmp/ush-{}", pid);
        struct stat buffer;

        while(stat(ushin.c_str(), &buffer) != 0);

        ofstream rcmd;
        rcmd.open(ushin);
        return rcmd;
    }

    protected:

    child    proc;
    ofstream input;
};

class Args {
    public:
    Args(int argc, char* argv[]) {
        this->args = strings(argc-1);

        for(int i = 1; i < argc; i++) this->args[i-1] = string(argv[i]);
    }

    template<class F>
    void runIf(F f) {
        if(args.size()) f(args.begin(), args.end());
    }

    protected:
    strings args;
};

#define getter(FIELD) auto get_##FIELD() { return this->FIELD; };

class CSV {
    public:
    CSV(string filename): filename(filename) {
        regex name_re("([^/]+).csv$");
        smatch matches;

        if(regex_search(filename, matches, name_re)) {
            this->name = matches[1].str();
        } else {
            this->name = "??";
        }
    }

    void plot(UshR &rsession) {
        rsession.cmd() << "price_chart(" << this->name << ");" << endl;
    }

    getter(name);
    getter(filename);

    protected:
    string name;
    string filename;
};

class DWMEvs {
    public:
    DWMEvs(string prefix) {
        cout << "Init dwmevs" << endl;
        ofstream cmd;
        cmd.open("/tmp/dwm.cmd");
        cmd << "g" << prefix << endl;
        cmd.close();

        cout << "Cmd sent" << endl;
        
        this->outstream.open("/tmp/dwm.out");

        this->ready = false;
    }


    optional<string> next() {
        if(!this->ready) {
            string evfile;

            cout << "wait_for_stream" << endl;

            getline(this->outstream, evfile);
            this->outstream.close();

            cout << "Evfile: " << evfile << endl;
    
            this->outstream.open(evfile);
    
            cout << "stream ready" << endl;
            this->ready = true;
        }

        if (this->outstream.good()) {
            string msg;
            getline(this->outstream, msg);
            return optional(msg);
        } else {
            return optional<string>();
        }
    }

    protected:
    bool ready;
    ifstream outstream;
};

typedef vector<CSV> CSVs;
int main(int argc, char* argv[]) {
    Args arg(argc,argv);

    arg.runIf([] (istrings begin, istrings end) {
        UshR rproc;

        rproc.source("helpers.R");
        CSVs csvs = CSVs();
        for(istrings it = begin; it < end; it++) {
            CSV csv(*it);
            csvs.push_back(csv);
            rproc.let_csv(csv.get_name(), csv.get_filename());
        }

        DWMEvs evs("R Graphics");
        csvs[0].plot(rproc);

        while(auto ev = evs.next()) {
            cout << ev.value() << endl;
        }

        cout << "evs end" << endl;

        rproc.wait();
    });
}
