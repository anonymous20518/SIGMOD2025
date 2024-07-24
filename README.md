### Executing the code

Build the code using CMake and run it with the following input parameters:

1. k (clique relaxation)
2. g (group size, >=2k+1)
3. d (number of dimensions, 2-5)
4. dataset (4:YouTube, 5:case study, 10:LiveJournal, 11:DBLP, 12:Amazon, 13:WikiTalk, 14:CitPatent)
5. label distribution type (0: independent, 1: correlated, 2: anti-correlated)
6. algorithm (1: PKPlex, 2: Baseline)
7. [t] (number of threads if running PKPlex)