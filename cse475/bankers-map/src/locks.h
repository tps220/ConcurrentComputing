#pragma once

void initializeLocks(unsigned int size);
void deleteLocks();
bool acquireWriteLock(int idx);
bool releaseWriteLock(int idx);
bool acquireWriteLocks(int i, int j);
bool releaseWriteLocks(int i, int j);
bool acquireAll_Reader();
bool releaseAll_Reader();