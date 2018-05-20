//
// Created by Benny  on 5/15/18.
//

#include "apriori.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    char *fInName = "test_datasets/freqout.txt";
    char *fOutName = "result.txt";
    int minSupport = 50;
    int minSetSize = 2;
    int maxSetSize = 10;
    int targetItem = 0; // Item to search
    int detectedItems[] = {0, 2, 7, 31, 41, 43, 27};
    // Train model
    AssociativeList aList = apTrain(minSupport, maxSetSize, fInName, fOutName);
    // Or read from trained model
    // AssociativeList aList = readTrained(fOutName);
    AssociatedGroupList *aGroups = getAssociatedGroupsOf(aList, targetItem, 7, detectedItems);
    int i, j;
    // Get all associated groups and frequencies
    for (i = 0; i < aGroups->length; i++) {
        for (j = 0; j < aGroups->groups[i].length; j++) {
            printf("%d ", aGroups->groups[i].items[j]);
        }
        printf(": %f\n", aGroups->groups[i].possibility); // Possibility of finding item when group is found
    }
    exit(0);
}