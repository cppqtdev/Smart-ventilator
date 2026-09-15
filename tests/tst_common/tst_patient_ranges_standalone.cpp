// Exercises the real bound arithmetic from VentilatorController, transcribed
// verbatim so it can run without Qt.
#include <cstdio>
#include <string>
#include <algorithm>

static int qMax(int a, int b) { return a > b ? a : b; }
static int qMin(int a, int b) { return a < b ? a : b; }

struct C {
    std::string cat; int ibw;
    int ceilingVt() const {
        if (cat=="Neonatal")  return qMin(80,  qMax(12,  ibw*8));
        if (cat=="Pediatric") return qMin(500, qMax(40,  ibw*10));
        return qMin(900, qMax(160, ibw*10));
    }
    int minVt() const {
        const int byWeight = ibw * (cat=="Pediatric" ? 5 : 4);
        int floorMl = 150;
        if (cat=="Neonatal") floorMl = 10;
        else if (cat=="Pediatric") floorMl = 30;
        return qMin(qMax(floorMl, byWeight), ceilingVt());
    }
    int maxVt() const { return qMax(minVt(), ceilingVt()); }
    int ceilingRr() const { return cat=="Neonatal" ? 80 : (cat=="Pediatric" ? 50 : 35); }
    int minRr() const { return cat=="Neonatal" ? 20 : (cat=="Pediatric" ? 10 : 4); }
    int maxRr() const { return qMax(minRr(), ceilingRr()); }
};

int main() {
    const char *cats[] = {"Adult","Pediatric","Neonatal"};
    int bad = 0, checked = 0;
    for (const char *c : cats) {
        for (int ibw = 1; ibw <= 180; ++ibw) {
            C k{c, ibw};
            ++checked;
            if (k.maxVt() < k.minVt()) {
                std::printf("  INVERTED Vt  %-10s ibw=%3d  min=%4d max=%4d\n", c, ibw, k.minVt(), k.maxVt());
                ++bad;
            }
            if (k.maxRr() < k.minRr()) {
                std::printf("  INVERTED Rr  %-10s ibw=%3d  min=%4d max=%4d\n", c, ibw, k.minRr(), k.maxRr());
                ++bad;
            }
            // The transient the start-up sync used to create: a category
            // changed while the previous weight is still loaded.
            for (const char *previous : cats) {
                C mixed{c, C{previous, 73}.ibw};
                if (mixed.maxVt() < mixed.minVt()) { std::printf("  INVERTED mixed %s/%s\n", c, previous); ++bad; }
            }
        }
    }
    std::printf("\n%d combinations checked, %d inverted range(s)\n", checked, bad);

    std::printf("\nsample bounds:\n");
    for (const char *c : cats)
        for (int ibw : {1, 3, 8, 20, 73, 180}) {
            C k{c, ibw};
            std::printf("  %-10s ibw=%3d  Vt %4d-%4d   Rr %3d-%3d\n",
                        c, ibw, k.minVt(), k.maxVt(), k.minRr(), k.maxRr());
        }
    return bad == 0 ? 0 : 1;
}
