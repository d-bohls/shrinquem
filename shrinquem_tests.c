// shrinquem - An algorithm for logic minimization.
// Copyright (C) 2021 Damon Bohls <damonbohls@gmail.com>
// MIT License: https://github.com/d-bohls/shrinquem/blob/main/LICENSE

#include <stdlib.h>
#include <stdio.h> // used for printf, ect.
#include <time.h> // used for time() to seed srand
#include <math.h> // used for floor
#include "shrinquem.h"

#pragma warning( disable : 28159)

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

// Test function declarations
static void GitHubExample1(void);
static void GitHubExample2(void);
static void TestSimpleExample(void);
static void TestOneSpecificTruthTable(void);
static void TestOneRandomTruthTable(void);
static void TestEquationGeneration(void);
static void TestAllTruthTables(void);
static void TestAllTruthTablesWithOneFalse(void);
static void TestAllTruthTablesWithOneTrue(void);
static void TestSomeRandomTruthTables(void);

// Helper functions
static void TestAllInputs(const SumOfProducts sumOfProducts, const triLogic truthTable[], unsigned long* numRight, unsigned long* numWrong);
static long GetRandomLong(long min, long max);
static void GetRandomBoolArray(unsigned long numElements, char boolArray[]);

int main(int argc, char* argv[])
{
    srand((unsigned int)time(NULL));

    GitHubExample1();
    GitHubExample2();
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

static void GitHubExample1(void)
{
    printf("\n\n============================================================");
    printf("\n\nGitHub example 1...\n\n");
    const numVars = 3;
    const triLogic truthTable[8] = { 1,1,0,1,1,0,0,0 };
    SumOfProducts sumOfProducts = { numVars };
    ReduceLogic(truthTable, &sumOfProducts);
    GenerateEquationString(&sumOfProducts, NULL);
    printf("f(A, B, C) = %s", sumOfProducts.equation);
    FinalizeSumOfProducts(&sumOfProducts);
}

static void GitHubExample2(void)
{
    printf("\n\n============================================================");
    printf("\n\nGitHub example 2...\n\n");
    const numVars = 2;
    const triLogic truthTable[4] =
    {
        LOGIC_FALSE,
        LOGIC_TRUE,
        LOGIC_FALSE,
        LOGIC_DONT_CARE
    };
    SumOfProducts sumOfProducts = { numVars };
    ReduceLogic(truthTable, &sumOfProducts);
    GenerateEquationString(&sumOfProducts, NULL);
    printf("f(A, B) = %s", sumOfProducts.equation);
    FinalizeSumOfProducts(&sumOfProducts);
}

static void TestSimpleExample(void)
{
    printf("\n\n============================================================");
    printf("\n\nPerforming TestSimpleExample test...\n\n");
    const triLogic truthTable[16] = { 1,1,1,0,0,1,1,1,1,0,0,1,0,0,1,1 };
    SumOfProducts sumOfProducts = { 4 };
    ReduceLogic(truthTable, &sumOfProducts);
    GenerateEquationString(&sumOfProducts, NULL);
    printf("%s\n", sumOfProducts.equation);
    FinalizeSumOfProducts(&sumOfProducts);
}

static void TestOneSpecificTruthTable(void)
{
    shrinquemStatus retVal;
    unsigned long numRight = 0;
    unsigned long numWrong = 0;
    unsigned long numFailures = 0;

    printf("\n\n============================================================");
    printf("\n\nPerforming TestOneSpecificTruthTable test...\n\n");

    ResetTermCounters();

    SumOfProducts sumOfProducts = { 4 };
    const triLogic truthTable[16] =
    {
        1,1,1,0,
        0,1,1,1,
        1,0,0,1,
        0,0,1,1,
    };

    unsigned long timer = GetTickCountForOS();
    retVal = ReduceLogic(truthTable, &sumOfProducts);
    timer = GetTickCountForOS() - timer;
    printf("Test took %i %s with %i variables...\n", timer, unitsGetTickCount, sumOfProducts.numVars);

    if (retVal == STATUS_OKAY)
    {
        GenerateEquationString(&sumOfProducts, NULL);
        printf("\n%s\n", sumOfProducts.equation);
        TestAllInputs(sumOfProducts, truthTable, &numRight, &numWrong);
        FinalizeSumOfProducts(&sumOfProducts);
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
    shrinquemStatus retVal;
    unsigned long numRight = 0;
    unsigned long numWrong = 0;
    unsigned long numFailures = 0;

    printf("\n\n============================================================");
    printf("\n\nPerforming TestOneRandomTruthTable test...\n\n");

    ResetTermCounters();

    SumOfProducts sumOfProducts = { 5 };
    unsigned long numOfPossibleInputs = 1 << sumOfProducts.numVars;
    size_t memSize = numOfPossibleInputs * sizeof(triLogic);
    triLogic* truthTable = (triLogic*)malloc(memSize);
    GetRandomBoolArray(numOfPossibleInputs, truthTable);

    unsigned long timer = GetTickCountForOS();
    retVal = ReduceLogic(truthTable, &sumOfProducts);
    timer = GetTickCountForOS() - timer;
    printf("Test took %i %s with %i variables...\n", timer, unitsGetTickCount, sumOfProducts.numVars);

    if (retVal == STATUS_OKAY)
    {
        GenerateEquationString(&sumOfProducts, NULL);
        printf("\n%s\n", sumOfProducts.equation);
        TestAllInputs(sumOfProducts, truthTable, &numRight, &numWrong);
        FinalizeSumOfProducts(&sumOfProducts);
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
    shrinquemStatus retVal;
    unsigned long numRight = 0;
    unsigned long numWrong = 0;
    unsigned long numFailures = 0;

    printf("\n\n============================================================");
    printf("\n\nPerforming TestEquationGeneration test...\n\n");

    ResetTermCounters();

    SumOfProducts sumOfProducts = { 4 };
    unsigned long numOfPossibleInputs = 1 << sumOfProducts.numVars;
    size_t memSize = numOfPossibleInputs * sizeof(triLogic);
    triLogic* truthTable = (triLogic*)malloc(memSize);
    GetRandomBoolArray(numOfPossibleInputs, truthTable);

    unsigned long timer = GetTickCountForOS();
    retVal = ReduceLogic(truthTable, &sumOfProducts);
    timer = GetTickCountForOS() - timer;
    printf("Test took %i %s with %i variables...\n", timer, unitsGetTickCount, sumOfProducts.numVars);

    if (retVal == STATUS_OKAY)
    {
        TestAllInputs(sumOfProducts, truthTable, &numRight, &numWrong);

        char* variableNames[] =
        {
            "Apple",
            "Pear",
            "Banana",
            "Mango",
        };

        GenerateEquationString(&sumOfProducts, variableNames);
        printf("\n%s\n", sumOfProducts.equation);
        TestAllInputs(sumOfProducts, truthTable, &numRight, &numWrong);
        FinalizeSumOfProducts(&sumOfProducts);
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
    shrinquemStatus retVal;
    unsigned long numOfPossibleInputs;
    unsigned long numOfPossibleTruthTables;
    unsigned long iNumVars;
    unsigned long iInput;
    triLogic* truthTable = NULL;
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
            SumOfProducts sumOfProducts = { iNumVars };
            retVal = ReduceLogic(truthTable, &sumOfProducts);
            if (retVal == STATUS_OKAY)
            {
                if (printEquation)
                {
                    GenerateEquationString(&sumOfProducts, NULL);
                    printf("\n%s\n", sumOfProducts.equation);
                }

                TestAllInputs(sumOfProducts, truthTable, &numRight, &numWrong);
                FinalizeSumOfProducts(&sumOfProducts);
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
    shrinquemStatus retVal;
    triLogic* truthTable = NULL;
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

            SumOfProducts sumOfProducts = { iVars };
            retVal = ReduceLogic(truthTable, &sumOfProducts);
            if (retVal == STATUS_OKAY)
            {
                TestAllInputs(sumOfProducts, truthTable, &numRight, &numWrong);
                FinalizeSumOfProducts(&sumOfProducts);
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
    shrinquemStatus retVal;
    triLogic* truthTable = NULL;
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

            SumOfProducts sumOfProducts = { iVars };
            retVal = ReduceLogic(truthTable, &sumOfProducts);
            if (retVal == STATUS_OKAY)
            {
                TestAllInputs(sumOfProducts, truthTable, &numRight, &numWrong);
                FinalizeSumOfProducts(&sumOfProducts);
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

    shrinquemStatus retVal;
    unsigned long iTest;
    unsigned long numOfPossibleInputs;
    triLogic* truthTable = NULL;
    unsigned long numRight = 0;
    unsigned long numWrong = 0;
    unsigned long numFailures = 0;
    unsigned long timer = 0;

    printf("\n\n============================================================");
    printf("\n\nPerforming TestSomeRandomTruthTables test...\n\n");

    ResetTermCounters();

    for (iTest = 1; iTest <= numTests; iTest++)
    {
        SumOfProducts sumOfProducts = { GetRandomLong(minVar, maxVar) };
        numOfPossibleInputs = 1 << sumOfProducts.numVars;
        truthTable = malloc(numOfPossibleInputs * sizeof(long));
        GetRandomBoolArray(numOfPossibleInputs, truthTable);
        printf("Test %i with %i variables...\n", iTest, sumOfProducts.numVars);

        retVal = ReduceLogic(truthTable, &sumOfProducts);
        if (retVal == STATUS_OKAY)
        {
            TestAllInputs(sumOfProducts, truthTable, &numRight, &numWrong);
            FinalizeSumOfProducts(&sumOfProducts);
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
    const SumOfProducts sumOfProducts,
    const triLogic truthTable[],
    unsigned long* numRight,
    unsigned long* numWrong)
{
    triLogic result;
    unsigned long numOfPossibleInputs = 1 << sumOfProducts.numVars;

    for (unsigned long iInput = 0; iInput < numOfPossibleInputs; iInput++)
    {
        if (truthTable[iInput] == LOGIC_DONT_CARE)
            (*numRight)++;
        else
        {
            result = EvaluateSumOfProducts(sumOfProducts, iInput);
            if (truthTable[iInput] == result)
                (*numRight)++;
            else
                (*numWrong)++;
        }
    }
}
