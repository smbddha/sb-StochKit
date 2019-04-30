
# Stochastic Toolkit
A few VCV Rack modules building off of Xenakis's stochastic synthesis and GENDYN program.

# Manual

## GRANDY
A stochastic synthesis generator. Grandy implements an extended version of Xenakis's Dynamic Stochastic Synthesis coined Granular Dynamic Stochastic Synthesis due to the added synchronous granular synthesis twist. All knob controls can be controlled by +/-5 CV.

### Features / Controls
**bpts** -> vary the number of breakpoints used in the synthesis \
**astp** -> maximum step that a breakpoint's amplitude value can take \
**dstp** -> maximum step that a breakpoint's duration value can take \
**pdst** -> change the probability distribution used to generate all step values. l - LINEAR, c - CAUCHY, a - ARCSIN \
**mirr** -> toggle between the wrapping and mirroring of breakpoints if they surpass amplitude or duration bounds 

#### sine mode
**gfreq** -> control frequency of the sin wave that is granulated if in sine wave mode

#### fm mode
**fcar** -> frequency of the carrier wave \
**fmod** -> frequency of the modulating wave \
**imod** -> index of modulation 

## STITCHER
A module for stochastic concatenation. Controls for four seperate GRANDY oscillators on the left and global controls on the right.

### Features / Controls
#### local
**f** -> oscillator frequency \
**b** -> number of breakpoints \
**a** -> maximum amplitude step \
**d** -> maximum duration step \
**stutter** -> number of cycles to output 

#### global
All global controls are -1 - +1 and affect all oscillators.

## GenECHO
A module for stochastic 'decomposition' ... make of it what you will

### Features / Controls
**l** -> length of sample \
**i** -> input for sample signal \
**g** -> on gate will sample incoming signal to **i** \
**acc** -> toggle breakpoint accumulation \
**mirr** -> same as for GRANDY \
**pdst** -> same as for GRANDY \
**bpts** -> affect spacing between breakpoints / number of breakpoints \
**astp** -> same as GRANDY \
**dstp** -> same as GRANDY \
**env** -> same as GRANDY 

# Questions or Comments?
