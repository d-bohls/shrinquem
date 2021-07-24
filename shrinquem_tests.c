// shrinquem - An algorithm for logic minimization.
// Copyright (C) 2021 Damon Bohls <damonbohls@gmail.com>
// MIT License: https://github.com/d-bohls/shrinquem/blob/main/LICENSE

#include <stdlib.h>
#include <stdio.h> // used for printf, ect.
#include <time.h> // used for time() to seed srand
#include <math.h> // used for floor
#include "shrinquem.h"

#if defined(_WIN32)

#include <windows.h>
static const char* unitsGetTickCount = "milliseconds";
#define GetTickCountForOS GetTickCount

#elif defined(__unix__) || defined(__linux__)

#include <errno.h>
#include <sys/time.h>
static const char* unitsGetTickCount = "microseconds";

static long GetTickCountForOS(void)
{
    struct timeval tv;
    unsigned long rc;

    rc = gettimeofday(&tv, (struct timezone*)0);
    if (rc)
    {
        fprintf(stderr, "gettimeofday: %s\n", strerror(errno));
        abort();
    }

    // Convert to microseconds.
    return tv.tv_sec * 1000000L + tv.tv_usec;
}

#endif

static void TestSimpleExample(void);
static void TestOneSpecificTruthTable(void);
static void TestOneRandomTruthTable(void);
static void TestEquationGeneration(void);
static void TestAllTruthTables(void);
static void TestAllTruthTablesWithOneFalse(void);
static void TestAllTruthTablesWithOneTrue(void);
static void TestSomeRandomTruthTables(void);
static void PrintEquation(const unsigned long numVars, const unsigned long numTerms, const unsigned long terms[], const unsigned long dontCares[]);
static void TestAllInputs(const unsigned long numVars, const triLogic truthTable[], const unsigned long numTerms, const unsigned long terms[], const unsigned long dontCares[], unsigned long* numRight, unsigned long* numWrong);
static long GetRandomLong(long min, long max);
static void GetRandomBoolArray(unsigned long numElements, char boolArray[]);

int main(int argc, char* argv[])
{
    srand((unsigned int)time(NULL));

    TestSimpleExample();
    TestOneSpecificTruthTable();
    TestOneRandomTruthTable();
    TestEquationGeneration();
    TestAllTruthTables();
    TestAllTruthTablesWithOneFalse();
    TestAllTruthTablesWithOneTrue();
    TestSomeRandomTruthTables();

    return 0;
}

static void TestSimpleExample(void)
{
    printf("\n\n============================================================");
    printf("\n\nPerforming TestSimpleExample test...\n\n");

    const numVars = 4;
    const triLogic truthTable[16] = { 1,1,1,0,0,1,1,1,1,0,0,1,0,0,1,1, };
    unsigned long numTerms;
    unsigned long* terms = NULL;
    unsigned long* dontCares = NULL;
    char* equation = NULL;
    ReduceLogic(numVars, truthTable, &numTerms, &terms, &dontCares);
    GenerateEquationString(numVars, NULL, numTerms, terms, dontCares, &equation);
    printf("%s", equation);
    free(terms);
    free(dontCares);
    free(equation);

    printf("\n");
}

static void TestOneSpecificTruthTable(void)
{
    enum shrinqStatus retVal;
    unsigned long numTerms;
    unsigned long* terms = NULL;
    unsigned long* dontCares = NULL;
    unsigned long numRight = 0;
    unsigned long numWrong = 0;
    unsigned long numFailures = 0;

    printf("\n\n============================================================");
    printf("\n\nPerforming TestOneSpecificTruthTable test...\n\n");

    ResetTermCounters();

    const numVars = 4;
    const triLogic truthTable[16] =
    {
        1,1,1,0,
        0,1,1,1,
        1,0,0,1,
        0,0,1,1,
    };

    unsigned long timer = GetTickCountForOS();
    retVal = ReduceLogic(numVars, truthTable, &numTerms, &terms, &dontCares);
    timer = GetTickCountForOS() - timer;
    printf("Test took %i %s with %i variables...\n", timer, unitsGetTickCount, numVars);

    if (retVal == STATUS_OKAY)
    {
        PrintEquation(numVars, numTerms, terms, dontCares);
        TestAllInputs(numVars, truthTable, numTerms, terms, dontCares, &numRight, &numWrong);
        free(terms);
        free(dontCares);
    }
    else
    {
        numFailures++;
    }

    printf("\nNumber right    : %i", numRight);
    printf("\nNumber wrong    : %i", numWrong);
    printf("\nNumber failures : %i", numFailures);
    printf("\nTerms kept      : %i", GetNumTermsKept());
    printf("\nTerms removed   : %i", GetNumTermsRemoved());
    printf("\n");
}

static void TestOneRandomTruthTable(void)
{
    enum shrinqStatus retVal;
    unsigned long numTerms;
    unsigned long* terms = NULL;
    unsigned long* dontCares = NULL;
    unsigned long numRight = 0;
    unsigned long numWrong = 0;
    unsigned long numFailures = 0;

    printf("\n\n============================================================");
    printf("\n\nPerforming TestOneRandomTruthTable test...\n\n");

    ResetTermCounters();

    const unsigned long numVars = 5;
    unsigned long numOfPossibleInputs = 1 << numVars;
    size_t memSize = numOfPossibleInputs * sizeof(triLogic);
    triLogic* truthTable = (triLogic*)malloc(memSize);
    GetRandomBoolArray(numOfPossibleInputs, truthTable);

    unsigned long timer = GetTickCountForOS();
    retVal = ReduceLogic(numVars, truthTable, &numTerms, &terms, &dontCares);
    timer = GetTickCountForOS() - timer;
    printf("Test took %i %s with %i variables...\n", timer, unitsGetTickCount, numVars);

    if (retVal == STATUS_OKAY)
    {
        PrintEquation(numVars, numTerms, terms, dontCares);
        TestAllInputs(numVars, truthTable, numTerms, terms, dontCares, &numRight, &numWrong);
        free(terms);
        free(dontCares);
    }
    else
    {
        numFailures++;
    }

    printf("\nNumber right    : %i", numRight);
    printf("\nNumber wrong    : %i", numWrong);
    printf("\nNumber failures : %i", numFailures);
    printf("\nTerms kept      : %i", GetNumTermsKept());
    printf("\nTerms removed   : %i", GetNumTermsRemoved());
    printf("\n");
}

static void TestEquationGeneration(void)
{
    enum shrinqStatus retVal;
    unsigned long numTerms;
    unsigned long* terms = NULL;
    unsigned long* dontCares = NULL;
    unsigned long numRight = 0;
    unsigned long numWrong = 0;
    unsigned long numFailures = 0;

    printf("\n\n============================================================");
    printf("\n\nPerforming TestEquationGeneration test...\n\n");

    ResetTermCounters();

    const unsigned long numVars = 4;
    unsigned long numOfPossibleInputs = 1 << numVars;
    size_t memSize = numOfPossibleInputs * sizeof(triLogic);
    triLogic* truthTable = (triLogic*)malloc(memSize);
    GetRandomBoolArray(numOfPossibleInputs, truthTable);

    unsigned long timer = GetTickCountForOS();
    retVal = ReduceLogic(numVars, truthTable, &numTerms, &terms, &dontCares);
    timer = GetTickCountForOS() - timer;
    printf("Test took %i %s with %i variables...\n", timer, unitsGetTickCount, numVars);

    if (retVal == STATUS_OKAY)
    {
        TestAllInputs(numVars, truthTable, numTerms, terms, dontCares, &numRight, &numWrong);

        char* variableNames[] =
        {
            "Apple",
            "Pear",
            "Banana",
            "Mango",
        };

        char* equation;
        GenerateEquationString(numVars, variableNames, numTerms, terms, dontCares, &equation);
        if (equation)
        {
            printf("\n%s\n", equation);
            free(equation);
            equation = NULL;
        }

        free(terms);
        free(dontCares);
    }
    else
    {
        numFailures++;
    }

    printf("\nNumber right    : %i", numRight);
    printf("\nNumber wrong    : %i", numWrong);
    printf("\nNumber failures : %i", numFailures);
    printf("\nTerms kept      : %i", GetNumTermsKept());
    printf("\nTerms removed   : %i", GetNumTermsRemoved());
    printf("\n");
}

static void TestAllTruthTables(void)
{
    enum shrinqStatus retVal;
    unsigned long numOfPossibleInputs;
    unsigned long numOfPossibleTruthTables;
    unsigned long iNumVars;
    unsigned long iInput;
    unsigned long numTerms;
    triLogic* truthTable = NULL;
    unsigned long* terms = NULL;
    unsigned long* dontCares = NULL;
    unsigned long numRight = 0;
    unsigned long numWrong = 0;
    unsigned long numFailures = 0;
    int printEquation = 0;

    printf("\n\n============================================================");
    printf("\n\nPerforming TestAllTruthTables test...\n\n");

    ResetTermCounters();

    const unsigned long startNumVars = 1;
    const unsigned long endNumVars = 4;
    for (iNumVars = startNumVars; iNumVars <= endNumVars; iNumVars++)
    {
        printf("Testing %i variables...\n", iNumVars);
        numOfPossibleInputs = 1 << iNumVars;
        numOfPossibleTruthTables = 1 << numOfPossibleInputs;
        printf("    %i possible inputs.\n", numOfPossibleInputs);
        printf("    %i possible truth tables.\n", numOfPossibleTruthTables);
        truthTable = calloc(numOfPossibleInputs, sizeof(triLogic));
        if (truthTable == NULL)
        {
            numFailures++;
            break;
        }

        while (1)
        {
            retVal = ReduceLogic(iNumVars, truthTable, &numTerms, &terms, &dontCares);
            if (retVal == STATUS_OKAY)
            {
                if (printEquation)
                {
                    PrintEquation(iNumVars, numTerms, terms, dontCares);
                }
                TestAllInputs(iNumVars, truthTable, numTerms, terms, dontCares, &numRight, &numWrong);
                free(terms);
                free(dontCares);
                terms = NULL;
                dontCares = NULL;
            }
            else
            {
                numFailures++;
            }

            // set up the next truth table to test
            for (iInput = 0; iInput < numOfPossibleInputs; iInput++)
            {
                if (truthTable[iInput] == LOGIC_TRUE)
                    truthTable[iInput] = LOGIC_FALSE;
                else
                {
                    truthTable[iInput] = LOGIC_TRUE;
                    break;
                }
            }
            if (iInput == numOfPossibleInputs)
                break;
        }
        if (truthTable)
        {
            free(truthTable);
            truthTable = NULL;
        }
    }

    printf("\nNumber right    : %i", numRight);
    printf("\nNumber wrong    : %i", numWrong);
    printf("\nNumber failures : %i", numFailures);
    printf("\nTerms kept      : %i", GetNumTermsKept());
    printf("\nTerms removed   : %i", GetNumTermsRemoved());
    printf("\n");
}

static void TestAllTruthTablesWithOneFalse(void)
{
    enum shrinqStatus retVal;
    unsigned long numTerms;
    triLogic* truthTable = NULL;
    unsigned long* terms = NULL;
    unsigned long* dontCares = NULL;
    unsigned long numRight = 0;
    unsigned long numWrong = 0;
    unsigned long numFailures = 0;

    printf("\n\n============================================================");
    printf("\n\nPerforming TestAllTruthTablesWithOneFalse test...\n\n");

    ResetTermCounters();

    const unsigned long minVar = 1;
    const unsigned long maxVar = 12;
    for (unsigned long iVars = minVar; iVars <= maxVar; iVars++)
    {
        printf("Testing %d variables...\n", iVars);
        unsigned long numOfPossibleInputs = 1 << iVars;
        truthTable = malloc(numOfPossibleInputs * sizeof(long));
        if (truthTable == NULL)
        {
            numFailures++;
            break;
        }

        for (unsigned long iFalse = 0; iFalse < numOfPossibleInputs; iFalse++)
        {
            for (unsigned long iInput = 0; iInput < numOfPossibleInputs; iInput++)
            {
                truthTable[iInput] = LOGIC_TRUE;
            }
            truthTable[iFalse] = LOGIC_FALSE;
            retVal = ReduceLogic(iVars, truthTable, &numTerms, &terms, &dontCares);
            if (retVal == STATUS_OKAY)
            {
                TestAllInputs(iVars, truthTable, numTerms, terms, dontCares, &numRight, &numWrong);
                free(terms);
                free(dontCares);
                terms = NULL;
                dontCares = NULL;
            }
            else
            {
                numFailures++;
            }
        }

        free(truthTable);
        truthTable = NULL;
    }

    printf("\nNumber right    : %i", numRight);
    printf("\nNumber wrong    : %i", numWrong);
    printf("\nNumber failures : %i", numFailures);
    printf("\nTerms kept      : %i", GetNumTermsKept());
    printf("\nTerms removed   : %i", GetNumTermsRemoved());
    printf("\n");
}

static void TestAllTruthTablesWithOneTrue(void)
{
    enum shrinqStatus retVal;
    unsigned long numTerms;
    triLogic* truthTable = NULL;
    unsigned long* terms = NULL;
    unsigned long* dontCares = NULL;
    unsigned long numRight = 0;
    unsigned long numWrong = 0;
    unsigned long numFailures = 0;

    printf("\n\n============================================================");
    printf("\n\nPerforming TestAllTruthTablesWithOneTrue test...\n\n");

    ResetTermCounters();

    const unsigned long minVar = 1;
    const unsigned long maxVar = 12;
    for (unsigned long iVars = minVar; iVars <= maxVar; iVars++)
    {
        printf("Testing %d variables...\n", iVars);
        unsigned long numOfPossibleInputs = 1 << iVars;
        truthTable = malloc(numOfPossibleInputs * sizeof(long));
        if (truthTable == NULL)
        {
            numFailures++;
            break;
        }

        for (unsigned long iTrue = 0; iTrue < numOfPossibleInputs; iTrue++)
        {
            for (unsigned long iInput = 0; iInput < numOfPossibleInputs; iInput++)
            {
                truthTable[iInput] = LOGIC_FALSE;
            }
            truthTable[iTrue] = LOGIC_TRUE;
            retVal = ReduceLogic(iVars, truthTable, &numTerms, &terms, &dontCares);
            if (retVal == STATUS_OKAY)
            {
                TestAllInputs(iVars, truthTable, numTerms, terms, dontCares, &numRight, &numWrong);
                free(terms);
                free(dontCares);
                terms = NULL;
                dontCares = NULL;
            }
            else
            {
                numFailures++;
            }
        }

        free(truthTable);
        truthTable = NULL;
    }

    printf("\nNumber right    : %i", numRight);
    printf("\nNumber wrong    : %i", numWrong);
    printf("\nNumber failures : %i", numFailures);
    printf("\nTerms kept      : %i", GetNumTermsKept());
    printf("\nTerms removed   : %i", GetNumTermsRemoved());
    printf("\n");
}

static void TestSomeRandomTruthTables(void)
{
    const unsigned long numTests = 10;
    const unsigned long minVar = 13;
    const unsigned long maxVar = 15;

    enum shrinqStatus retVal;
    unsigned long iTest;
    unsigned long numVars;
    unsigned long numOfPossibleInputs;
    unsigned long numTerms;
    triLogic* truthTable = NULL;
    unsigned long* terms = NULL;
    unsigned long* dontCares = NULL;
    unsigned long numRight = 0;
    unsigned long numWrong = 0;
    unsigned long numFailures = 0;
    unsigned long timer = 0;

    printf("\n\n============================================================");
    printf("\n\nPerforming TestSomeRandomTruthTables test...\n\n");

    ResetTermCounters();

    for (iTest = 1; iTest <= numTests; iTest++)
    {
        numVars = GetRandomLong(minVar, maxVar);
        numOfPossibleInputs = 1 << numVars;
        truthTable = malloc(numOfPossibleInputs * sizeof(long));
        GetRandomBoolArray(numOfPossibleInputs, truthTable);
        printf("Test %i with %i variables...\n", iTest, numVars);
        retVal = ReduceLogic(numVars, truthTable, &numTerms, &terms, &dontCares);
        if (retVal == STATUS_OKAY)
        {
            TestAllInputs(numVars, truthTable, numTerms, terms, dontCares, &numRight, &numWrong);
            free(terms);
            free(dontCares);
            terms = NULL;
            dontCares = NULL;
        }
        else
        {
            numFailures++;
        }
        free(truthTable);
        truthTable = NULL;
    }

    printf("\nNumber right    : %i", numRight);
    printf("\nNumber wrong    : %i", numWrong);
    printf("\nNumber failures : %i", numFailures);
    printf("\nTerms kept      : %i", GetNumTermsKept());
    printf("\nTerms removed   : %i", GetNumTermsRemoved());
    printf("\n");
}

// returns a random integer between min and max inclusively
static long GetRandomLong(
    long min,
    long max)
{
    long result = min + (long)(floor(((double)((max - min) * rand()) / ((double)RAND_MAX)) + 0.5));

    return result;
}

static void GetRandomBoolArray(
    unsigned long numElements,
    char boolArray[])
{
    for (unsigned long i = 0; i < numElements; i++)
        boolArray[i] = (char)GetRandomLong(0, 1);
}

static void TestAllInputs(
    const unsigned long numVars,
    const triLogic truthTable[],
    const unsigned long numTerms,
    const unsigned long terms[],
    const unsigned long dontCares[],
    unsigned long* numRight,
    unsigned long* numWrong)
{
    triLogic result;
    unsigned long numOfPossibleInputs = 1 << numVars;

    for (unsigned long iInput = 0; iInput < numOfPossibleInputs; iInput++)
    {
        if (truthTable[iInput] == LOGIC_DONT_CARE)
            (*numRight)++;
        else
        {
            result = EvaluateSumOfProducts(numVars, numTerms, terms, dontCares, iInput);
            if (truthTable[iInput] == result)
                (*numRight)++;
            else
                (*numWrong)++;
        }
    }
}

static void PrintEquation(
    const unsigned long numVars,
    const unsigned long numTerms,
    const unsigned long terms[],
    const unsigned long dontCares[])
{
    char* equation = NULL;
    GenerateEquationString(numVars, NULL, numTerms, terms, dontCares, &equation);
    if (equation)
    {
        printf("\n%s\n", equation);
        free(equation);
        equation = NULL;
    }
}
