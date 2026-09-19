#ifndef TIMEPARSER_H
#define TIMEPARSER_H
 
// Error codes
#define TIME_LEN_ERROR      -1
#define TIME_ARRAY_ERROR    -2
#define TIME_VALUE_ERROR    -3
 
#define SEQ_OK             0
#define SEQ_NULL_ERROR    -10
#define SEQ_COLOR_ERROR   -11
#define SEQ_FORMAT_ERROR  -12
#define SEQ_TIME_ERROR    -13
 
using namespace std;
 
int time_parse(char *time);
int sequence_parse(const char *seq);
 
#endif