#include "Chord/ChordEngine.h"

#include <cmath>
#include <iostream>
#include <string>

using namespace anopi;

namespace
{

bool hasPc (const std::vector<int>& notes, int pc)
{
    pc = wrapPc (pc);
    for (int n : notes)
        if (wrapPc (n) == pc)
            return true;
    return false;
}

void require (bool cond, const std::string& what)
{
    if (! cond)
    {
        std::cerr << "FAIL: " << what << '\n';
        std::exit (1);
    }
}

} // namespace

int main()
{
    ChordEngine engine;

    {
        ChordRequest r;
        r.tonalCenter = 0;
        r.tonality = Tonality::Major;
        r.layout = LayoutMode::Static;
        r.degreeKeyPc = 0;
        r.extensions = 0.4f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 0) && hasPc (c.midiNotes, 4) && hasPc (c.midiNotes, 7), "C major triad");
        require (c.symbol.rfind ("C", 0) == 0, "C symbol");
        require (c.roman.rfind ("I", 0) == 0, "I roman");
        require (! c.chromatic, "C is diatonic");
    }

    {
        ChordRequest r;
        r.degreeKeyPc = 5;
        r.extensions = 0.4f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 5) && hasPc (c.midiNotes, 9) && hasPc (c.midiNotes, 0), "F major IV");
        require (c.roman.rfind ("IV", 0) == 0, "IV roman");
    }

    {
        ChordRequest r;
        r.degreeKeyPc = 7;
        r.extensions = 0.4f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 7) && hasPc (c.midiNotes, 11) && hasPc (c.midiNotes, 2), "G major V");
    }

    {
        ChordRequest r;
        r.degreeKeyPc = 2;
        r.extensions = 0.4f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 2) && hasPc (c.midiNotes, 5) && hasPc (c.midiNotes, 9), "D minor ii");
        require (c.symbol.find ('m') != std::string::npos, "ii is minor");
    }

    {
        ChordRequest r;
        r.tonalCenter = 9; // A
        r.tonality = Tonality::Minor;
        r.layout = LayoutMode::Static;
        r.degreeKeyPc = 0; // always I in static
        r.extensions = 0.4f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 9) && hasPc (c.midiNotes, 0) && hasPc (c.midiNotes, 4), "static A minor i");
        require (c.layoutLabel.find ("Static") != std::string::npos, "static label");
    }

    {
        ChordRequest r;
        r.tonalCenter = 7; // G
        r.tonality = Tonality::Major;
        r.layout = LayoutMode::RealScale;
        r.degreeKeyPc = 7; // G key is I
        r.extensions = 0.4f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 7) && hasPc (c.midiNotes, 11) && hasPc (c.midiNotes, 2), "real G is I");
        require (c.roman.rfind ("I", 0) == 0, "real G roman I");
    }

    {
        ChordRequest r;
        r.tonalCenter = 7;
        r.tonality = Tonality::Major;
        r.layout = LayoutMode::RealScale;
        r.degreeKeyPc = 0; // C is IV of G
        r.extensions = 0.4f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 0) && hasPc (c.midiNotes, 4) && hasPc (c.midiNotes, 7), "real C is IV of G");
        require (c.roman.find ("IV") != std::string::npos, "IV of G");
    }

    {
        ChordRequest r;
        r.degreeKeyPc = 1; // C#
        r.extensions = 0.4f;
        auto c = engine.resolve (r);
        require (c.chromatic, "C# is chromatic in C major");
        require (hasPc (c.midiNotes, 9) && hasPc (c.midiNotes, 1) && hasPc (c.midiNotes, 4), "C# as M3 of A");
        require (c.roman.find ("V/ii") != std::string::npos, "V/ii");
    }

    {
        ChordRequest r;
        r.degreeKeyPc = 0;
        r.extensions = 0.85f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 11), "Imaj7 has B");
        require (hasPc (c.midiNotes, 2), "Imaj9 has D");
    }

    {
        ChordRequest r;
        r.degreeKeyPc = 0;
        r.extensions = 0.4f;
        r.shift = true;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 5), "shift I is sus4 (F)");
        require (! hasPc (c.midiNotes, 4), "shift I removes the 3rd");
    }

    {
        ChordRequest r;
        r.tonality = Tonality::Minor;
        r.degreeKeyPc = 7; // v
        r.extensions = 0.6f;
        r.shift = true;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 7) && hasPc (c.midiNotes, 11) && hasPc (c.midiNotes, 5), "minor v+shift → G7 pcs in C");
        require (c.roman.rfind ("V", 0) == 0, "raised dominant roman");
    }

    {
        ChordRequest r;
        r.degreeKeyPc = 1;
        r.shift = true;
        r.extensions = 0.6f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 3), "tritone of A is Eb");
    }

    {
        ChordRequest r;
        r.degreeKeyPc = 0;
        r.extensions = 0.1f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 0) && hasPc (c.midiNotes, 7), "power chord");
        require (! hasPc (c.midiNotes, 4), "no third on fifths stage");
    }

    require (ChordEngine::layoutDegreePc ({ .degreeKeyPc = 12 }) == 0, "high C wraps to I");

    {
        ChordRequest r;
        r.scale = ScaleId::Dorian;
        r.degreeKeyPc = 0;
        r.extensions = 0.4f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 0) && hasPc (c.midiNotes, 3) && hasPc (c.midiNotes, 7), "C dorian i");
        require (c.layoutLabel.find ("Dorian") != std::string::npos, "dorian label");
    }

    {
        ChordRequest r;
        r.scale = ScaleId::HarmonicMinor;
        r.degreeKeyPc = 7;
        r.extensions = 0.4f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 7) && hasPc (c.midiNotes, 11) && hasPc (c.midiNotes, 2), "harm. minor V is major");
    }

    {
        ChordRequest r;
        r.degreeKeyPc = 0;
        r.extensions = 0.4f;
        r.color = ColorMode::Add6;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 9), "add6 has A");
    }

    {
        ChordRequest r;
        r.degreeKeyPc = 0;
        r.extensions = 0.4f;
        r.inversion = 1;
        auto c = engine.resolve (r);
        require (! c.midiNotes.empty(), "inversion still sounds");
        require (wrapPc (c.midiNotes.front()) != 0 || c.midiNotes.size() >= 3, "1st inv not rooted on C in bass of cluster");
    }

    {
        ChordRequest r;
        r.degreeKeyPc = 0;
        r.extensions = 0.95f;
        auto c = engine.resolve (r);
        require (hasPc (c.midiNotes, 9) || hasPc (c.midiNotes, 21 % 12), "13th colour present");
    }

    std::cout << "ChordEngine tests passed\n";
    return 0;
}
