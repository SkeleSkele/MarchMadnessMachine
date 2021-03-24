# MarchMadnessMachine
This is a program that will approximate the winning probabilities of different people in a March Madness bracket pool. This is achieved by running Monte Carlo simulations of the tournament from any given point and recording the results.

# How It Works
MMM relies on an input text file which contains the following things:
 1. The points awarded for correct predictions for each round.
 2. The initial tournament bracket.
 3. An integer skill rating for each team with a specified rating system.
 4. A series of prediction brackets for each person in the pool.
 5. The results of the tournament at present.

The program can be run like so:
```./mmmc -t 10000000 -i 45 -v input.txt```

The input file name is the only required argument. There are four optional flags:
-t : Specifies the number of trials to run. (1,000,000 if unspecified.)
-i : Specifies a time limit in seconds for the simulation. If the time limit is exceeded before the number of trials has been met, the simulation will end early. (No time limit if unspecified.)
-o : Specifies the result file name. (Default is results.csv.)
-v : Runs in verbose mode.

The program's output is a .csv file which shows each of the following for each person in the pool:
1. Their current score.
2. Their average final score across all simulations.
3. Their probability of winning the pool (in other words, the fraction of trials in the simulation which they won).
4. Whether or not that person can win in a tie.

The probability of a tie is also given.

# Rating Systems
Currently the only supported rating system is that used by FiveThirtyEight, which is outlined here. https://fivethirtyeight.com/features/how-our-march-madness-predictions-work-2/

Note however that MMM uses integer ratings and expects the FiveThirtyEight ratings to be multiplied by 10.
