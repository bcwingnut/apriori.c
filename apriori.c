#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "lib/cJSON.h"
#include "apriori.h"
#include "lib/uthash.h"

#define MAX_LINE_LENGTH 1000
#define MAX_ITEM_DIGITS 20

int comp2Length;
AssociatedGroupList *queriedList = NULL;
int maxQueriedListSize = 100;

struct GroupCount {
    char *strGroup;
    int count;
    UT_hash_handle hh;
} *Counter = NULL;

int comp1(const void *a, const void *b) {
    return (*(int *) a - *(int *) b);
}

int comp2(const void *a, const void *b) {
    int i;
    int *ap = *(int **) a;
    int *bp = *(int **) b;
    for (i = 0; i < comp2Length; i++) {
        if (ap[i] != bp[i])
            return ap[i] - bp[i];
    }
    return 0;
}

char *group2Str(int size, int *group) {
    int i;
    char str[MAX_LINE_LENGTH] = "";
    for (i = 0; i < size; i++) {
        char t[MAX_ITEM_DIGITS];
        sprintf(t, "%d ", group[i]);
        strcat(str, t);
    }
    char *p = malloc(strlen(str) + 1);
    for (i = 0; i < strlen(str); i++) {
        p[i] = str[i];
    }
    p[i] = '\0';
    return p;
}

int compGroup(const void *a, const void *b) {
    AssociatedGroup g1 = *(AssociatedGroup *) a;
    AssociatedGroup g2 = *(AssociatedGroup *) b;
    if (g1.possibility > g2.possibility) return -1;
    return 1;
}

int *initArray(int len) {
    int i;
    int *arr = malloc(len * sizeof(int));
    for (i = 0; i < len; i++) {
        arr[i] = 0;
    }
    return arr;
}

int arrayEqual(int length, const int *arr1, const int *arr2) {
    int i;
    for (i = 0; i < length; i++) {
        if (arr1[i] != arr2[i]) {
            return 0;
        }
    }
    return 1;
}

int arrayDiff(int length, const int *arr1, const int *arr2) {
    int diff = 0, i;
    for (i = 0; i < length; i++) {
        if (arr1[i] != arr2[i]) {
            diff++;
        }
    }
    return diff;
}

int *arrayExcept(int length, const int *arr, int num) {
    int *result = (int *) malloc((length - 1) * sizeof(int));
    int pos = 0, i;
    for (i = 0; i < length; i++) {
        if (arr[i] != num) result[pos++] = arr[i];
    }
    return result;
}

int containsAll(int lengthLong, int lengthShort, const int *arrLong, const int *arrShort) {
    // Arrays must be in ascending order
    int i = 0, j = 0;
    while (i < lengthShort) {
        if (j == lengthLong || arrShort[i] < arrLong[j]) return 0;
        else if (arrShort[i] == arrLong[j]) i++;
        j++;
    }
    return 1;
}

int *arrayJoin(int length, const int *arr1, const int *arr2) {
    int i;
    int *result = (int *) malloc((length + 1) * sizeof(int));
    for (i = 0; i < length; i++) {
        result[i] = arr1[i];
        if (arr1[i] != arr2[i]) result[length] = arr2[i];
    }
    qsort(result, (size_t) (length) + 1, sizeof(int), comp1);
    return result;
}

int **dropInfrequent(int candidatesSize, int groupSize, int **candidates,
                     int minSupport, const int *counter, int **support, int *freqSize) {
    int i, j = 0;
    freqSize[groupSize] = 0;
    for (i = 0; i < candidatesSize; i++) {
        if (counter[i] >= minSupport) (freqSize[groupSize])++;
    }
    int **freq = (int **) malloc(freqSize[groupSize] * sizeof(int *));
    support[groupSize] = initArray(freqSize[groupSize]);
    for (i = 0; i < candidatesSize; i++) {
        if (counter[i] >= minSupport) {
            support[groupSize][j] = counter[i];
            freq[j++] = candidates[i];
        }
    }
    return freq;
}

int **generatePrev(int groupSize, int lastPrevSize, int lastFreqSize, int **lastPrev, int *pPrevSize, int **lastFreq) {
    int i, j, k = 0, cnt;
    comp2Length = groupSize - 1;
    int pfSize = 0;
    for (i = 0; i < lastPrevSize; i++) {
        if (bsearch(&lastPrev[i], lastFreq, (size_t) lastFreqSize, sizeof(int *), comp2) != NULL)
            pfSize++;
    }
    int **pf = (int **) malloc(pfSize * sizeof(int *)); // prev in last frequent list
    cnt = 0;
    for (i = 0; i < lastPrevSize; i++) {
        if (bsearch(&lastPrev[i], lastFreq, (size_t) lastFreqSize, sizeof(int *), comp2) != NULL)
            pf[cnt++] = lastPrev[i];
    }
    cnt = 0;
    for (i = 0; i < pfSize - 1; i++)
        for (j = i + 1; j < pfSize; j++)
            if (arrayDiff(groupSize - 1, pf[i], pf[j]) == 1) cnt++;
    if (cnt == 0) {
        *pPrevSize = 0;
        return NULL;
    }
    int **pwr = (int **) malloc(cnt * sizeof(int *)); //prev with repeat
    for (i = 0; i < pfSize - 1; i++)
        for (j = i + 1; j < pfSize; j++)
            if (arrayDiff(groupSize - 1, pf[i], pf[j]) == 1)
                pwr[k++] = arrayJoin(groupSize - 1, pf[i], pf[j]);
    comp2Length = groupSize;
    qsort(pwr, (size_t) cnt, sizeof(int *), comp2);
    int size = 1;
    for (i = 1; i < cnt; i++)
        if (arrayEqual(groupSize, pwr[i - 1], pwr[i]) == 0) size++;
    *pPrevSize = size;
    int **prev = (int **) malloc(size * sizeof(int *));
    k = 0;
    prev[0] = pwr[0];
    for (i = 1; i < cnt; i++)
        if (arrayEqual(groupSize, pwr[i - 1], pwr[i]) == 0)
            prev[++k] = pwr[i];
    return prev;
}

void addFrequent(char *str) {
    struct GroupCount *gCount;
    HASH_FIND_STR(Counter, str, gCount);
    if (gCount == NULL) {
        gCount = (struct GroupCount *) malloc(sizeof(struct GroupCount));
        gCount->strGroup = str;
        gCount->count = 1;
        HASH_ADD_KEYPTR(hh, Counter, gCount->strGroup, strlen(gCount->strGroup), gCount);
    } else
        gCount->count++;
}

int getCountOf(char *str) {
    struct GroupCount *gCount;
    HASH_FIND_STR(Counter, str, gCount);
    if (gCount != NULL) {
        return gCount->count;
    } else return 0;
}

int **generateCandidates(int nTrans, const int *prevSize, int k, int ***prev, int *candidatesSize) {
    int cnt = 0, i, j;
    for (i = 0; i < nTrans; i++) cnt += prevSize[i];
    int **cwr = (int **) malloc(cnt * sizeof(int *)); // Candidates with repeat
    cnt = 0;
    for (i = 0; i < nTrans; i++) {
        for (j = 0; j < prevSize[i]; j++) {
            cwr[cnt++] = prev[i][j];
            char *str = group2Str(k, cwr[cnt - 1]);
            addFrequent(str);
        }
    }
    comp2Length = k;
    qsort(cwr, (size_t) cnt, sizeof(int *), comp2);
    int nCandidates = 1;
    for (i = 1; i < cnt; i++) {
        if (arrayEqual(k, cwr[i], cwr[i - 1]) == 0) nCandidates++;
    }
    *candidatesSize = nCandidates;
    int **candidates = (int **) malloc(nCandidates * sizeof(int *));
    j = 1;
    candidates[0] = cwr[0];
    for (i = 1; i < cnt; i++) {
        if (arrayEqual(k, cwr[i], cwr[i - 1]) == 0) candidates[j++] = cwr[i];
    }
    return candidates;
}

int *arrayRemoveRepeat(int length, const int *array, int *realLength) {
    // Array must be sorted ascending
    int i, j = 1, len = 1;
    for (i = 1; i < length; i++) if (array[i] != array[i - 1]) len++;
    int *arr = initArray(len);
    *realLength = len;
    arr[0] = array[0];
    for (i = 1; i < length; i++) if (array[i] != array[i - 1]) arr[j++] = array[i];
    return arr;
}

int *count(int candidatesSize, int groupSize, int **candidates) {
    int *ctr = (int *) malloc(candidatesSize * sizeof(int));
    int i;
    comp2Length = groupSize;
    for (i = 0; i < candidatesSize; i++) {
        char *str = group2Str(groupSize, candidates[i]);
        ctr[i] = getCountOf(str);
    }
    return ctr;
}

FrequentList findFrequent(char *fName, int minSupport, int maxSetSize) {
    int item, i, j, **candidates, *counter, candidatesSize; // Temporary variables
    int *transSize, nTrans = 0, **records, *trans, ***freq, *freqSize, **support, *items;
    int ***prev, *prevSize;
    clock_t time[maxSetSize + 1];
    printf("Start training.\n");
    time[0] = clock();
    freq = (int ***) malloc((maxSetSize + 1) * sizeof(int **));
    freqSize = initArray(maxSetSize + 1);
    support = (int **) malloc((maxSetSize + 1) * sizeof(int *));
    FILE *f = fopen(fName, "r");
    if (f == NULL) {
        printf("Cannot read file '%s'.\n", fName);
        exit(-1);
    }
    char *p;
    char line[MAX_LINE_LENGTH];
    // Get number of transactions
    while (fgets(line, MAX_LINE_LENGTH, f) != NULL) nTrans++;
    fclose(f);
    records = (int **) malloc(nTrans * sizeof(int **));
    prev = (int ***) malloc(nTrans * sizeof(int **));
    prevSize = (int *) malloc(nTrans * sizeof(int *));
    transSize = initArray(nTrans);
    f = fopen(fName, "r");
    i = 0;
    while (fgets(line, MAX_LINE_LENGTH, f) != NULL) {
        p = line;
        int twrSize = 0; // Size with repeat
        j = 0;
        while (*p != 0) {
            while (*p == ' ' || *p == '\t') p++;
            if (sscanf(p, "%d", &item) == 1) {
                twrSize++;
            }
            p++;
            while (*p >= '0' && *p <= '9') p++;
        }
        int *twr = initArray(twrSize); // Transcript with repeat
        p = line;
        while (*p != 0) {
            while (*p == ' ' || *p == '\t') p++;
            if (sscanf(p, "%d", &item) == 1) twr[j++] = item;
            p++;
            while (*p >= '0' && *p <= '9') p++;
        }
        qsort(twr, (size_t) twrSize, sizeof(int), comp1);
        trans = arrayRemoveRepeat(twrSize, twr, &transSize[i]); // Transaction without repeat items
        records[i++] = trans;
    }

    // Generate candidates 1
    int nItems = 0; // All items in data set, with many repeat items
    for (i = 0; i < nTrans; i++) {
        nItems += transSize[i];
        prevSize[i] = transSize[i];
        prev[i] = (int **) malloc(prevSize[i] * sizeof(int *));
        for (j = 0; j < transSize[i]; j++) {
            prev[i][j] = initArray(1);
            prev[i][j][0] = records[i][j];
        }
    }
    items = (int *) malloc(nItems * sizeof(int));
    int k = 0;
    comp2Length = 1;
    for (i = 0; i < nTrans; i++) {
        for (j = 0; j < transSize[i]; j++) {
            items[k++] = records[i][j];
        }
    }
    qsort(items, (size_t) nItems, sizeof(int), comp1);
    candidatesSize = 1;
    for (i = 1; i < nItems; i++)if (items[i] != items[i - 1]) candidatesSize++;
    candidates = generateCandidates(nTrans, prevSize, 1, prev, &candidatesSize);
    counter = count(candidatesSize, 1, candidates);
    freq[1] = dropInfrequent(candidatesSize, 1, candidates, minSupport, counter, support, freqSize);
    comp2Length = 1;
    qsort(freq[1], (size_t) freqSize[1], sizeof(int *), comp2);
    time[1] = clock();
    printf("%lf Frequent 1-item-sets generated, size = %d.\n", (double) (time[1] - time[0]) / CLOCKS_PER_SEC,
           freqSize[1]);
    int mlf = 1; // Max length of frequent groups
    for (i = 2; i <= maxSetSize && freqSize[i - 1] >= i; i++) {
        for (j = 0; j < nTrans; j++) {
            prev[j] = generatePrev(i, prevSize[j], freqSize[i - 1], prev[j], &prevSize[j], freq[i - 1]);
        }
        candidates = generateCandidates(nTrans, prevSize, i, prev, &candidatesSize);
        counter = count(candidatesSize, i, candidates);
        freq[i] = dropInfrequent(candidatesSize, i, candidates, minSupport, counter, support, freqSize);
        if (freqSize[i] > 0) {
            mlf++;
            time[i] = clock();
            printf("%lf Frequent %d-item-sets generated, size = %d.\n", (double) (time[i] - time[0]) / CLOCKS_PER_SEC,
                   i, freqSize[i]);
        }
    }
    FrequentList frequentList;
    frequentList.length = mlf + 1;
    frequentList.groupLists = (GroupList *) malloc(frequentList.length * sizeof(GroupList));
    for (i = 1; i < frequentList.length; i++) {
        frequentList.groupLists[i].length = freqSize[i];
        frequentList.groupLists[i].groups = (Group *) malloc(freqSize[i] * sizeof(Group));
        for (j = 0; j < freqSize[i]; j++) {
            frequentList.groupLists[i].groups[j].support = support[i][j];
            frequentList.groupLists[i].groups[j].items = freq[i][j];
        }
    }
    return frequentList;
}

cJSON *aList2JSON(AssociativeList aList) {
    cJSON *aListJson = cJSON_CreateArray();
    int i, j;
    for (i = 0; i < aList.length; i++) {
        cJSON *groupListJson = cJSON_CreateObject();
        cJSON *groupsJson = cJSON_CreateArray();
        cJSON_AddNumberToObject(groupListJson, "i", aList.groupLists[i].item);
        cJSON_AddItemToObject(groupListJson, "g", groupsJson);
        for (j = 0; j < aList.groupLists[i].length; j++) {
            cJSON *groupJson = cJSON_CreateObject();
            cJSON *itemsJson = cJSON_CreateIntArray(aList.groupLists[i].groups[j].items,
                                                    aList.groupLists[i].groups[j].length);
            cJSON_AddItemToObject(groupJson, "i", itemsJson);
            cJSON_AddNumberToObject(groupJson, "p", aList.groupLists[i].groups[j].possibility);
            cJSON_AddItemToArray(groupsJson, groupJson);
        }
        cJSON_AddItemToArray(aListJson, groupListJson);
    }
    return aListJson;
}

AssociativeList JSON2AList(cJSON *aListJson) {
    AssociativeList aList;
    int i, j, k;
    aList.length = cJSON_GetArraySize(aListJson);
    aList.groupLists = (AssociatedGroupList *) malloc(aList.length * sizeof(AssociatedGroupList));
    for (i = 0; i < aList.length; i++) {
        cJSON *groupListJson = cJSON_GetArrayItem(aListJson, i);
        cJSON *groupsJson = cJSON_GetObjectItem(groupListJson, "g");
        aList.groupLists[i].item = cJSON_GetObjectItem(groupListJson, "i")->valueint;
        aList.groupLists[i].length = cJSON_GetArraySize(groupsJson);
        aList.groupLists[i].groups = (AssociatedGroup *)
                malloc(aList.groupLists[i].length * sizeof(AssociatedGroup));
        for (j = 0; j < aList.groupLists[i].length; j++) {
            cJSON *groupJson = cJSON_GetArrayItem(groupsJson, j);
            cJSON *itemsJson = cJSON_GetObjectItem(groupJson, "i");
            aList.groupLists[i].groups[j].length = cJSON_GetArraySize(itemsJson);
            aList.groupLists[i].groups[j].possibility = (float) cJSON_GetObjectItem(groupJson, "p")->valuedouble;
            aList.groupLists[i].groups[j].items = (int *) malloc(aList.groupLists[i].groups[j].length * sizeof(int));
            for (k = 0; k < aList.groupLists[i].groups[j].length; k++) {
                aList.groupLists[i].groups[j].items[k] = cJSON_GetArrayItem(itemsJson, k)->valueint;
            }
        }
    }
    return aList;
}

AssociativeList findAssociation(FrequentList fList, char *fOutName) {
    FILE *fOut = fopen(fOutName, "w");
    int i, j, k, u;
    int length = fList.groupLists[1].length;
    AssociativeList aList;
    aList.length = length;
    aList.groupLists = (AssociatedGroupList *) malloc(length * sizeof(AssociatedGroupList));
    for (i = 0; i < length; i++) {
        aList.groupLists[i].item = fList.groupLists[1].groups[i].items[0];
        int nGroups = 1;
        // Find groups with item
        for (j = 2; j < fList.length; j++) {
            for (k = 0; k < fList.groupLists[j].length; k++) {
                if (containsAll(j, 1, fList.groupLists[j].groups[k].items,
                                fList.groupLists[1].groups[i].items)) {
                    nGroups++;
                }
            }
        }
        aList.groupLists[i].length = nGroups;
        aList.groupLists[i].groups = (AssociatedGroup *) malloc(nGroups * sizeof(AssociatedGroup));
        AssociatedGroup self;
        self.possibility = 1;
        self.length = 1;
        self.items = initArray(0);
        self.items[0] = aList.groupLists[i].item;
        aList.groupLists[i].groups[0] = self;
        int groupInd = 1;
        for (j = 2; j < fList.length; j++) {
            for (k = 0; k < fList.groupLists[j].length; k++) {
                if (containsAll(j, 1, fList.groupLists[j].groups[k].items,
                                fList.groupLists[1].groups[i].items)) {
                    aList.groupLists[i].groups[groupInd].length = j - 1;
                    aList.groupLists[i].groups[groupInd].items =
                            arrayExcept(j, fList.groupLists[j].groups[k].items,
                                        fList.groupLists[1].groups[i].items[0]);
                    aList.groupLists[i].groups[groupInd].possibility = 0;
                    for (u = 0; u < fList.groupLists[j - 1].length; u++) {
                        if (arrayDiff(j - 1, aList.groupLists[i].groups[groupInd].items,
                                      fList.groupLists[j - 1].groups[u].items) == 0) {
                            aList.groupLists[i].groups[groupInd].possibility =
                                    (float) (fList.groupLists[j].groups[k].support) /
                                    fList.groupLists[j - 1].groups[u].support;
                            break;
                        }
                    }
                    groupInd++;
                }
            }
        }
        qsort(aList.groupLists[i].groups + 1, (size_t) (aList.groupLists[i].length - 1), sizeof(AssociatedGroupList),
              compGroup);
    }
    cJSON *aListJson = aList2JSON(aList);
    fprintf(fOut, "%s", cJSON_Print(aListJson));
    fclose(fOut);
    return aList;
};

void printFrequentList(FrequentList frequentList) {
    int i;
    for (i = 0; i < frequentList.length; i++) {
        for (int j = 0; j < frequentList.groupLists[i].length; j++) {
            for (int k = 0; k < i; k++) {
                printf("%d ", frequentList.groupLists[i].groups[j].items[k]);
            }
            printf(":%d\n", frequentList.groupLists[i].groups[j].support);
        }
    }
}

void printAssociativeList(AssociativeList aList) {
    int i, j, k;
    for (i = 0; i < aList.length; i++) {
        printf("%d:\n", aList.groupLists[i].item);
        for (j = 0; j < aList.groupLists[i].length; j++) {
            for (k = 0; k < aList.groupLists[i].groups[j].length; k++) {
                printf("%d ", aList.groupLists[i].groups[j].items[k]);
            }
            printf(":%f\n", aList.groupLists[i].groups[j].possibility);
        }
    }
}

AssociatedGroupList *getAssociatedGroupsOf(AssociativeList aList, int item, int nDetectedItems, int *detectedItems) {
    int i;
    AssociatedGroupList aGList;
    qsort(detectedItems, (size_t) nDetectedItems, sizeof(int), comp1);
    if (queriedList == NULL) {
        queriedList = (AssociatedGroupList *) malloc(sizeof(AssociatedGroupList));
        queriedList->groups = (AssociatedGroup *) malloc(maxQueriedListSize * sizeof(AssociatedGroup));
    }
    for (i = 0; i < aList.length; i++) {
        if (aList.groupLists[i].item == item)
            aGList = aList.groupLists[i];
    }
    queriedList->length = 0;
    for (i = 0; i < aGList.length; i++) {
        if (containsAll(nDetectedItems, aGList.groups[i].length, detectedItems, aGList.groups[i].items))
            queriedList->length++;
    }
    if (queriedList->length > maxQueriedListSize) {
        maxQueriedListSize = queriedList->length;
        free(queriedList->groups);
        queriedList->groups = (AssociatedGroup *) malloc(maxQueriedListSize * sizeof(AssociatedGroup));
    }
    int cnt = 0;
    for (i = 0; i < aGList.length; i++) {
        if (containsAll(nDetectedItems, aGList.groups[i].length, detectedItems, aGList.groups[i].items))
            queriedList->groups[cnt++] = aGList.groups[i];
    }
    return queriedList;
};

AssociativeList apTrain(int minSupport, int maxSetSize, char *fInName, char *fOutName) {
    FrequentList fList = findFrequent(fInName, minSupport, maxSetSize);
    return findAssociation(fList, fOutName);
}

AssociativeList readTrained(char *fName) {
    FILE *fp = fopen(fName, "rb");
    char *str;
    //char txt[65537];
    int fileSize;
    int curlen = 0;
    fseek(fp, 0, SEEK_END);
    fileSize = (int) ftell(fp);
    str = (char *) malloc((size_t) fileSize);
    str[0] = 0;
    rewind(fp);
    printf("Start reading while loop!\n");
    char *curpos = str;
    while ((fgets(curpos, 65536, fp)) != NULL) {
        curlen = strlen(curpos);
        curpos += curlen;
    }
    fclose(fp);
    printf("File Read Finished!\n");
    printf("Parsing...\n");
    cJSON *aListJson = cJSON_Parse(str);
    return JSON2AList(aListJson);
}