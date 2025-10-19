// cacheSim.cpp – 2-level inclusive cache simulator (fixed per spec)
// Build with: g++ -std=c++17 -O2 -Wall cacheSim.cpp -o cacheSim

#include <bits/stdc++.h>
using namespace std;

struct Line {
    bool valid = false;
    bool dirty = false;
    uint64_t tag = 0;
    unsigned lru = 0;   // smaller = more recently used
};

struct Cache {
    unsigned sets, ways, blockB;
    vector<vector<Line>> data;

    Cache(unsigned sizeLog2, unsigned assocLog2, unsigned blockLog2)
      : ways(1u<<assocLog2), blockB(blockLog2)
    {
        unsigned nblocks = 1u << (sizeLog2 - blockB);
        sets = nblocks / ways;
        data.assign(sets, vector<Line>(ways));
    }

    struct Res { bool hit; unsigned set; uint64_t tag; Line *ln; };

    Res probe(uint64_t addr) {
        uint64_t blk = addr >> blockB;
        unsigned idx = blk % sets;
        uint64_t t = blk / sets;
        auto &S = data[idx];
        for (auto &L: S)
            if (L.valid && L.tag == t)
                return {true, idx, t, &L};
        for (auto &L: S)
            if (!L.valid)
                return {false, idx, t, &L};
        auto victim = max_element(S.begin(), S.end(),
                       [&](auto &a, auto &b){ return a.lru < b.lru; });
        return {false, idx, t, &*victim};
    }

    void updateLRU(unsigned idx, Line *recent) {
        for (auto &L: data[idx]) L.lru++;
        recent->lru = 0;
    }
};

struct Simulator {
    unsigned memC, blkB, wrAlloc, tL1, tL2;
    Cache L1, L2;
    uint64_t nL1=0, mL1=0, nL2=0, mL2=0, cycles=0;

    Simulator(unsigned _m, unsigned _b, unsigned _w,
              unsigned s1, unsigned a1, unsigned c1,
              unsigned s2, unsigned a2, unsigned c2)
      : memC(_m), blkB(_b), wrAlloc(_w),
        tL1(c1), tL2(c2),
        L1(s1,a1,blkB), L2(s2,a2,blkB)
    {}

    // on L1 eviction of a dirty line, inclusive write-back into L2
    void writeBackL1(const Line &old, unsigned set1) {
        if (!old.valid || !old.dirty) return;
        uint64_t blk = old.tag * L1.sets + set1;
        uint64_t addr = blk << blkB;
        auto r2 = L2.probe(addr);
        if (!r2.hit) {
            Line ev = *r2.ln;
            if (ev.valid && ev.dirty) {
                // background write to memory
            }
            if (ev.valid)
                invalidateL1(ev.tag, r2.set);
            *r2.ln = {true, true, r2.tag, 0};
        } else {
            r2.ln->dirty = true;
        }
        L2.updateLRU(r2.set, r2.ln);
    }

    // invalidate a line in L1 corresponding to an L2-way eviction
    void invalidateL1(uint64_t tag2, unsigned set2) {
        uint64_t blk2 = tag2 * L2.sets + set2;
        unsigned s1 = blk2 % L1.sets;
        uint64_t t1 = blk2 / L1.sets;
        for (auto &L: L1.data[s1]) {
            if (L.valid && L.tag == t1) {
                L.valid = false;
                break;
            }
        }
    }

    void access(char op, uint64_t addr) {
        bool isW = (op=='w'||op=='W');
        nL1++;
        auto r1 = L1.probe(addr);
        if (r1.hit) {
            cycles += tL1;
            if (isW) r1.ln->dirty = true;
            L1.updateLRU(r1.set, r1.ln);
            return;
        }
        mL1++;

        if (isW && wrAlloc==0) {
            nL2++;
            auto r2 = L2.probe(addr);
            if (r2.hit) {
                cycles += tL1 + tL2;
                r2.ln->dirty = true;
                L2.updateLRU(r2.set, r2.ln);
            } else {
                mL2++;
                cycles += tL1 + tL2 + memC;
            }
            return;
        }

        nL2++;
        auto r2 = L2.probe(addr);
        if (r2.hit) {
            cycles += tL1 + tL2;
            writeBackL1(*r1.ln, r1.set);
            *r1.ln = {true, isW, r1.tag, 0};
            L1.updateLRU(r1.set, r1.ln);
            L2.updateLRU(r2.set, r2.ln);
        } else {
            mL2++;
            cycles += tL1 + tL2 + memC;
            Line ev2 = *r2.ln;
            if (ev2.valid) invalidateL1(ev2.tag, r2.set);
            *r2.ln = {true, false, r2.tag, 0};
            L2.updateLRU(r2.set, r2.ln);
            writeBackL1(*r1.ln, r1.set);
            *r1.ln = {true, isW, r1.tag, 0};
            L1.updateLRU(r1.set, r1.ln);
        }
    }

    void report() {
        double mr1 = double(mL1)/nL1;
        double mr2 = nL2 ? double(mL2)/nL2 : 0.0;
        double avg = double(cycles)/nL1;
        printf("L1miss=%.03f L2miss=%.03f AccTimeAvg=%.03f\n",
               mr1, mr2, avg);
    }
};

static inline string ltrim(const string &s) {
    size_t p = s.find_first_not_of(" \t\r\n");
    return p==string::npos ? "" : s.substr(p);
}

int main(int argc, char** argv) {
    if (argc < 20) {
        cerr << "Usage error\n";
        return 1;
    }
    string trace = argv[1];
    unsigned memC=0, bB=0, wA=0, l1S=0, a1=0, c1=0, l2S=0, a2=0, c2=0;
    for (int i = 2; i + 1 < argc; i += 2) {
        string k = argv[i]; unsigned v = atoi(argv[i+1]);
        if      (k == "--mem-cyc")  memC = v;
        else if (k == "--bsize")    bB   = v;
        else if (k == "--wr-alloc") wA   = v;
        else if (k == "--l1-size")  l1S  = v;
        else if (k == "--l1-assoc") a1   = v;
        else if (k == "--l1-cyc")   c1   = v;
        else if (k == "--l2-size")  l2S  = v;
        else if (k == "--l2-assoc") a2   = v;
        else if (k == "--l2-cyc")   c2   = v;
        else { cerr << "Bad arg " << k << "\n"; return 1; }
    }
    Simulator sim(memC, bB, wA, l1S, a1, c1, l2S, a2, c2);
    ifstream in(trace);
    if (!in) { cerr << "Cannot open " << trace << "\n"; return 1; }
    string line;
    while (getline(in, line)) {
        line = ltrim(line);
        if (line.empty()) continue;
        char op = 'r';
        if (strchr("rRwW", line[0])) { op = line[0]; line = line.substr(1); }
        line = ltrim(line);
        if (line.rfind("0x", 0) == 0 || line.rfind("0X", 0) == 0) line = line.substr(2);
        uint64_t addr = strtoull(line.c_str(), nullptr, 16);
        sim.access(op, addr);
    }
    sim.report();
    return 0;
}

