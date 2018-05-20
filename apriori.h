//
// Created by Benny  on 5/15/18.
//

#ifndef APRIORI_APRIORI_H
#define APRIORI_APRIORI_H

#endif //APRIORI_APRIORI_H

#include "lib/cJSON.h"

typedef struct {
    int support;
    int *items;
} Group;

typedef struct {
    int length;
    float possibility;
    int *items;
} AssociatedGroup;

typedef struct {
    int length;
    Group *groups;
} GroupList;

typedef struct {
    int length;
    int item;
    AssociatedGroup *groups;
} AssociatedGroupList;

typedef struct {
    int length;
    GroupList *groupLists;
} FrequentList;

typedef struct {
    int length;
    AssociatedGroupList *groupLists;
} AssociativeList;

FrequentList findFrequent(char *fName, int minSupport, int maxSetSize);

AssociativeList apTrain(int minSupport, int maxSetSize, char *fInName, char *fOutName);

AssociativeList findAssociation(FrequentList fList, char *fOut);

AssociatedGroupList *getAssociatedGroupsOf(AssociativeList aList, int item, int nDetectedItems, int *detectedItems);

AssociativeList readTrained(char *fName);