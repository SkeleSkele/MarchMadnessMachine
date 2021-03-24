#define _GNU_SOURCE
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <time.h>

typedef struct Config_s {
    enum {
        undefined,
        power538,
    } system;
    int values[6];
} Config;

typedef struct Bracket_s {
    enum {
        unused,
        used,
        rStatus,
    } status;
    char* name;
    char winners[6][32];     // 1 byte integers, basically
    // 0: Ro64 winners
    // 1: Ro32 winners
    // 2: Ro16 winners
    // 3: Ro8 winners
    // 4: Ro4 winners
    // 5: Champion
    int baseScore;
    int tempScore;
} Bracket;

typedef struct Team_s {
    char* name;
    int rating;
    int index;
} Team;

typedef struct SimResults_s {
    char* names[20];
    int numBrackets;
    int baseScores[20];
    long int totalScores[20];
    int totalWins[20];
    int canTies[20];
    int numTies;
    int numTrials;
} SimResults;

void panic(char* msg, char* line) {
    // {{{
    // Print the error message and kill the program
    fprintf(stderr, "%s: %s\n", msg, line);
    exit(1);
} // }}}

void printBracket(Bracket* b, Team** teams) {
    // {{{
    // Prints out the contents of a bracket.
    printf("Name: ");
    if (b->name == NULL)
        printf("NULL\n");
    else
        printf("%s\n", b->name);
    printf("Status: ");
    switch(b->status) {
        case unused:
            printf("unused\n");
            break;
        case used:
            printf("used\n");
            break;
        case rStatus:
            printf("results\n");
            break;
    }
    printf("Base Score: %d\n", b->baseScore);
    printf("Temp Score: %d\n", b->tempScore);
    for (int i = 0; i < 6; i++) {
        printf("Round %d Winners:\n", i);
        int sz = 1 << (5 - i);
        for (int j = 0; j < sz; j++) {
            printf("%d. ", j);
            int t = b->winners[i][j];
            if (t == -1)
                printf("TBD\n");
            else
                printf("%s\n", teams[t]->name);
        }
    }
    printf("\n");
} // }}}

void printSimResults(SimResults* sr) {
    // {{{
    // Prints the results of the SimResults to the console. The 
    printf("RESULTS\n");
    for (int i = 0; i < sr->numBrackets; i++) {
        double e = (double)sr->totalScores[i] / (double)sr->numTrials;
        double p = 100.0 * (double)sr->totalWins[i] / (double)sr->numTrials;
        printf("Name: %s\n", sr->names[i]);
        printf("Base score: %d\n", sr->baseScores[i]);
        printf("Expected Final Score: %f\n", e);
        printf("Win probability: %f%%\n", p);
        printf("Can tie: %d\n\n", sr->canTies[i]);
    }
    printf("Ties: %d\n", sr->numTies);
} // }}}

int lookupTeam(char* name, Team** teams) {
    //{{{
    // Returns the index of the Team with the given name, or -1 on failure.
    for (int i = 0; i < 64; i++) {
        if (strcmp(name, teams[i]->name) == 0) {
            return i;
        }
    }
    panic("Unable to find team", name);
    return -1;
}//}}}

Team** allocTeams() {
    // {{{
    // Allocates an array of 64 Teams. Teams are initialized to {NULL, -1}.
    Team** teams = malloc(64 * sizeof(Team*));
    for (int i = 0; i < 64; i++) {
        Team* t = malloc(sizeof(Team));
        t->name = NULL;
        t->rating = -1;
        t->index = i;
        teams[i] = t;
    }
    return teams;
} // }}}

Bracket* allocBracket() {
    //{{{
    // Allocates memory for a Bracket. Initializes name to NULL, status to
    // unused, and all winners to -1.
    Bracket* bracket = malloc(sizeof(Bracket));
    bracket->baseScore = 0;
    bracket->tempScore = 0;
    bracket->status = unused;
    bracket->name = NULL;
    for (int i = 0; i < 6; i++) {
        int sz = 1 << (5 - i);
        for (int j = 0; j < sz; j++) {
            bracket->winners[i][j] = -1;
        }
    }
    return bracket;
}
//}}}

Bracket** allocPreds() {
    // {{{
    // Allocate and inits an array of 20 Brackets.
    Bracket** preds = malloc(20 * sizeof(Bracket*));
    for (int i = 0; i < 20; i++) {
        preds[i] = allocBracket();
    }
    return preds;
}
// }}}

SimResults* allocSimResults() {
    // {{{
    // Allocates and initializes a SimResults.
    SimResults* sr = malloc(sizeof(SimResults));
    sr->numBrackets = 0;
    for (int i = 0; i < 20; i++) {
        sr->names[i] = NULL;
        sr->totalScores[i] = 0;
        sr->totalWins[i] = 0;
        sr->baseScores[i] = 0;
        sr->canTies[i] = 0;
    }
    sr->numTies = 0;
    sr->numTrials = 0;
    return sr;
} // }}}

char* trimSpace(char* str) {
    //{{{
    // Returns the string with leading and trailing whitespace removed.
    // IMPORTANT:
    // 1. This will mutilate the original string.
    // 2. The original pointer must still be freed -- DON'T free the pointer that
    // is returned form this function!

    // Move pointer up until first non-space is found
    while(isspace((unsigned char)*str)) {
        str++;
    }

    // Check if the entire string was whitespace
    if (*str == '\0') {
        return str;
    }

    // Do the same thing but for a pointer to the back
    char* end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) {
        end--;
    }

    // Write new null character
    end[1] = '\0';

    return str;
}
/*}}}*/

double getRandomDecimal() {
    // {{{
    // Returns a double between 0.0 and 1.0 uniformly at random.
    return (double)rand() / (double)RAND_MAX;
} //}}}

double getWinProb538(int a, int b) {
    // {{{
    // Given teams with ratings a and b, returns the probability that team a wins
    // according to 538's power rating model.
    double ad = ((double)a)/10.0;
    double bd = ((double)b)/10.0;
    double exp = (bd - ad) * 30.464 / 400.0;
    return 1.0 / (1.0 + pow(10, exp));
} // }}}

double getWinProb(Config* cfg, int a, int b) {
    // {{{
    // Returns the probability that the team with rating a beats the team with
    // rating b, according to the system specific by cfg
    switch(cfg->system) {
        case power538:
            return getWinProb538(a, b);
        default:
            panic("Probability model undefined","");
    }
    return -1.0;
} // }}}

int parseInputFile(char* fileName, Team** teams, Bracket** preds, 
        Config* cfg, Bracket* results) {
    /*{{{*/
    // Attempts to parse the input file and populate preds and cfg. On success,
    // returns the number of brackets that were parsed.
    // Open file for input
    FILE* f = fopen(fileName, "r");
    if (f == NULL) {
        panic("Error opening input file", fileName);
    }

    // used by getline
    char* line = NULL;
    size_t linesz = 0;

    // keeps track of what we must parse next
    int c = 1;

    // the next free index of the preds array.
    int b = 0;

    while (getline(&line, &linesz, f) != -1) {
        // Remove leading and trailing white space
        char* cleanLine = trimSpace(line);
        // Ignore empty lines or lines starting with #
        if (strlen(cleanLine) == 0) {
            continue;
        }
        if (cleanLine[0] == '#') {
            continue;
        }

        //// PARSE STUFF BASED ON c ////
        // LINE 1: Rating System
        if (c == 1) {
            if (strcmp(cleanLine, "538power") == 0) {
                cfg->system = power538;
                c++;
                continue;
            }
            else {
                panic("Unrecognized rating system", cleanLine);
            }
        }

        // LINE 2: Scoring System
        if (c == 2) {
            for (int i = 0; i < 6; i++) {
                char* value;
                if (i == 0)
                    value = strtok(cleanLine, ";");
                else
                    value = strtok(NULL, ";");
                if (value == NULL)
                    panic("Bad parse on scoring system", 
                            "Need 6 comma-separated integers.");
                cfg->values[i] = atoi(value);
            }
            c++;
            continue;
        }

        // LINE 3 - 66: Load Teams
        if (c >= 3 && c <= 66) {
            Team* t = teams[c-3];
            char* teamName = strtok(cleanLine, ";");
            if (teamName == NULL)
                panic("Bad parse on team", "Missing name before ;?");
            t->name = strdup(teamName);
            char* rating = strtok(NULL, ";");
            if (rating == NULL)
                panic("Bad parse on team", teamName);
            t->rating = atoi(rating);
            teams[c-3] = t;
            c++;
            continue;
        }

        // LINE 67 - 72: Load results
        if (c >= 67 && c <= 72) {
            int numTeams = 1 << (72 - c);
            for (int i = 0; i < numTeams; i++) {
                char* teamName;
                if (i == 0)
                    teamName = strtok(cleanLine, ";");
                else
                    teamName = strtok(NULL, ";");
                if (teamName == NULL)
                    panic("Bad parse on results line", "check ;s");
                if (strcmp(teamName, "_") == 0)
                    continue;

                // Determine the index that this team should be put in
                int tIndex = lookupTeam(teamName, teams);
                int mIndex = tIndex / (1 << (c - 66));
                results->winners[c-67][mIndex] = tIndex;
            }
            c++;
            continue;
        }

        // From here on, we will load an unknown amount of people's prediction
        // brackets.
        if (c == 73) {
            if (b >= 20)
                panic("Too many brackets", "Maximum supported is 20");
            Bracket* bracket = preds[b];
            bracket->name = strdup(cleanLine);
            bracket->status = used;
            int round = 0;
            while (round < 6) {
                if (getline(&line, &linesz, f) == -1)
                    panic("Ran out of lines for bracket", bracket->name);
                cleanLine = trimSpace(line);
                if (strlen(cleanLine) == 0)
                    continue;
                if (cleanLine[0] == '#')
                    continue;

                int sz = 1 << (5 - round);
                for (int i = 0; i < sz; i++) {
                    char* teamName;
                    if (i == 0)
                        teamName = strtok(cleanLine, ";");
                    else
                        teamName = strtok(NULL, ";");
                    if (teamName == NULL)
                        panic("Error parsing line in bracket", bracket->name);

                    // Lookup the team's index, since it might not be in order
                    int tIndex = lookupTeam(teamName, teams);
                    int mIndex = tIndex / (1 << (round + 1));
                    bracket->winners[round][mIndex] = tIndex;

                    // Check if this pick is already in results
                    if (tIndex == results->winners[round][mIndex]) {
                        bracket->baseScore += cfg->values[round];
                    }
                }
                round++;
            }
            b++;
        }
    }

    free(line);
    return b;
} // }}}

void runTrial(Team** teams, Bracket** preds, Config* cfg, Bracket* results) {
    // {{{
    // Simulates a tournament. The tempScore field of each Bracket will 
    // contain the final score for this trial.
    Bracket trial = *results;
    // Reset tempScores back to baseScore.
    int numBrackets;
    for (int i = 0; i < 20; i++) {
        if (preds[i]->status != used) {
            numBrackets = i;
            break;
        }
        preds[i]->tempScore = preds[i]->baseScore;
    }

    // Go round-by-round and replace any -1 entries with a team from the
    // previous round
    for (int round = 0; round < 6; round++) {
        int sz = 1 << (5 - round);
        for (int i = 0; i < sz; i++) {
            if (trial.winners[round][i] == -1) {
                // For first round, the competing teams are just i*2, i*2 + 1
                int t1, t2;
                if (round == 0) {
                    t1 = i*2;
                    t2 = i*2 + 1;
                }
                else {
                    t1 = trial.winners[round-1][i*2];
                    t2 = trial.winners[round-1][i*2 + 1];
                }
                int r1 = teams[t1]->rating;
                int r2 = teams[t2]->rating;
                double p = getWinProb(cfg, r1, r2);
                int winner;
                if (getRandomDecimal() <= p) {
                    winner = t1;
                }
                else {
                    winner = t2;
                }
                trial.winners[round][i] = winner;

                // Update the score of anyone who got this pick
                for (int b = 0; b < numBrackets; b++) 
                    if (preds[b]->winners[round][i] == winner) {
                        preds[b]->tempScore += cfg->values[round];
                    }
            }
        }
    }
} // }}}

SimResults* runTrials(int n, Team** teams, Bracket** preds, Config* cfg, 
        Bracket* results, int interrupt) {
    // {{{
    clock_t start = clock();
    SimResults* sr = allocSimResults();

    // Count number of brackets and set baseScores, names
    int numBrackets;
    for (int i = 0; i < 20; i++) {
        if (preds[i]->status != used) {
            numBrackets = i;
            break;
        }
        sr->names[i] = strdup(preds[i]->name);
        sr->baseScores[i] = preds[i]->baseScore;
    }

    sr->numTrials = n;
    sr->numBrackets = numBrackets;

    // Will contain total cumulative scores for each prediction bracket
    for (int i = 0; i < n; i++) {
        runTrial(teams, preds, cfg, results);

        // Add the scores to cumulative totals
        int max = 0;
        int tieFlag = 1; // set to 1 if tie, 0 if no tie
        for (int b = 0; b < numBrackets; b++) {
            int score = preds[b]->tempScore;
            if (score > max) {
                max = score;
                tieFlag = 0;
            }
            else if (score == max)
                tieFlag = 1;
            sr->totalScores[b] += score;
        }

        for (int b = 0; b < numBrackets; b++) {
            if (preds[b]->tempScore == max) {
                if (tieFlag == 0) {
                    sr->totalWins[b]++;
                    break;
                }
                else
                    sr->canTies[b] = 1;
            }
        }
        sr->numTies += tieFlag;

        // End early if we have exceeded the interrupt time
        if (interrupt > 0) {
            clock_t end = clock();
            double elapsedTime = ((double)(end - start)) / CLOCKS_PER_SEC;
            if (elapsedTime > interrupt) {
                sr->numTrials = i + 1;
                return sr;
            }
        }
    }

    return sr;
} // }}}

void printToCSV(char* fileName, SimResults* sr) {
    // {{{
    FILE* f = fopen(fileName, "w");
    if (f == NULL)
        panic("Error opening file for writing", fileName);

    // Row 1: first cell blank, then the names of brackets
    fprintf(f, ",");
    for (int i = 0; i < sr->numBrackets; i++) {
        fprintf(f, "%s,", sr->names[i]);
    }
    // Row 2: first cell "Score", then base scores
    fprintf(f, "\nScore,");
    for (int i = 0; i < sr->numBrackets; i++) {
        fprintf(f, "%d,", sr->baseScores[i]);
    }
    // Row 3: Expected Final Scores
    fprintf(f, "\nExpected Final Score,");
    for (int i = 0; i < sr->numBrackets; i++) {
        double e = (double)sr->totalScores[i] / (double)sr->numTrials;
        fprintf(f, "%f,", e);
    }
    fprintf(f, "Ties\n");

    // Row 4: Probability of winning
    fprintf(f, "Chance to Win,");
    for (int i = 0; i < sr->numBrackets; i++) {
        double e = (double)sr->totalWins[i] / (double)sr->numTrials;
        fprintf(f, "%f,", e);
    }
    double t = (double)sr->numTies / (double)sr->numTrials;
    fprintf(f, "%f,", t);

    // Row 5: Can win in tie
    fprintf(f, "\nCan Win in Tie,");
    for (int i = 0; i < sr->numBrackets; i++) {
        if (sr->canTies[i] == 1)
            fprintf(f, "Yes,");
        else
            fprintf(f, "No,");
    }
    fclose(f);
    return;
} // }}}

int main(int argc, char* argv[]) {
    clock_t start = clock();

    // Parse command line arguments.
    extern char* optarg;
    extern int optind;
    int c = 0;

    char* goodOptions = "vi:t:o:";

    int numTrials = 1000000;
    char* outFileName = "results.csv";
    int verboseFlag = 0;
    int interrupt = 0;

    while ((c = getopt(argc, argv, goodOptions)) != -1) {
        switch(c) {
            case '?':
                panic("Unrecognized command line option. Use", goodOptions);
                break;
            case 't':
                numTrials = atoi(optarg);
                break;
            case 'o':
                outFileName = strdup(optarg);
                break;
            case 'v':
                verboseFlag = 1;
                break;
            case 'i':
                interrupt = atoi(optarg);
                break;
        }
    }

    // Should be 1 left over argument for the filename
    if (optind != argc - 1) {
        panic("Bad usage", "./mmm2 inputfilename.txt");
    }
    char* inFileName = argv[optind];

    // An array to store the teams. 64 teams total.
    Team** teams = allocTeams();

    // An array of people's prediction brackets. Maximum 20!
    Bracket** preds = allocPreds();

    // Contains miscellaneous settings.
    Config* cfg = malloc(sizeof(Config));
    cfg->system = undefined;

    // One bracket to store the results of the tournament.
    Bracket* results = allocBracket();
    results->status = rStatus;
    results->name = "Results";

    if (verboseFlag)
        printf("Parsing input file %s...\n", inFileName);
    parseInputFile(inFileName, teams, preds, cfg, results);

    // Set rng seed
    srand(time(NULL));

    if (verboseFlag)
        printf("Running %d trials...\n", numTrials);

    SimResults* sr = 
        runTrials(numTrials, teams, preds, cfg, results, interrupt);

    if (verboseFlag)
        printf("Completed %d trials.\n", sr->numTrials);

    // Write results
    if (verboseFlag)
        printf("Writing results to %s...\n", outFileName);
    printToCSV(outFileName, sr);

    if (verboseFlag) {
        printf("\n");
        printSimResults(sr);
    }

    clock_t end = clock();
    printf("Done!\n");
    double elapsedTime = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time elapsed: %f seconds.\n", elapsedTime);
}
