#pragma once

#include <map>
#include <string>
#include <vector>

struct NetlistTopology;

// option parameters class — parses and serializes Xyce .OPTIONS directives
class OptionParameters
{
public:
    // construct an option parameters instance from individual fields
    OptionParameters(std::map<std::string, std::string> device, std::map<std::string, std::string> timeint, std::map<std::string, std::string> nonlin, std::map<std::string, std::string> linsol, std::map<std::string, std::string> fft);

    // parse all .OPTIONS directives into an OptionParameters instance
    [[nodiscard]] static OptionParameters from_xyce_directives(const std::vector<std::string>& directives);

    // serialize this instance to a list of Xyce directive strings
    [[nodiscard]] std::vector<std::string> to_xyce_directives(const NetlistTopology& topology) const;

    // equality operator
    [[nodiscard]] bool operator==(const OptionParameters& other) const;

    // top-level device options applied across simulations
    std::map<std::string, std::string> device;
    // transient integration control parameters
    std::map<std::string, std::string> timeint;
    // generic nonlinear solver parameters
    std::map<std::string, std::string> nonlin;
    // generic linear solver parameters
    std::map<std::string, std::string> linsol;
    // options controlling all .FFT statements (FFT_ACCURATE, FFTOUT, FFT_MODE)
    std::map<std::string, std::string> fft;
};
