
# Stochastic Toolkit
A few VCV Rack modules building off of Xenakis's stochastic synthesis and GENDYN program.

# Manual

## GRANDY
A stochastic synthesis generator. Grandy implements an extended version of Xenakis's Dynamic Stochastic Synthesis coined Granular Dynamic Stochastic Synthesis due to the added synchronous granular synthesis twist.

### Features / Controls
**bpts** -> vary the number of breakpoints used in the synthesis

**astp** -> maximum step that a breakpoint's amplitude value can take

**dstp** -> maximum step that a breakpoint's duration value can take

**pdst** -> change the probability distribution used to generate all step values. l - LINEAR, c - CAUCHY, a - ARCSIN

**mirr** -> toggle between the wrapping and mirroring of breakpoints if they surpass amplitude or duration bounds

#### sine mode
**gfreq** -> control frequency of the sin wave that is granulated if in sine wave mode

#### fm mode
**fcar** -> frequency of the carrier wave

**fmod** -> frequency of the modulating wave

**imod** -> index of modulation

## STITCHER
TODO

## GenECHO
TODO

## Stochastic STEPPER
TODO
