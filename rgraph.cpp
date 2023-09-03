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
    static optional<CSV> from_file(string filename) {
        smatch matches;
        if(regex_search(filename, matches, regex("([^/]+).csv$"))) {
            return optional(CSV(filename, matches[1].str()));
        } else {
            return optional<CSV>();
        }
    }

    void plot(UshR &rsession) {
        rsession.cmd() << "n <- nrow(" << this->name << ")*" << this->range_size << ";" << endl
                       << "p <- nrow(" << this->name << ")*" << this->range_pos << ";" << endl
                       << "price_chart(squash(" << this->name << "[1:n+p,]," << chunks << "));" << endl;
    }

    CSV& change_resolution(float multiplier) {
        this->chunks *= multiplier;
        if(this->chunks < 10) this->chunks = 10;
        else if (this->chunks > 200) this->chunks = 200;
        return *this;
    }

    CSV& move(int chg) {
        float max = 1.0 - this->range_size;
        this->range_pos += chg * this->range_size;
        if(this->range_pos < 0.0) this->range_pos = 0.0;
        else if(this->range_pos > max) this->range_pos = max;
        return *this;
    }

    CSV& zoom(float multiplier) {
        float orig = this->range_size;
        this->range_size *= multiplier;
        if(this->range_size > 1) this->range_size = 1;

        this->range_pos += orig - this->range_size;
        return *this;
    }

    getter(name);
    getter(filename);

    protected:
    CSV(string filename, string name): filename(filename), name(name),
                                       chunks(60), range_size(1.0), range_pos(0.0)  {}

    string name;
    string filename;

    int chunks;
    float range_size;
    float range_pos;
};

typedef vector<CSV> CSVs;

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

class RGraph {
    public:
    RGraph(istrings csvs_begin, istrings csvs_end) : evs(DWMEvs("R Graphics")), plotnum(0) {
        rproc.source("helpers.R");

        for(istrings it = csvs_begin; it < csvs_end; it++) {
            if(auto csv = CSV::from_file(*it)) {
                this->data.push_back(csv.value());
                rproc.let_csv(csv.value().get_name(), csv.value().get_filename());
            } else {
                cerr << "Wrong csv filename: " << *it << endl;
            };
        }
    }

    void run() {
        dataset().plot(rproc);

        while(auto ev = this->evs.next()) {
            smatch matches;
            if(regex_search(ev.value(), matches, regex("key:([0-9a-f]+)/([0-9a-f]+)"))) {
                int code = (stoi(matches[1].str(), 0, 16) << 16)
                         + stoi(matches[2].str(), 0, 16);

                if(code == left_bracket) {
                    dataset().change_resolution(0.75)
                             .plot(rproc);
                } else if (code == right_bracket) {
                    dataset().change_resolution(1.25)
                             .plot(rproc);
                } else if (code == up) {
                    switch_set(1).plot(rproc);
                } else if (code == down) {
                    switch_set(-1).plot(rproc);
                } else if (code == plus) {
                    dataset().zoom(0.75).plot(rproc);
                } else if (code == minus) {
                    dataset().zoom(1.25).plot(rproc);
                } else if (code == left) {
                    dataset().move(-1).plot(rproc);
                } else if (code == right) {
                    dataset().move(1).plot(rproc);
                } 
                else
                    cout << "unknown key " << hex << code << endl;
            }
        }

        cout << "evs end" << endl;
    
        rproc.wait();
    }

    protected:
    CSV &dataset() { return data[plotnum]; }
    CSV &switch_set(int change) { 
        this->plotnum += change;
        if(this->plotnum < 0)
            this->plotnum = 0;
        else if(this->plotnum >= data.size())
            this->plotnum = data.size() - 1;

        return dataset();
    }

    UshR rproc;
    CSVs data;
    DWMEvs evs;

    int plotnum;

    static const int left_bracket = 0x5b0000;
    static const int right_bracket = 0x5d0000;
    static const int up = 0xff520000;
    static const int down = 0xff540000;
    static const int left = 0xff510000;
    static const int right = 0xff530000;
    static const int plus = 0x3d0001;
    static const int minus = 0x2d0000;
};

int main(int argc, char* argv[]) {
    Args arg(argc,argv);

    arg.runIf([] (istrings begin, istrings end) {
        RGraph graph(begin, end);
        graph.run();
    });
}
