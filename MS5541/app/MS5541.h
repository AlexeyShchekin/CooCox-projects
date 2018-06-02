#ifndef MS5541_H
#define MS5541_H

void InitSensor(void);
void ResetSensor(void);
void readCalibration(long C[]);
void readData(long C[], long long DATA[]);

#endif
