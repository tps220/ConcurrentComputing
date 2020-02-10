#include <iostream>

class Range {
    int start;
    int end;
    bool hasValue(int val) {
        return val >= start && val <= end;
    }
};