#pragma once

void initializeLocks(unsigned int size);
void deleteLocks();
bool acquireLock(int idx);
bool releaseLock(int idx);
bool acquireLocks(int i, int j);
bool releaseLocks(int i, int j);
bool acquireAll_Reader();
bool releaseAll_Reader();
